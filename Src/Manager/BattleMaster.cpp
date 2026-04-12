#define NOMINMAX
#include "BattleMaster.h"
#include <DxLib.h>
#include <cmath>
#include <algorithm>
#include "../Input/InputManager.h"

namespace App {

    BattleMaster::BattleMaster()
        : m_currentPhase(Phase::PlayerTurn)
        // 9x9マップに合わせたオフセット調整
        , m_mapGrid(80, Vector2(100, 100))
        , m_isPlayerSelected(false)
        , m_hoverGrid(-1, -1)
        , m_enemyAIStarted(false)
    {
    }

    void BattleMaster::AddLog(const std::string& message) {
        m_actionLog.push_back(message);
        if (m_actionLog.size() > 6) {
            m_actionLog.erase(m_actionLog.begin());
        }
    }

    void BattleMaster::Init() {
        // 四隅と中央の配置を意識した初期位置
        m_player = std::make_unique<Player>(IntVector2{ 1, 1 }, m_mapGrid.GetCellCenter(1, 1), 5, 5, 5);
        m_enemy = std::make_unique<Enemy>(IntVector2{ 7, 7 }, m_mapGrid.GetCellCenter(7, 7), 7, 5, 5);

        m_currentPhase = Phase::PlayerTurn;
        m_isPlayerSelected = false;

        m_actionLog.clear();
        AddLog(">>> バトル開始！ 敵を殲滅せよ！");
    }

    // 共通の移動判定ロジック（数字による距離と方向の制限）
    bool BattleMaster::CanMove(int number, char op, IntVector2 start, IntVector2 target, int& outCost) const {
        if (start == target) return false;

        int dx = std::abs(target.x - start.x);
        int dy = std::abs(target.y - start.y);

        // 基本の移動距離（1,4,7=1マス | 2,5,8=2マス | 3,6,9=3マス）
        int maxDist = (number - 1) % 3 + 1;

        bool isValidDirection = false;
        bool isJump = false; // 「÷」のドット部分へのジャンプ判定

        // 1. 数字による基本の方向判定
        if (number >= 1 && number <= 3) {
            isValidDirection = (dx == 0 || dy == 0); // 縦横
        }
        else if (number >= 4 && number <= 6) {
            isValidDirection = (dx == dy); // 斜め
        }
        else if (number >= 7 && number <= 9) {
            isValidDirection = (dx == 0 || dy == 0 || dx == dy); // 縦横斜め（全方向）
        }

        // 2. 装備している演算子による方向の「追加」
        if (op == '+') {
            isValidDirection |= (dx == 0 || dy == 0); // 縦横を追加
        }
        else if (op == '-') {
            isValidDirection |= (dy == 0); // 横のみを追加
        }
        else if (op == '*') {
            isValidDirection |= (dx == dy); // 斜めを追加
        }
        else if (op == '/') {
            // 「÷」の特殊な形を追加
            isValidDirection |= (dy == 0); // 真ん中の線：横方向を追加

            // 上下のドット：縦にちょうど2マスの位置なら「ジャンプ」可能
            if (dx == 0 && dy == 2) {
                isValidDirection = true;
                isJump = true; // ジャンプフラグを立てる
            }
        }

        // 3. 距離制限の適用
        if (isValidDirection) {
            if (isJump) {
                // 「÷」のジャンプは、自分の数字の距離制限を無視してコスト2で飛べる
                outCost = 2;
                return true;
            }
            else if (dx <= maxDist && dy <= maxDist) {
                // 通常移動は最大距離以内なら移動可能
                outCost = (std::max)(dx, dy);
                return true;
            }
        }

        return false;
    } 
    void BattleMaster::Update() {
        auto& input = InputManager::GetInstance();
        input.Update();

        if (m_player) m_player->Update();
        if (m_enemy)  m_enemy->Update();

        Vector2 mousePos = input.GetMousePos();
        m_hoverGrid = m_mapGrid.ScreenToGrid(mousePos);

        int sbX = 960;

        // 【1. プレイヤー：移動フェーズ】
   // --- 1. プレイヤー：移動フェーズ ---
        if (m_currentPhase == Phase::PlayerTurn && m_player && !m_player->IsMoving()) {
            if (input.IsMouseLeftTrg()) {
                IntVector2 pPos = m_player->GetGridPos();

                // 【A】 まだユニットを選択していない場合
                if (!m_isPlayerSelected) {
                    if (m_hoverGrid == pPos) {
                        m_isPlayerSelected = true; // 範囲を表示（ここで自分の足元も黄色く光る）
                    }
                }
                // 【B】 すでにユニットを選択中の場合（二度目のクリック）
                else {
                    // 1. 自分自身の場所をクリック -> 「待機（数値調整）」を実行
                    if (m_hoverGrid == pPos) {
                        m_player->AddNumber(-1); // 待機コストを支払う
                        AddLog("【待機】 その場で数値を調整した (コスト: -1)");

                        m_isPlayerSelected = false;
                        m_currentPhase = Phase::ActionSelect; // そのまま行動選択（アイテム取得など）へ
                    }
                    // 2. 移動先をクリックした場合
                    else if (m_mapGrid.IsWithinBounds(m_hoverGrid.x, m_hoverGrid.y)) {
                        int cost = 0;
                        if (CanMove(m_player->GetNumber(), m_player->GetOp(), pPos, m_hoverGrid, cost)) {
                            // 移動処理を実行
                            m_player->AddNumber(-cost);
                            AddLog("【移動】 プレイヤー (コスト: -" + std::to_string(cost) + ")");

                            // 自動経路生成（直線移動）
                            std::queue<Vector2> autoPath;
                            int stepX = (m_hoverGrid.x > pPos.x) ? 1 : (m_hoverGrid.x < pPos.x) ? -1 : 0;
                            int stepY = (m_hoverGrid.y > pPos.y) ? 1 : (m_hoverGrid.y < pPos.y) ? -1 : 0;
                            IntVector2 curr = pPos;
                            while (curr != m_hoverGrid) {
                                curr.x += stepX; curr.y += stepY;
                                autoPath.push(m_mapGrid.GetCellCenter(curr.x, curr.y));
                            }

                            m_player->StartMove(m_hoverGrid, autoPath);
                            m_isPlayerSelected = false;
                            m_currentPhase = Phase::ActionSelect; // 移動完了後にアクション選択へ
                        }
                        else {
                            // 移動できない場所をクリックしたら選択解除
                            m_isPlayerSelected = false;
                        }
                    }
                }
            }
        }

        // 【1.5 プレイヤー：行動選択フェーズ（移動後）】
        if (m_currentPhase == Phase::ActionSelect && m_player && !m_player->IsMoving()) {
            // アイテムの取得処理（移動終了時に1回だけ判定）
            IntVector2 pP = m_player->GetGridPos();
            char pickedItem = m_mapGrid.PickUpItem(pP.x, pP.y);
            if (pickedItem != '\0') {
                m_player->SetOp(pickedItem);
                AddLog("【取得】 演算子 [" + std::string(1, pickedItem) + "] を装備した！");
            }

            IntVector2 eP = m_enemy ? m_enemy->GetGridPos() : IntVector2{ -1, -1 };
            bool canAttack = m_enemy && (std::abs(pP.x - eP.x) + std::abs(pP.y - eP.y) == 1);

            if (input.IsMouseLeftTrg()) {
                if (canAttack) {
                    if (CheckButtonClick(sbX + 100, 950, 450, 90, mousePos)) {
                        ExecuteBattle(*m_player, *m_enemy);
                        m_currentPhase = Phase::EnemyTurn;
                        m_enemyAIStarted = false;
                    }
                    else if (CheckButtonClick(sbX + 570, 950, 300, 90, mousePos)) {
                        AddLog("【待機】 ターンを終了した");
                        m_currentPhase = Phase::EnemyTurn;
                        m_enemyAIStarted = false;
                    }
                }
                else {
                    if (CheckButtonClick(sbX + 200, 950, 560, 90, mousePos)) {
                        AddLog("【待機】 ターンを終了した");
                        m_currentPhase = Phase::EnemyTurn;
                        m_enemyAIStarted = false;
                    }
                }
            }
        }

        // 【2. 敵のAIターンフェーズ】
        if (m_currentPhase == Phase::EnemyTurn) {
            if (m_player && !m_player->IsMoving()) {
                if (!m_enemyAIStarted) {
                    ExecuteEnemyAI();
                    m_enemyAIStarted = true;
                }
                else if (m_enemy && !m_enemy->IsMoving()) {
                    IntVector2 eP = m_enemy->GetGridPos();
                    char pickedItem = m_mapGrid.PickUpItem(eP.x, eP.y);
                    if (pickedItem != '\0') {
                        m_enemy->SetOp(pickedItem);
                        AddLog("【取得】 敵が演算子 [" + std::string(1, pickedItem) + "] を装備した");
                    }

                    IntVector2 pP = m_player->GetGridPos();
                    if (std::abs(pP.x - eP.x) + std::abs(pP.y - eP.y) == 1) {
                        ExecuteBattle(*m_enemy, *m_player);
                    }

                    // ターン終了時にマップの演算子をサイクルさせる
                    m_mapGrid.UpdateTurn();
                    m_currentPhase = Phase::PlayerTurn;
                }
            }
        }

        // --- 3. UIのクリック判定 ---
        if (input.IsMouseLeftTrg()) {
            if (m_player) {
                if (CheckButtonClick(sbX + 200, 140, 60, 40, mousePos)) m_player->AddNumber(1);
                if (CheckButtonClick(sbX + 270, 140, 60, 40, mousePos)) m_player->AddNumber(-1);
                if (CheckButtonClick(sbX + 340, 140, 60, 40, mousePos)) m_player->CycleOp();
                if (CheckButtonClick(sbX + 200, 230, 60, 40, mousePos)) m_player->AddStocks(1);
                if (CheckButtonClick(sbX + 270, 230, 60, 40, mousePos)) m_player->AddStocks(-1);
            }
            if (m_enemy) {
                if (CheckButtonClick(sbX + 680, 140, 60, 40, mousePos)) m_enemy->AddNumber(1);
                if (CheckButtonClick(sbX + 750, 140, 60, 40, mousePos)) m_enemy->AddNumber(-1);
                if (CheckButtonClick(sbX + 820, 140, 60, 40, mousePos)) m_enemy->CycleOp();
                if (CheckButtonClick(sbX + 680, 230, 60, 40, mousePos)) m_enemy->AddStocks(1);
                if (CheckButtonClick(sbX + 750, 230, 60, 40, mousePos)) m_enemy->AddStocks(-1);
            }
        }
    }

    void BattleMaster::ExecuteEnemyAI() {
        if (!m_enemy || m_enemy->GetStocks() <= 0 || !m_player) {
            m_mapGrid.UpdateTurn();
            m_currentPhase = Phase::PlayerTurn;
            return;
        }

        IntVector2 ePos = m_enemy->GetGridPos();
        IntVector2 pPos = m_player->GetGridPos();
        int eNum = m_enemy->GetNumber();
        char eOp = m_enemy->GetOp();

        IntVector2 bestTarget = ePos;
        int maxScore = -999999;
        int bestCost = 1; // 待機（調整）のデフォルトコスト

        // マップ全体を走査して最適な行動を探す
        for (int x = 0; x < m_mapGrid.GetWidth(); ++x) {
            for (int y = 0; y < m_mapGrid.GetHeight(); ++y) {
                IntVector2 target{ x, y };

                // プレイヤーのマスには重なれない
                if (target == pPos) continue;

                int cost = 0;
                bool canMove = false;

                // 1. プレイヤーと同じく、自分自身の場所は「待機（コスト1）」として判定する
                if (target == ePos) {
                    canMove = true;
                    cost = 1;
                }
                // 2. それ以外のマスは通常の移動判定
                else {
                    canMove = CanMove(eNum, eOp, ePos, target, cost);
                }

                // 移動（または待機）可能な場合のスコア計算
                if (canMove) {
                    int score = 0;
                    int distToPlayer = std::abs(pPos.x - target.x) + std::abs(pPos.y - target.y);

                    if (distToPlayer == 1) {
                        score += 1000; // プレイヤーに隣接できるなら最優先（次ターンに攻撃）
                    }
                    else {
                        score -= distToPlayer * 10; // 近づくほど高評価
                    }

                    // アイテムがあれば優先して取りに行く
                    if (target != ePos && m_mapGrid.GetItemAt(target.x, target.y) != '\0') {
                        score += 50;
                    }

                    // 無駄な遠回りをするくらいなら、待機して数値を調整する方を少し優先
                    if (target == ePos) {
                        score += 5;
                    }

                    if (score > maxScore) {
                        maxScore = score;
                        bestTarget = target;
                        bestCost = cost;
                    }
                }
            }
        }

        // --- 行動の実行 ---
        if (bestTarget != ePos) {
            AddLog("【移動】 敵ユニット (コスト: -" + std::to_string(bestCost) + ")");

            std::queue<Vector2> screenPath;
            int stepX = (bestTarget.x > ePos.x) ? 1 : (bestTarget.x < ePos.x) ? -1 : 0;
            int stepY = (bestTarget.y > ePos.y) ? 1 : (bestTarget.y < ePos.y) ? -1 : 0;
            IntVector2 curr = ePos;
            while (curr != bestTarget) {
                curr.x += stepX; curr.y += stepY;
                screenPath.push(m_mapGrid.GetCellCenter(curr.x, curr.y));
            }
            m_enemy->AddNumber(-bestCost);
            m_enemy->StartMove(bestTarget, screenPath);
        }
        else {
            // ★変更点：敵も「ただパス」するのではなく、コストを払って数値を調整する
            m_enemy->AddNumber(-bestCost);
            AddLog("【待機】 敵ユニットがその場で数値を調整した (コスト: -1)");
        }
    }
    // バトル結果の数値とストック処理をまとめた関数
    void BattleMaster::ApplyBattleResult(UnitBase& unit, int resultNum) {
        std::string name = (&unit == m_player.get()) ? "プレイヤー" : "敵";

        // --- 超計算：循環ループ・ロジック ---
        // 1?9の範囲を時計のようにループさせる計算式
        // (resultNum - 1) % 9 を行うことで、0?8の範囲に変換し、負の数にも対応させる
        int cycleValue = (resultNum - 1) % 9;
        if (cycleValue < 0) cycleValue += 9; // C++の負の数剰余への対応

        int remain = cycleValue + 1; // 1?9の範囲に戻す

        // --- ストック（残機）の計算 ---
        if (resultNum <= 0) {
            // 0以下になった場合：マイナス方向に何周したかを計算
            int lostStockCount = (std::abs(resultNum) / 9) + 1;
            unit.AddStocks(-lostStockCount);
            AddLog("【ダメージ】 " + name + " の残機減少！ 数値が " + std::to_string(remain) + " になった。");
        }
        else if (resultNum > 9) {
            // 9を超えた場合：プラス方向に何周したかを計算
            int addStockCount = (resultNum - 1) / 9;
            unit.AddStocks(addStockCount);
            AddLog("【回復】 " + name + " の残機増加！ 数値が " + std::to_string(remain) + " になった。");
        }
        else {
            // 1?9に収まっている場合は数値の書き換えのみ
            unit.SetNumber(remain);
        }

        unit.SetNumber(remain);
    }
    void BattleMaster::ExecuteBattle(UnitBase& attacker, UnitBase& defender) {
        IntVector2 pP = attacker.GetGridPos();
        IntVector2 eP = defender.GetGridPos();

        // 隣接判定（念のため）
        if (std::abs(pP.x - eP.x) + std::abs(pP.y - eP.y) == 1) {
            char aOp = attacker.GetOp();
            int aNum = attacker.GetNumber();
            int dNum = defender.GetNumber();

            int result = 0;
            if (aOp == '+') result = aNum + dNum;
            else if (aOp == '-') result = aNum - dNum;
            else if (aOp == '*') result = aNum * dNum;
            else if (aOp == '/') result = (dNum != 0) ? aNum / dNum : 0;

            std::string aName = (&attacker == m_player.get()) ? "プレイヤー" : "敵";
            AddLog(">> " + aName + " の計算！ (" + std::to_string(aNum) + " " + std::string(1, aOp) + " " + std::to_string(dNum) + " = " + std::to_string(result) + ")");

            // ★修正ポイント：結果を「攻撃側（自分）」のみに適用し、相手は変更しない
            ApplyBattleResult(attacker, result);

            // 攻撃後、演算子を消費してデフォルトの '+' に戻す
            attacker.SetOp('+');
        }
    }
    void BattleMaster::DrawEnemyDangerArea() {
        // 敵が存在しない、または移動中の場合は描画しない
        if (!m_enemy || m_enemy->GetStocks() <= 0 || m_enemy->IsMoving()) return;

        IntVector2 ePos = m_enemy->GetGridPos();
        int eNum = m_enemy->GetNumber();
        char eOp = m_enemy->GetOp();

        // 敵の危険エリアは少し薄め（アルファ値80?100程度）にする
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 80);

        for (int x = 0; x < m_mapGrid.GetWidth(); ++x) {
            for (int y = 0; y < m_mapGrid.GetHeight(); ++y) {
                IntVector2 target{ x, y };
                Vector2 center = m_mapGrid.GetCellCenter(x, y);

                // --- 1. 敵のいる場所（足元・核） ---
                if (target == ePos) {
                    // 敵の核は目立つ「明るい赤」
                    DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, GetColor(255, 50, 50), TRUE);
                    continue;
                }

                // --- 2. 移動範囲の判定と色分け ---
                int dummyCost = 0;
                // 演算子なしの基本範囲
                bool canMoveBase = CanMove(eNum, '\0', ePos, target, dummyCost);
                // 演算子込みの最終範囲
                bool canMoveCombined = CanMove(eNum, eOp, ePos, target, dummyCost);

                if (canMoveCombined) {
                    if (canMoveBase) {
                        // 基本範囲：危険を示す「暗めの赤」
                        DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, GetColor(200, 50, 50), TRUE);
                    }
                    else {
                        // 演算子による拡張：警戒を促す「マゼンタ（紫がかったピンク）」
                        DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, GetColor(255, 100, 200), TRUE);
                    }
                }
            }
        }

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
    void BattleMaster::DrawMovableArea() {
        // プレイヤーが選択されていない、または移動中は描画しない
        if (!m_isPlayerSelected || !m_player || m_player->IsMoving()) return;

        IntVector2 pPos = m_player->GetGridPos();
        int pNum = m_player->GetNumber();
        char pOp = m_player->GetOp();

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);

        for (int x = 0; x < m_mapGrid.GetWidth(); ++x) {
            for (int y = 0; y < m_mapGrid.GetHeight(); ++y) {
                IntVector2 target{ x, y };
                Vector2 center = m_mapGrid.GetCellCenter(x, y);

                // --- 1. 自分のいる場所（足元）の描画 ---
                if (target == pPos) {
                    // ここで黄色（255, 255, 0）を塗ることで記号を繋げる
                    DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, GetColor(255, 255, 0), TRUE);
                    continue; // 自分の場所の判定はここで終わり
                }

                // --- 2. 移動範囲の判定と色分け ---
                int dummyCost = 0;
                // 演算子なしの基本範囲
                bool canMoveBase = CanMove(pNum, '\0', pPos, target, dummyCost);
                // 演算子込みの拡張範囲
                bool canMoveCombined = CanMove(pNum, pOp, pPos, target, dummyCost);

                if (canMoveCombined) {
                    if (canMoveBase) {
                        // 基本範囲：青
                        DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, GetColor(100, 200, 255), TRUE);
                    }
                    else {
                        // 演算子による拡張：オレンジ
                        DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, GetColor(255, 180, 50), TRUE);
                    }
                }
            }
        }

        // --- 3. マウスホバー時の強調（ここも足元に対応） ---
        if (m_mapGrid.IsWithinBounds(m_hoverGrid.x, m_hoverGrid.y)) {
            int hoverCost = 0;
            bool isHoverSelf = (m_hoverGrid == pPos);

            // 自分自身、または移動可能なマスにマウスがある場合
            if (isHoverSelf || CanMove(pNum, pOp, pPos, m_hoverGrid, hoverCost)) {
                Vector2 hc = m_mapGrid.GetCellCenter(m_hoverGrid.x, m_hoverGrid.y);

                // 選択中を白枠で強調
                DrawBox((int)hc.x - 35, (int)hc.y - 35, (int)hc.x + 35, (int)hc.y + 35, GetColor(255, 255, 255), FALSE);

                // コスト表示
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                int displayCost = isHoverSelf ? 1 : hoverCost;
                DrawFormatString((int)hc.x - 10, (int)hc.y - 20, GetColor(255, 50, 50), "-%d", displayCost);
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
            }
        }

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    } 
    void BattleMaster::Draw() {
        SetFontSize(24);
        m_mapGrid.Draw();
        DrawEnemyDangerArea();
        DrawMovableArea();
        if (m_player) m_player->Draw();
        if (m_enemy)  m_enemy->Draw();

        int sbX = 960;

        // UI背景の描画
        DrawBox(sbX, 0, 1920, 1080, GetColor(25, 25, 30), TRUE);
        DrawLine(sbX, 0, sbX, 1080, GetColor(80, 120, 180), 4);

        // --- フェーズヘッダー ---
        if (m_currentPhase == Phase::PlayerTurn || m_currentPhase == Phase::ActionSelect) {
            DrawBox(sbX, 0, 1920, 80, GetColor(0, 70, 180), TRUE);
            SetFontSize(48);
            if (m_currentPhase == Phase::PlayerTurn) {
                DrawFormatString(sbX + 40, 15, GetColor(200, 230, 255), ">>> プレイヤーのターン (移動) <<<");
            }
            else {
                DrawFormatString(sbX + 40, 15, GetColor(255, 255, 100), ">>> プレイヤーのターン (行動選択) <<<");
            }
        }
        else {
            DrawBox(sbX, 0, 1920, 80, GetColor(180, 40, 40), TRUE);
            SetFontSize(48); DrawFormatString(sbX + 40, 15, GetColor(255, 200, 200), ">>> 敵のターン <<<");
        }

        // --- 1. TURN INFO (ターン情報) ---
        int ty = 100;
        SetFontSize(28);
        DrawFormatString(sbX + 40, ty, GetColor(200, 200, 200), "【 進行状況 】 総経過ターン : %d", m_mapGrid.GetTotalTurns());
        DrawFormatString(sbX + 40, ty + 40, GetColor(255, 200, 50), "アイテム再生成まで : あと %d ターン", m_mapGrid.GetTurnsUntilNextSpawn());

        // --- 2. BATTLE SIM (戦闘シミュレーター) ---
        int sy = 190;
        SetFontSize(32);
        DrawFormatString(sbX + 40, sy, GetColor(255, 255, 0), "■ 戦闘シミュレーター");
        DrawBox(sbX + 40, sy + 40, sbX + 920, sy + 210, GetColor(40, 40, 60), TRUE);

        IntVector2 pP = m_player ? m_player->GetGridPos() : IntVector2{ -1,-1 };
        IntVector2 eP = m_enemy ? m_enemy->GetGridPos() : IntVector2{ -1,-1 };
        bool canAttack = (std::abs(pP.x - eP.x) + std::abs(pP.y - eP.y) == 1);

        if (canAttack) {
            char pOp = m_player->GetOp();
            int pNum = m_player->GetNumber();
            int eNum = m_enemy->GetNumber();

            int res = 0;
            if (pOp == '+') res = pNum + eNum;
            else if (pOp == '-') res = pNum - eNum;
            else if (pOp == '*') res = pNum * eNum;
            else if (pOp == '/') res = (eNum != 0) ? pNum / eNum : 0;

            SetFontSize(24);
            DrawFormatString(sbX + 60, sy + 50, GetColor(100, 200, 255), "自分 [%c]", pOp);
            DrawFormatString(sbX + 500, sy + 50, GetColor(255, 100, 100), "相手");

            SetFontSize(80);
            DrawFormatString(sbX + 270, sy + 70, GetColor(255, 255, 255), "%d %c %d = %d", pNum, pOp, eNum, res);

            // 結果の日本語プレビュー
            SetFontSize(32);
            if (res <= 0) {
                DrawFormatString(sbX + 330, sy + 160, GetColor(255, 50, 50), "《 覚醒 (残機減少) 》");
            }
            else if (res >= 10) {
                DrawFormatString(sbX + 330, sy + 160, GetColor(50, 255, 50), "《 回復 (残機増加) 》");
            }
            else {
                DrawFormatString(sbX + 300, sy + 160, GetColor(200, 200, 200), "《 数値書き換え : %d 》", res);
            }
        }
        else {
            SetFontSize(32);
            DrawFormatString(sbX + 250, sy + 100, GetColor(120, 120, 120), "（敵に隣接していません）");
        }

        // --- 3. UNIT INFO (ステータス比較) ---
        int uy = 430;
        DrawLine(sbX + 480, uy, sbX + 480, uy + 170, GetColor(60, 60, 80), 2);

        // 移動タイプを名前に変換するラムダ式
        auto getMoveTypeName = [](int num) {
            if (num >= 1 && num <= 3) return "十字 (縦横)";
            if (num >= 4 && num <= 6) return "クロス (斜め)";
            if (num >= 7 && num <= 9) return "スター (全方位)";
            return "不明";
            };

        // プレイヤー側
        SetFontSize(28);
        DrawFormatString(sbX + 40, uy, GetColor(100, 200, 255), "■ プレイヤー");
        if (m_player) {
            SetFontSize(24);
            DrawFormatString(sbX + 60, uy + 45, GetColor(255, 255, 255), "現在数値 : %d", m_player->GetNumber());
            DrawFormatString(sbX + 60, uy + 80, GetColor(255, 255, 255), "演算子　 : [ %c ]", m_player->GetOp());
            DrawFormatString(sbX + 60, uy + 115, GetColor(255, 255, 0), "残機数　 : %d / %d", m_player->GetStocks(), m_player->GetMaxStocks());
            DrawFormatString(sbX + 60, uy + 150, GetColor(150, 255, 150), "移動型　 : %s", getMoveTypeName(m_player->GetNumber()));
        }

        // 敵側
        int ex = sbX + 520;
        SetFontSize(28);
        DrawFormatString(ex, uy, GetColor(255, 100, 100), "■ ターゲット (敵)");
        if (m_enemy) {
            SetFontSize(24);
            DrawFormatString(ex + 20, uy + 45, GetColor(255, 255, 255), "現在数値 : %d", m_enemy->GetNumber());
            DrawFormatString(ex + 20, uy + 80, GetColor(255, 255, 255), "演算子　 : [ %c ]", m_enemy->GetOp());
            DrawFormatString(ex + 20, uy + 115, GetColor(255, 100, 100), "残機数　 : %d / %d", m_enemy->GetStocks(), m_enemy->GetMaxStocks());
            DrawFormatString(ex + 20, uy + 150, GetColor(150, 255, 150), "移動型　 : %s", getMoveTypeName(m_enemy->GetNumber()));
        }

        // --- 4. ACTION LOG (戦況ログ) ---
        int ly = 630;
        SetFontSize(28);
        DrawFormatString(sbX + 40, ly, GetColor(200, 200, 200), "■ アクションログ");
        DrawBox(sbX + 40, ly + 40, sbX + 920, ly + 220, GetColor(15, 15, 20), TRUE);

        SetFontSize(22);
        for (size_t i = 0; i < m_actionLog.size(); ++i) {
            DrawFormatString(sbX + 55, ly + 50 + (int)i * 28, GetColor(180, 255, 180), "%s", m_actionLog[i].c_str());
        }

        // --- 5. REFERENCE (早見表) ---
        int ry = 880;
        SetFontSize(20);
        DrawFormatString(sbX + 40, ry, GetColor(150, 150, 150), "【移動の法則】 1~3: 十字移動 | 4~6: 斜め移動 | 7~9: 全方位移動");
        DrawFormatString(sbX + 40, ry + 25, GetColor(150, 150, 150), "【演算子効果】 +: 十字拡張 | -: 横拡張 | x: 斜め拡張 | ÷: ジャンプ＆横");

        // --- 行動選択ボタン (Update関数の判定座標と完全一致させています) ---
        if (m_currentPhase == Phase::ActionSelect) {
            if (canAttack) {
                DrawBox(sbX + 100, 950, sbX + 550, 1040, GetColor(200, 50, 50), TRUE);
                SetFontSize(36); DrawFormatString(sbX + 220, 975, GetColor(255, 255, 255), "攻 撃 す る");

                DrawBox(sbX + 570, 950, sbX + 870, 1040, GetColor(80, 80, 100), TRUE);
                SetFontSize(36); DrawFormatString(sbX + 610, 975, GetColor(255, 255, 255), "待機 (終了)");
            }
            else {
                DrawBox(sbX + 200, 950, sbX + 760, 1040, GetColor(80, 80, 100), TRUE);
                SetFontSize(42); DrawFormatString(sbX + 330, 975, GetColor(255, 255, 255), "待機 (ターン終了)");
            }
        }
    }
    bool BattleMaster::CheckButtonClick(int x, int y, int w, int h, Vector2 m) {
        return (m.x >= (float)x && m.x <= (float)(x + w) && m.y >= (float)y && m.y <= (float)(y + h));
    }

    bool BattleMaster::IsGameOver() const {
        if (m_player && m_player->GetStocks() <= 0) return true;
        if (m_enemy && m_enemy->GetStocks() <= 0) return true;
        return false;
    }

}
        // namespace App