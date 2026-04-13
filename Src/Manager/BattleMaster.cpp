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
        m_player = std::make_unique<Player>(IntVector2{ 1, 1 }, m_mapGrid.GetCellCenter(1, 1), 5, 5, 5);
        m_enemy = std::make_unique<Enemy>(IntVector2{ 7, 7 }, m_mapGrid.GetCellCenter(7, 7), 7, 5, 5);

        m_player->SetOp('\0');
        m_enemy->SetOp('\0');

        m_currentPhase = Phase::PlayerTurn;
        m_isPlayerSelected = false;

        m_actionLog.clear();
        AddLog(">>> バトル開始！ まずは演算子を拾え！");
    }

    bool BattleMaster::CanMove(int number, char op, IntVector2 start, IntVector2 target, int& outCost) const {
        if (start == target) return false;

        int dx = std::abs(target.x - start.x);
        int dy = std::abs(target.y - start.y);
        int maxDist = (number - 1) % 3 + 1;

        bool isValidDirection = false;
        bool isJump = false;

        if (number >= 1 && number <= 3) isValidDirection = (dx == 0 || dy == 0);
        else if (number >= 4 && number <= 6) isValidDirection = (dx == dy);
        else if (number >= 7 && number <= 9) isValidDirection = (dx == 0 || dy == 0 || dx == dy);

        if (op == '+') isValidDirection |= (dx == 0 || dy == 0);
        else if (op == '-') isValidDirection |= (dy == 0);
        else if (op == '*') isValidDirection |= (dx == dy);
        else if (op == '/') {
            isValidDirection |= (dy == 0);
            if (dx == 0 && dy == 2) {
                isValidDirection = true;
                isJump = true;
            }
        }

        if (isValidDirection) {
            if (isJump) {
                outCost = 2;
                return true;
            }
            else if (dx <= maxDist && dy <= maxDist) {
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

        // 【1. プレイヤー：移動フェーズ】 (変更なし)
        if (m_currentPhase == Phase::PlayerTurn && m_player && !m_player->IsMoving()) {
            if (input.IsMouseLeftTrg()) {
                IntVector2 pPos = m_player->GetGridPos();

                if (!m_isPlayerSelected) {
                    if (m_hoverGrid == pPos) m_isPlayerSelected = true;
                }
                else {
                    if (m_hoverGrid == pPos) {
                        m_player->AddNumber(-1);
                        AddLog("【待機】 その場で数値を調整した (コスト: -1)");
                        m_isPlayerSelected = false;
                        m_currentPhase = Phase::ActionSelect;
                    }
                    else if (m_mapGrid.IsWithinBounds(m_hoverGrid.x, m_hoverGrid.y)) {
                        int cost = 0;
                        if (CanMove(m_player->GetNumber(), m_player->GetOp(), pPos, m_hoverGrid, cost)) {
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
                            m_currentPhase = Phase::ActionSelect;
                        }
                        else {
                            m_isPlayerSelected = false;
                        }
                    }
                }
            }
        }

        // 【1.5 プレイヤー：行動選択フェーズ（移動後）】
        if (m_currentPhase == Phase::ActionSelect && m_player && !m_player->IsMoving()) {
            IntVector2 pP = m_player->GetGridPos();
            char pickedItem = m_mapGrid.PickUpItem(pP.x, pP.y);
            if (pickedItem != '\0') {
                m_player->SetOp(pickedItem);
                AddLog("【取得】 演算子 [" + std::string(1, pickedItem) + "] を装備した！");
            }

            IntVector2 eP = m_enemy ? m_enemy->GetGridPos() : IntVector2{ -1, -1 };
            bool canAttack = m_enemy && (std::abs(pP.x - eP.x) + std::abs(pP.y - eP.y) == 1);
            bool hasOp = (m_player->GetOp() != '\0'); // ★追加：アイテムを持っているか

            if (input.IsMouseLeftTrg()) {
                if (canAttack && hasOp) { // ★修正：隣接 ＆ アイテム所持時のみ
                    if (CheckButtonClick(sbX + 40, 950, 270, 90, mousePos)) {
                        ExecuteBattle(*m_player, *m_enemy, *m_player);
                        m_currentPhase = Phase::EnemyTurn;
                        m_enemyAIStarted = false;
                    }
                    else if (CheckButtonClick(sbX + 330, 950, 270, 90, mousePos)) {
                        ExecuteBattle(*m_player, *m_enemy, *m_enemy);
                        m_currentPhase = Phase::EnemyTurn;
                        m_enemyAIStarted = false;
                    }
                    else if (CheckButtonClick(sbX + 620, 950, 270, 90, mousePos)) {
                        AddLog("【待機】 ターンを終了した");
                        m_currentPhase = Phase::EnemyTurn;
                        m_enemyAIStarted = false;
                    }
                }
                else {
                    // ★修正：敵がいない、またはアイテムがない場合は待機ボタンのみ
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
                        char eOp = m_enemy->GetOp();
                        if (eOp != '\0') { // ★追加：敵もアイテムがないと攻撃できない
                            int eNum = m_enemy->GetNumber();
                            int pNum = m_player->GetNumber();
                            int res = 0;
                            if (eOp == '+') res = eNum + pNum;
                            else if (eOp == '-') res = eNum - pNum;
                            else if (eOp == '*') res = eNum * pNum;
                            else if (eOp == '/') res = (pNum != 0) ? eNum / pNum : 0;

                            if (res > 9) ExecuteBattle(*m_enemy, *m_player, *m_enemy);
                            else ExecuteBattle(*m_enemy, *m_player, *m_player);
                        }
                        else {
                            AddLog("【待機】 敵は演算子がないため様子を見ている");
                        }
                    }

                    m_mapGrid.UpdateTurn();
                    m_currentPhase = Phase::PlayerTurn;
                }
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
        int bestCost = 1;

        for (int x = 0; x < m_mapGrid.GetWidth(); ++x) {
            for (int y = 0; y < m_mapGrid.GetHeight(); ++y) {
                IntVector2 target{ x, y };
                if (target == pPos) continue;

                int cost = 0;
                bool canMove = false;

                if (target == ePos) {
                    canMove = true;
                    cost = 1;
                }
                else {
                    canMove = CanMove(eNum, eOp, ePos, target, cost);
                }

                if (canMove) {
                    int score = 0;
                    int distToPlayer = std::abs(pPos.x - target.x) + std::abs(pPos.y - target.y);

                    if (distToPlayer == 1) {
                        score += 1000;
                    }
                    else {
                        score -= distToPlayer * 10;
                    }

                    if (target != ePos && m_mapGrid.GetItemAt(target.x, target.y) != '\0') {
                        score += 50;
                    }
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
            m_enemy->AddNumber(-bestCost);
            AddLog("【待機】 敵ユニットがその場で数値を調整した (コスト: -1)");
        }
    }

    void BattleMaster::ApplyBattleResult(UnitBase& unit, int resultNum) {
        std::string name = (&unit == m_player.get()) ? "プレイヤー" : "敵";

        // --- 1. 数値のループ計算 (1～9の時計) ---
        int cycleValue = (resultNum - 1) % 9;
        if (cycleValue < 0) cycleValue += 9;
        int remain = cycleValue + 1;

        // --- 2. ストックの増減計算（★修正：インフレ防止のため 1 固定） ---
        if (resultNum <= 0) {
            // 0以下ならどれだけ小さくてもダメージは「1」
            unit.AddStocks(-1);
            AddLog("【的中】 " + name + " の残機 -1！");
        }
        else if (resultNum > 9) {
            // 9を超えたらどれだけ大きくても回復は「1」
            unit.AddStocks(1);
            AddLog("【回復】 " + name + " の残機 +1！");
        }
        else {
            AddLog("【変化】 " + name + " の数値が書き換わった。");
        }

        // 最終的な数値をセット
        unit.SetNumber(remain);
        AddLog("【結果】 " + name + " の数値は [" + std::to_string(remain) + "] になった。");
    }

    void BattleMaster::ExecuteBattle(UnitBase& attacker, UnitBase& defender, UnitBase& target) {
        char aOp = attacker.GetOp();
        int aNum = attacker.GetNumber();
        int dNum = defender.GetNumber();

        int result = 0;
        if (aOp == '+') result = aNum + dNum;
        else if (aOp == '-') result = aNum - dNum;
        else if (aOp == '*') result = aNum * dNum;
        else if (aOp == '/') result = (dNum != 0) ? aNum / dNum : 0;

        std::string aName = (&attacker == m_player.get()) ? "プレイヤー" : "敵";
        std::string tName = (&target == m_player.get()) ? "プレイヤー" : "敵";

        AddLog(">> " + aName + " の計算！ [" + std::to_string(aNum) + " " + std::string(1, aOp) + " " + std::to_string(dNum) + " = " + std::to_string(result) + "]");
        AddLog(">> 結果を " + tName + " に適用！");

        ApplyBattleResult(target, result);

        // ★修正：使用した演算子を消費し、未所持（\0）に戻す
        attacker.SetOp('\0');
    }

    void BattleMaster::DrawEnemyDangerArea() {
        if (!m_enemy || m_enemy->GetStocks() <= 0 || m_enemy->IsMoving()) return;

        IntVector2 ePos = m_enemy->GetGridPos();
        int eNum = m_enemy->GetNumber();
        char eOp = m_enemy->GetOp();

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 80);

        for (int x = 0; x < m_mapGrid.GetWidth(); ++x) {
            for (int y = 0; y < m_mapGrid.GetHeight(); ++y) {
                IntVector2 target{ x, y };
                Vector2 center = m_mapGrid.GetCellCenter(x, y);

                if (target == ePos) {
                    DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, GetColor(255, 50, 50), TRUE);
                    continue;
                }

                int dummyCost = 0;
                bool canMoveBase = CanMove(eNum, '\0', ePos, target, dummyCost);
                bool canMoveCombined = CanMove(eNum, eOp, ePos, target, dummyCost);

                if (canMoveCombined) {
                    if (canMoveBase) {
                        DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, GetColor(200, 50, 50), TRUE);
                    }
                    else {
                        DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, GetColor(255, 100, 200), TRUE);
                    }
                }
            }
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    void BattleMaster::DrawMovableArea() {
        if (!m_isPlayerSelected || !m_player || m_player->IsMoving()) return;

        IntVector2 pPos = m_player->GetGridPos();
        int pNum = m_player->GetNumber();
        char pOp = m_player->GetOp();

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);

        for (int x = 0; x < m_mapGrid.GetWidth(); ++x) {
            for (int y = 0; y < m_mapGrid.GetHeight(); ++y) {
                IntVector2 target{ x, y };
                Vector2 center = m_mapGrid.GetCellCenter(x, y);

                if (target == pPos) {
                    DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, GetColor(255, 255, 0), TRUE);
                    continue;
                }

                int dummyCost = 0;
                bool canMoveBase = CanMove(pNum, '\0', pPos, target, dummyCost);
                bool canMoveCombined = CanMove(pNum, pOp, pPos, target, dummyCost);

                if (canMoveCombined) {
                    if (canMoveBase) {
                        DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, GetColor(100, 200, 255), TRUE);
                    }
                    else {
                        DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, GetColor(255, 180, 50), TRUE);
                    }
                }
            }
        }

        if (m_mapGrid.IsWithinBounds(m_hoverGrid.x, m_hoverGrid.y)) {
            int hoverCost = 0;
            bool isHoverSelf = (m_hoverGrid == pPos);

            if (isHoverSelf || CanMove(pNum, pOp, pPos, m_hoverGrid, hoverCost)) {
                Vector2 hc = m_mapGrid.GetCellCenter(m_hoverGrid.x, m_hoverGrid.y);
                DrawBox((int)hc.x - 35, (int)hc.y - 35, (int)hc.x + 35, (int)hc.y + 35, GetColor(255, 255, 255), FALSE);
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

        DrawBox(sbX, 0, 1920, 1080, GetColor(20, 20, 25), TRUE);
        DrawLine(sbX, 0, sbX, 1080, GetColor(80, 120, 180), 4);

        unsigned int phaseCol = (m_currentPhase == Phase::EnemyTurn) ? GetColor(180, 40, 40) : GetColor(40, 80, 160);
        DrawBox(sbX, 0, 1920, 70, phaseCol, TRUE);
        SetFontSize(38);
        const char* phaseName = (m_currentPhase == Phase::PlayerTurn) ? "プレイヤーのターン (移動選択)" :
            (m_currentPhase == Phase::ActionSelect) ? "プレイヤーのターン (行動選択)" : "エネミーのターン (思考中)";
        DrawFormatString(sbX + 40, 15, GetColor(255, 255, 255), ">>> %s <<<", phaseName);

        SetFontSize(24);
        DrawFormatString(sbX + 40, 85, GetColor(180, 180, 180), "経過ターン: %d", m_mapGrid.GetTotalTurns());
        DrawFormatString(sbX + 700, 85, GetColor(255, 200, 50), "アイテム再生成: %d", m_mapGrid.GetTurnsUntilNextSpawn());

        Vector2 mousePos = InputManager::GetInstance().GetMousePos();
        IntVector2 pP = m_player ? m_player->GetGridPos() : IntVector2{ -1,-1 };
        IntVector2 eP = m_enemy ? m_enemy->GetGridPos() : IntVector2{ -1,-1 };

        bool canAttack = m_enemy && (std::abs(pP.x - eP.x) + std::abs(pP.y - eP.y) == 1);
        bool hasOp = (m_player && m_player->GetOp() != '\0'); // ★アイテム所持フラグ

        bool isHoverSelf = canAttack && hasOp && CheckButtonClick(sbX + 40, 950, 270, 90, mousePos);
        bool isHoverEnemy = canAttack && hasOp && CheckButtonClick(sbX + 330, 950, 270, 90, mousePos);

        int sy = 140;
        SetFontSize(28);
        DrawString(sbX + 40, sy, "【 計算予測 】", GetColor(255, 255, 100));
        DrawBox(sbX + 30, sy + 35, sbX + 930, sy + 250, GetColor(35, 40, 55), TRUE);
        DrawBox(sbX + 30, sy + 35, sbX + 930, sy + 250, GetColor(80, 120, 180), FALSE);

        if (canAttack && hasOp) { // ★修正：アイテムがある時だけ計算表示
            int pNum = m_player->GetNumber();
            int eNum = m_enemy->GetNumber();
            char pOp = m_player->GetOp();
            int res = 0;
            if (pOp == '+') res = pNum + eNum;
            else if (pOp == '-') res = pNum - eNum;
            else if (pOp == '*') res = pNum * eNum;
            else if (pOp == '/') res = (eNum != 0) ? pNum / eNum : 0;

            SetFontSize(80);
            DrawFormatString(sbX + 220, sy + 60, GetColor(255, 255, 255), "%d %c %d =", pNum, pOp, eNum);

            int cv = (res - 1) % 9; if (cv < 0) cv += 9; int remain = cv + 1;

            unsigned int resColor = (res <= 0) ? GetColor(255, 100, 100) : (res > 9) ? GetColor(100, 255, 100) : GetColor(255, 255, 150);
            DrawFormatString(sbX + 680, sy + 60, resColor, "%d", res);

            SetFontSize(30);
            if (isHoverSelf || isHoverEnemy) {
                const char* target = isHoverSelf ? "プレイヤー" : "敵";
                unsigned int tCol = isHoverSelf ? GetColor(100, 255, 150) : GetColor(255, 120, 120);

                if (res <= 0)
                    DrawFormatString(sbX + 220, sy + 185, tCol, "▼ %s の残機が 1 減り、数値は %d になります", target, remain);
                else if (res > 9)
                    DrawFormatString(sbX + 220, sy + 185, tCol, "▼ %s の残機が 1 増え、数値は %d になります", target, remain);
                else
                    DrawFormatString(sbX + 220, sy + 185, tCol, "▼ %s の残機は変わらず、数値が %d に変化します", target, remain);
            }
            else {
                DrawString(sbX + 310, sy + 185, "適用するボタンを選んでください", GetColor(150, 160, 180));
            }
        }
        else if (canAttack && !hasOp) { // ★追加：敵はいるがアイテムがない時
            SetFontSize(32);
            DrawString(sbX + 280, sy + 100, "【 演算子アイテムが必要です 】", GetColor(255, 100, 100));
            SetFontSize(26);
            DrawString(sbX + 250, sy + 150, "マップ上のアイテムを拾うまで計算できません", GetColor(150, 150, 150));
        }
        else {
            SetFontSize(30);
            DrawString(sbX + 320, sy + 120, "( 射程内に敵がいません )", GetColor(100, 110, 130));
        }

        // --- 4. ユニット情報比較 ---
        auto drawUnitCard = [&](int x, int y, UnitBase* unit, bool isPlayer) {
            unsigned int baseCol = isPlayer ? GetColor(100, 200, 255) : GetColor(255, 100, 100);
            SetFontSize(28);
            DrawString(x, y, isPlayer ? "■ プレイヤー (自分)" : "■ ターゲット (相手)", baseCol);

            if (!unit) return;

            SetFontSize(20);
            DrawString(x + 20, y + 45, "残機:", GetColor(180, 180, 180));
            for (int i = 0; i < unit->GetMaxStocks(); ++i) {
                unsigned int sCol = (i < unit->GetStocks()) ? baseCol : GetColor(60, 60, 70);
                DrawBox(x + 80 + i * 25, y + 45, x + 100 + i * 25, y + 65, sCol, TRUE);
            }

            int num = unit->GetNumber();
            SetFontSize(24);
            DrawFormatString(x + 20, y + 85, GetColor(255, 255, 255), "現在の数値 : %d", num);

            // ★未所持の場合は表示を変える
            char curOp = unit->GetOp();
            const char* opDisp = (curOp == '\0') ? "未所持" : (curOp == '+') ? "[ ＋ ]" : (curOp == '-') ? "[ － ]" : (curOp == '*') ? "[ ＊ ]" : "[ ／ ]";
            DrawFormatString(x + 20, y + 115, GetColor(255, 255, 100), "演算子　　 : %s", opDisp);

            const char* moveName = (num <= 3) ? "十字移動 (+)" : (num <= 6) ? "ナナメ移動 (x)" : "全方位移動 (*)";
            DrawFormatString(x + 20, y + 145, GetColor(200, 200, 200), "移動タイプ : %s", moveName);

            int ix = x + 340, iy = y + 80;
            for (int i = 0; i < 9; ++i) {
                int gridX = i % 3, gridY = i / 3;
                bool canGo = false;
                if (i == 4) canGo = false;
                else if (num <= 3) canGo = (gridX == 1 || gridY == 1);
                else if (num <= 6) canGo = (gridX == gridY || gridX + gridY == 2);
                else canGo = true;

                unsigned int dotCol = canGo ? baseCol : GetColor(50, 50, 60);
                DrawBox(ix + gridX * 14, iy + gridY * 14, ix + gridX * 14 + 11, iy + gridY * 14 + 11, dotCol, TRUE);
            }
            };

        int uy = 430;
        drawUnitCard(sbX + 50, uy, m_player.get(), true);
        drawUnitCard(sbX + 500, uy, m_enemy.get(), false);

        int ly = 650;
        SetFontSize(26);
        DrawString(sbX + 40, ly, "【 直近の履歴 】", GetColor(150, 150, 160));
        DrawBox(sbX + 40, ly + 35, sbX + 920, ly + 210, GetColor(15, 15, 20), TRUE);
        SetFontSize(20);
        for (size_t i = 0; i < m_actionLog.size(); ++i) {
            DrawFormatString(sbX + 60, ly + 55 + (int)i * 24, GetColor(160, 220, 160), "> %s", m_actionLog[i].c_str());
        }

        int ry = 880;
        SetFontSize(18);
        DrawBox(sbX + 40, ry, sbX + 920, ry + 55, GetColor(30, 35, 45), TRUE);
        DrawFormatString(sbX + 60, ry + 10, GetColor(150, 150, 150), "移動の法則: 1-3＝十字 ｜ 4-6＝ナナメ ｜ 7-9＝全方位");
        DrawFormatString(sbX + 60, ry + 30, GetColor(150, 150, 150), "演算子の力: ＋＝足す ｜ －＝引く ｜ ＊＝掛ける ｜ ÷＝割る");

        // --- 7. 操作ボタン ---
        if (m_currentPhase == Phase::ActionSelect) {
            if (canAttack && hasOp) { // ★修正：アイテムがある時だけ３択表示
                auto drawBtn = [&](int x, int y, int w, int h, const char* text, unsigned int col, bool hover) {
                    unsigned int c = hover ? col : col - 0x303030;
                    DrawBox(x, y, x + w, y + h, c, TRUE);
                    DrawBox(x, y, x + w, y + h, GetColor(255, 255, 255), FALSE);
                    SetFontSize(28);
                    DrawFormatString(x + (w - GetDrawStringWidth(text, (int)strlen(text))) / 2, y + 30, GetColor(255, 255, 255), text);
                    };

                drawBtn(sbX + 40, 950, 270, 90, "自分を書き換える", GetColor(50, 150, 80), isHoverSelf);
                drawBtn(sbX + 330, 950, 270, 90, "敵を攻撃する", GetColor(180, 50, 50), isHoverEnemy);
                drawBtn(sbX + 620, 950, 270, 90, "待機する", GetColor(80, 80, 100), CheckButtonClick(sbX + 620, 950, 270, 90, mousePos));
            }
            else {
                // ★修正：敵がいない or アイテムがない時は「ターン終了」のみ
                bool isHoverWait = CheckButtonClick(sbX + 200, 950, 560, 90, mousePos);
                unsigned int waitCol = isHoverWait ? GetColor(100, 100, 120) : GetColor(70, 70, 90);
                DrawBox(sbX + 200, 950, sbX + 760, 1040, waitCol, TRUE);
                SetFontSize(36);
                DrawString(sbX + 365, 975, "ターンを終了する", GetColor(255, 255, 255));
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
    bool BattleMaster::IsPlayerWin() const {
        if (m_enemy && m_enemy->GetStocks() <= 0) return true;
        return false;
    }
} // namespace App