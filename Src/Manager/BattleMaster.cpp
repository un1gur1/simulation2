#define NOMINMAX
#include "BattleMaster.h"
#include <DxLib.h>
#include <cmath>
#include <algorithm>
#include "../Input/InputManager.h"

namespace App {

    BattleMaster::BattleMaster()
        : m_currentPhase(Phase::PlayerTurn)
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
        m_player = std::make_unique<Player>(IntVector2{ 1, 1 }, m_mapGrid.GetCellCenter(1, 1), 5, 3, 5);
        m_enemy = std::make_unique<Enemy>(IntVector2{ 5, 5 }, m_mapGrid.GetCellCenter(5, 5), 7, 1, 5);
        m_currentPhase = Phase::PlayerTurn;
        m_isPlayerSelected = false;

        m_actionLog.clear();
        AddLog(">>> バトル開始！ 敵を殲滅せよ！");

        //// フォントを滑らかにして見やすくする
        //ChangeFontType(DX_FONTTYPE_ANTIALIASING_EDGE);
        //ChangeFont("メイリオ", DX_CHARSET_DEFAULT);
    }

    void BattleMaster::Update() {
        auto& input = InputManager::GetInstance();
        input.Update();

        if (m_player) m_player->Update();
        if (m_enemy)  m_enemy->Update();

        Vector2 mousePos = input.GetMousePos();
        m_hoverGrid = m_mapGrid.ScreenToGrid(mousePos);

        int sbX = 960; // 画面半分で固定

        // --- 1. プレイヤー：移動フェーズ ---
        if (m_currentPhase == Phase::PlayerTurn && m_player && !m_player->IsMoving()) {
            if (input.IsMouseLeftTrg()) {
                IntVector2 pPos = m_player->GetGridPos();
                if (m_hoverGrid == pPos) {
                    m_isPlayerSelected = !m_isPlayerSelected;
                }
                else if (m_isPlayerSelected && m_mapGrid.IsWithinBounds(m_hoverGrid.x, m_hoverGrid.y)) {
                    char op = m_player->GetOp();
                    int dx = std::abs(m_hoverGrid.x - pPos.x);
                    int dy = std::abs(m_hoverGrid.y - pPos.y);
                    bool canMove = false;

                    if (op == '+') canMove = (dx == 0 || dy == 0);
                    else if (op == '-') canMove = (dy == 0);
                    else if (op == '*') canMove = (dx == dy);
                    else if (op == '/') canMove = (dx <= 1 && dy <= 1);

                    if (canMove && (dx > 0 || dy > 0)) {
                        int cost = (op == '/') ? 1 : (std::max)(dx, dy);
                        m_player->AddNumber(-cost);

                        AddLog("【移動】 プレイヤー (コスト: -" + std::to_string(cost) + ")");

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

                        // 移動完了後、「行動選択」フェーズへ
                        m_currentPhase = Phase::ActionSelect;
                    }
                    else {
                        m_isPlayerSelected = false;
                    }
                }
            }
        }

        // --- 1.5 プレイヤー：行動選択フェーズ（移動後） ---
        if (m_currentPhase == Phase::ActionSelect && m_player && !m_player->IsMoving()) {
            IntVector2 pP = m_player->GetGridPos();
            IntVector2 eP = m_enemy ? m_enemy->GetGridPos() : IntVector2{ -1, -1 };
            bool canAttack = m_enemy && (std::abs(pP.x - eP.x) + std::abs(pP.y - eP.y) == 1);

            if (input.IsMouseLeftTrg()) {
                if (canAttack) {
                    // 攻撃実行 (左側のボタン)
                    if (CheckButtonClick(sbX + 100, 950, 450, 90, mousePos)) {
                        ExecuteBattle(*m_player, *m_enemy);
                        m_currentPhase = Phase::EnemyTurn;
                        m_enemyAIStarted = false;
                    }
                    // 待機 (右側のボタン)
                    else if (CheckButtonClick(sbX + 570, 950, 300, 90, mousePos)) {
                        AddLog("【待機】 ターンを終了した");
                        m_currentPhase = Phase::EnemyTurn;
                        m_enemyAIStarted = false;
                    }
                }
                else {
                    // 敵がいない場合の巨大待機ボタン
                    if (CheckButtonClick(sbX + 200, 950, 560, 90, mousePos)) {
                        AddLog("【待機】 ターンを終了した");
                        m_currentPhase = Phase::EnemyTurn;
                        m_enemyAIStarted = false;
                    }
                }
            }
        }

        // --- 2. 敵のAIターンフェーズ ---
        if (m_currentPhase == Phase::EnemyTurn) {
            if (m_player && !m_player->IsMoving()) {
                if (!m_enemyAIStarted) {
                    ExecuteEnemyAI();
                    m_enemyAIStarted = true;
                }
                else if (m_enemy && !m_enemy->IsMoving()) {
                    IntVector2 pP = m_player->GetGridPos();
                    IntVector2 eP = m_enemy->GetGridPos();
                    if (std::abs(pP.x - eP.x) + std::abs(pP.y - eP.y) == 1) {
                        ExecuteBattle(*m_enemy, *m_player);
                    }
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
            if (m_player && CheckButtonClick(sbX + 40, 370, 880, 60, mousePos)) {
                IntVector2 pP = m_player->GetGridPos();
                char cur = m_mapGrid.GetSymbol(pP.x, pP.y);
                char next = (cur == '+') ? '-' : (cur == '-') ? '*' : (cur == '*') ? '/' : '+';
                m_mapGrid.SetSymbol(pP.x, pP.y, next);
                AddLog("【環境】 世界の記号が [" + std::string(1, next) + "] に変化した！");
            }
        }
    }

    void BattleMaster::ExecuteEnemyAI() {
        if (!m_enemy || m_enemy->GetStocks() <= 0 || !m_player) {
            m_currentPhase = Phase::PlayerTurn; return;
        }
        IntVector2 ePos = m_enemy->GetGridPos();
        IntVector2 pPos = m_player->GetGridPos();
        char currentOp = m_enemy->GetOp();
        IntVector2 bestTarget = ePos;
        int maxScore = -999999;

        for (int x = 0; x < m_mapGrid.GetWidth(); ++x) {
            for (int y = 0; y < m_mapGrid.GetHeight(); ++y) {
                IntVector2 target{ x, y };
                if (target == pPos || target == ePos) continue;
                int dx = std::abs(target.x - ePos.x);
                int dy = std::abs(target.y - ePos.y);
                bool canMove = false;
                if (currentOp == '+') canMove = (dx == 0 || dy == 0);
                else if (currentOp == '-') canMove = (dy == 0);
                else if (currentOp == '*') canMove = (dx == dy);
                else if (currentOp == '/') canMove = (dx <= 1 && dy <= 1);

                if (canMove) {
                    int score = 0;
                    int distToPlayer = std::abs(pPos.x - target.x) + std::abs(pPos.y - target.y);
                    int cost = (currentOp == '/') ? 1 : (std::max)(dx, dy);
                    int predictedNum = m_enemy->GetNumber() - cost;
                    int tempStocks = m_enemy->GetStocks();
                    while (predictedNum <= 0 && tempStocks > 0) { tempStocks--; predictedNum += 9; }
                    int digit = predictedNum % 10; if (digit == 0) digit = 10;
                    char predictedOp = (digit <= 2) ? '-' : (digit <= 5) ? '+' : (digit <= 8) ? '*' : '/';

                    if (distToPlayer == 1) {
                        char wOp = m_mapGrid.GetSymbol(target.x, target.y);
                        char pOp = m_player->GetOp();
                        char resOp = wOp;
                        if (predictedOp == '-') { if (resOp == '+') resOp = '-'; else if (resOp == '-') resOp = '+'; }
                        if (pOp == '-') { if (resOp == '+') resOp = '-'; else if (resOp == '-') resOp = '+'; }
                        int pNum = m_player->GetNumber();
                        int result = (resOp == '+') ? predictedNum + pNum : (resOp == '-') ? predictedNum - pNum : (resOp == '*') ? predictedNum * pNum : (pNum != 0 ? predictedNum / pNum : 0);

                        if (result <= 0) score += 10000;
                        else if (result >= 10) score -= 5000;
                        else score += (100 - result * 10);
                    }
                    else {
                        score -= distToPlayer * 10;
                    }
                    if (score > maxScore) { maxScore = score; bestTarget = target; }
                }
            }
        }

        if (bestTarget != ePos) {
            int dx = std::abs(bestTarget.x - ePos.x);
            int dy = std::abs(bestTarget.y - ePos.y);
            int cost = (currentOp == '/') ? 1 : (std::max)(dx, dy);

            AddLog("【移動】 敵ユニット (コスト: -" + std::to_string(cost) + ")");

            std::queue<Vector2> screenPath;
            int stepX = (bestTarget.x > ePos.x) ? 1 : (bestTarget.x < ePos.x) ? -1 : 0;
            int stepY = (bestTarget.y > ePos.y) ? 1 : (bestTarget.y < ePos.y) ? -1 : 0;
            IntVector2 curr = ePos;
            while (curr != bestTarget) {
                curr.x += stepX; curr.y += stepY;
                screenPath.push(m_mapGrid.GetCellCenter(curr.x, curr.y));
            }
            m_enemy->AddNumber(-cost);
            m_enemy->StartMove(bestTarget, screenPath);
        }
        else {
            AddLog("【待機】 敵は行動をパスした");
            m_currentPhase = Phase::PlayerTurn;
        }
    }

    void BattleMaster::ExecuteBattle(UnitBase& attacker, UnitBase& defender) {
        IntVector2 pP = attacker.GetGridPos();
        IntVector2 eP = defender.GetGridPos();
        if (std::abs(pP.x - eP.x) + std::abs(pP.y - eP.y) == 1) {
            char pOp = attacker.GetOp();
            char wOp = m_mapGrid.GetSymbol(pP.x, pP.y);
            char eOp = defender.GetOp();
            char resOp = wOp;
            if (pOp == '-') { if (resOp == '+') resOp = '-'; else if (resOp == '-') resOp = '+'; }
            if (eOp == '-') { if (resOp == '+') resOp = '-'; else if (resOp == '-') resOp = '+'; }

            int aNum = attacker.GetNumber();
            int dNum = defender.GetNumber();
            int result = (resOp == '+') ? aNum + dNum : (resOp == '-') ? aNum - dNum : (resOp == '*') ? aNum * dNum : (dNum != 0 ? aNum / dNum : 0);

            std::string aName = (&attacker == m_player.get()) ? "プレイヤー" : "敵";
            std::string dName = (&defender == m_player.get()) ? "プレイヤー" : "敵";

            AddLog(">> " + aName + " の攻撃！ (" + std::to_string(aNum) + " " + std::string(1, resOp) + " " + std::to_string(dNum) + " = " + std::to_string(result) + ")");

            if (result <= 0) { AddLog("【大ダメージ】 " + dName + " に 1 ダメージ！！"); }
            else if (result >= 10) { AddLog("【大失敗】 " + dName + " の命を 1 回復させてしまった！"); }
            else { AddLog("【変化】 " + dName + " の数値を " + std::to_string(result) + " に上書きした。"); }

            defender.SetNumber(result);
        }
    }

    void BattleMaster::DrawEnemyDangerArea() {
        if (!m_enemy || m_enemy->GetStocks() <= 0) return;
        IntVector2 ePos = m_enemy->GetGridPos();
        char op = m_enemy->GetOp();

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 80);
        for (int x = 0; x < m_mapGrid.GetWidth(); ++x) {
            for (int y = 0; y < m_mapGrid.GetHeight(); ++y) {
                if (x == ePos.x && y == ePos.y) continue;
                int dx = std::abs(x - ePos.x); int dy = std::abs(y - ePos.y);
                bool canMove = false;
                if (op == '+') canMove = (dx == 0 || dy == 0);
                else if (op == '-') canMove = (dy == 0);
                else if (op == '*') canMove = (dx == dy);
                else if (op == '/') canMove = (dx <= 1 && dy <= 1);

                if (canMove) {
                    Vector2 center = m_mapGrid.GetCellCenter(x, y);
                    DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, GetColor(255, 100, 100), TRUE);
                }
            }
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    void BattleMaster::DrawMovableArea() {
        if (!m_isPlayerSelected || !m_player || m_player->IsMoving()) return;
        IntVector2 pPos = m_player->GetGridPos();
        char op = m_player->GetOp();
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
        for (int x = 0; x < m_mapGrid.GetWidth(); ++x) {
            for (int y = 0; y < m_mapGrid.GetHeight(); ++y) {
                if (x == pPos.x && y == pPos.y) continue;
                int dx = std::abs(x - pPos.x); int dy = std::abs(y - pPos.y);
                bool canMove = (op == '+') ? (dx == 0 || dy == 0) : (op == '-') ? (dy == 0) : (op == '*') ? (dx == dy) : (dx <= 1 && dy <= 1);
                if (canMove) {
                    Vector2 center = m_mapGrid.GetCellCenter(x, y);
                    DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, GetColor(100, 200, 255), TRUE);
                }
            }
        }
        int hx = std::abs(m_hoverGrid.x - pPos.x); int hy = std::abs(m_hoverGrid.y - pPos.y);
        bool hoverValid = (op == '+') ? (hx == 0 || hy == 0) : (op == '-') ? (hy == 0) : (op == '*') ? (hx == hy) : (hx <= 1 && hy <= 1);
        if (hoverValid && m_hoverGrid != pPos && m_mapGrid.IsWithinBounds(m_hoverGrid.x, m_hoverGrid.y)) {
            Vector2 hc = m_mapGrid.GetCellCenter(m_hoverGrid.x, m_hoverGrid.y);
            DrawBox((int)hc.x - 35, (int)hc.y - 35, (int)hc.x + 35, (int)hc.y + 35, GetColor(255, 255, 100), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawFormatString((int)hc.x, (int)hc.y - 20, GetColor(255, 50, 50), "-%d", (op == '/') ? 1 : (std::max)(hx, hy));
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    void BattleMaster::Draw() {
        // ★描画前にフォントサイズをリセットし、マップの十字が巨大化するバグを防ぐ
        SetFontSize(24);

        // 1. マップとユニットの描画
        m_mapGrid.Draw();
        DrawEnemyDangerArea();
        DrawMovableArea();
        if (m_player) m_player->Draw();
        if (m_enemy)  m_enemy->Draw();

        int sbX = 960; // 画面半分で固定

        // 2. UI背景の描画
        DrawBox(sbX, 0, 1920, 1080, GetColor(25, 25, 30), TRUE);
        DrawLine(sbX, 0, sbX, 1080, GetColor(80, 120, 180), 4);

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

        int py = 100;
        DrawLine(sbX + 480, 100, sbX + 480, 350, GetColor(60, 60, 80), 2);

        // プレイヤー情報
        SetFontSize(32); DrawFormatString(sbX + 40, py, GetColor(100, 200, 255), "■ プレイヤー (自分)");
        if (m_player) {
            SetFontSize(24);
            DrawFormatString(sbX + 40, py + 50, GetColor(255, 255, 255), "数値: %2d", m_player->GetNumber());
            DrawFormatString(sbX + 40, py + 90, GetColor(255, 255, 255), "記号: [ %c ]", m_player->GetOp());
            DrawFormatString(sbX + 40, py + 130, GetColor(255, 255, 0), "命 : %d / %d", m_player->GetStocks(), m_player->GetMaxStocks());
            DrawBox(sbX + 200, py + 40, sbX + 260, py + 80, GetColor(50, 80, 120), TRUE); DrawFormatString(sbX + 210, py + 45, GetColor(255, 255, 255), "数+");
            DrawBox(sbX + 270, py + 40, sbX + 330, py + 80, GetColor(50, 80, 120), TRUE); DrawFormatString(sbX + 280, py + 45, GetColor(255, 255, 255), "数-");
            DrawBox(sbX + 340, py + 40, sbX + 400, py + 80, GetColor(50, 120, 80), TRUE); DrawFormatString(sbX + 345, py + 45, GetColor(255, 255, 255), "記号");
            DrawBox(sbX + 200, py + 130, sbX + 260, py + 170, GetColor(120, 60, 60), TRUE); DrawFormatString(sbX + 210, py + 135, GetColor(255, 255, 255), "命+");
            DrawBox(sbX + 270, py + 130, sbX + 330, py + 170, GetColor(80, 50, 50), TRUE); DrawFormatString(sbX + 280, py + 135, GetColor(255, 255, 255), "命-");
        }

        // 敵情報
        int ex = sbX + 520;
        SetFontSize(32); DrawFormatString(ex, py, GetColor(255, 100, 100), "■ ターゲット (敵)");
        if (m_enemy) {
            SetFontSize(24);
            DrawFormatString(ex, py + 50, GetColor(255, 255, 255), "数値: %2d", m_enemy->GetNumber());
            DrawFormatString(ex, py + 90, GetColor(255, 255, 255), "記号: [ %c ]", m_enemy->GetOp());
            DrawFormatString(ex, py + 130, GetColor(255, 100, 100), "命 : %d", m_enemy->GetStocks());
            DrawBox(ex + 160, py + 40, ex + 220, py + 80, GetColor(50, 80, 120), TRUE); DrawFormatString(ex + 170, py + 45, GetColor(255, 255, 255), "数+");
            DrawBox(ex + 230, py + 40, ex + 290, py + 80, GetColor(50, 80, 120), TRUE); DrawFormatString(ex + 240, py + 45, GetColor(255, 255, 255), "数-");
            DrawBox(ex + 300, py + 40, ex + 360, py + 80, GetColor(50, 120, 80), TRUE); DrawFormatString(ex + 305, py + 45, GetColor(255, 255, 255), "記号");
            DrawBox(ex + 160, py + 130, ex + 220, py + 170, GetColor(120, 60, 60), TRUE); DrawFormatString(ex + 170, py + 135, GetColor(255, 255, 255), "命+");
            DrawBox(ex + 230, py + 130, ex + 290, py + 170, GetColor(80, 50, 50), TRUE); DrawFormatString(ex + 240, py + 135, GetColor(255, 255, 255), "命-");
        }

        // 世界の記号変更
        int wy = 370;
        DrawBox(sbX + 40, wy, sbX + 920, wy + 60, GetColor(40, 140, 80), TRUE);
        SetFontSize(32);
        char wOpNow = m_mapGrid.GetSymbol(m_player ? m_player->GetGridPos().x : 1, m_player ? m_player->GetGridPos().y : 1);
        DrawFormatString(sbX + 150, wy + 15, GetColor(255, 255, 255), "現在の世界記号: [ %c ]  ＞＞ 変更する", wOpNow);

        // ログ
        int ly = 460;
        SetFontSize(32); DrawFormatString(sbX + 40, ly, GetColor(200, 200, 200), "■ 戦況ログ");
        DrawBox(sbX + 40, ly + 40, sbX + 920, ly + 250, GetColor(10, 10, 15), TRUE);
        DrawBox(sbX + 40, ly + 40, sbX + 920, ly + 250, GetColor(100, 100, 100), FALSE);
        SetFontSize(24);
        for (size_t i = 0; i < m_actionLog.size(); ++i) {
            DrawFormatString(sbX + 55, ly + 55 + (int)i * 30, GetColor(180, 255, 180), "%s", m_actionLog[i].c_str());
        }

        // ==================================================
        // ★ 計算ダッシュボード（完全復活！）
        // ==================================================
        int cy = 730;
        SetFontSize(32); DrawFormatString(sbX + 40, cy, GetColor(255, 255, 0), "■ 攻撃シミュレーション");

        IntVector2 p = m_player ? m_player->GetGridPos() : IntVector2{ -1,-1 };
        IntVector2 e = m_enemy ? m_enemy->GetGridPos() : IntVector2{ -1,-1 };
        bool canAttack = (std::abs(p.x - e.x) + std::abs(p.y - e.y) == 1);

        if (canAttack) {
            char pOp = m_player->GetOp(); char wOp = m_mapGrid.GetSymbol(p.x, p.y); char eOp = m_enemy->GetOp();
            int pNum = m_player->GetNumber(); int eNum = m_enemy->GetNumber();
            char resOp = wOp;
            bool pInvert = (pOp == '-'); bool eInvert = (eOp == '-');

            if (pInvert) { if (resOp == '+') resOp = '-'; else if (resOp == '-') resOp = '+'; }
            if (eInvert) { if (resOp == '+') resOp = '-'; else if (resOp == '-') resOp = '+'; }
            int res = (resOp == '+') ? pNum + eNum : (resOp == '-') ? pNum - eNum : (resOp == '*') ? pNum * eNum : (eNum != 0 ? pNum / eNum : 0);

            // 計算プロセスの詳細表示
            SetFontSize(24);
            DrawFormatString(sbX + 60, cy + 50, GetColor(100, 200, 255), "自分 [%c] %s", pOp, pInvert ? "-> 反転！" : "-> 通常");
            DrawFormatString(sbX + 320, cy + 50, GetColor(100, 255, 100), "世界 [%c]", wOp);
            DrawFormatString(sbX + 500, cy + 50, GetColor(255, 100, 100), "相手 [%c] %s", eOp, eInvert ? "-> 反転！" : "-> 通常");

            // デカい計算式
            DrawBox(sbX + 40, cy + 90, sbX + 920, cy + 190, GetColor(40, 40, 60), TRUE);
            SetFontSize(64);
            DrawFormatString(sbX + 300, cy + 110, GetColor(255, 255, 255), "%d %c %d = %d", pNum, resOp, eNum, res);

            // 行動選択（攻撃 or 待機）UI：行動選択フェーズの時だけボタン化する
            if (m_currentPhase == Phase::ActionSelect) {
                int btnColor = GetColor(100, 100, 130);
                std::string btnText = "攻撃 (数値書き換え)";
                if (res <= 0) { btnColor = GetColor(200, 50, 50); btnText = "攻撃 (ダメージ確定)"; }
                else if (res >= 10) { btnColor = GetColor(50, 150, 50); btnText = "攻撃 (敵が回復)"; }

                DrawBox(sbX + 100, 950, sbX + 550, 1040, btnColor, TRUE);
                SetFontSize(36); DrawFormatString(sbX + 120, 975, GetColor(255, 255, 255), "%s", btnText.c_str());
                DrawBox(sbX + 570, 950, sbX + 870, 1040, GetColor(80, 80, 100), TRUE);
                SetFontSize(36); DrawFormatString(sbX + 610, 975, GetColor(255, 255, 255), "待機 (ターン終了)");
            }
        }
        else {
            SetFontSize(28); DrawFormatString(sbX + 70, cy + 100, GetColor(120, 120, 120), "（敵に隣接していません）");

            // 敵がいない時は待機ボタンのみ
            if (m_currentPhase == Phase::ActionSelect) {
                DrawBox(sbX + 200, 950, sbX + 760, 1040, GetColor(80, 80, 100), TRUE);
                SetFontSize(42); DrawFormatString(sbX + 330, 975, GetColor(255, 255, 255), "待機 (ターン終了)");
            }
        }

        // 移動前のアナウンス
        if (m_currentPhase == Phase::PlayerTurn) {
            SetFontSize(28); DrawFormatString(sbX + 70, 950, GetColor(120, 120, 120), "（マップをクリックして移動先を選んでください）");
        }
    }

    bool BattleMaster::CheckButtonClick(int x, int y, int w, int h, Vector2 m) {
        return (m.x >= (float)x && m.x <= (float)(x + w) && m.y >= (float)y && m.y <= (float)(y + h));
    }

    void BattleMaster::CheckTurnEnd() {
    }

    bool BattleMaster::IsGameOver() const {
        return false;
    }

} // namespace App