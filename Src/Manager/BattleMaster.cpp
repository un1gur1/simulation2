#define NOMINMAX
#include "BattleMaster.h"
#include <DxLib.h>
#include <cmath>
#include <algorithm>
#include "../Input/InputManager.h"
#include "../Scene/SceneManager.h" 

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace App {

    BattleMaster::BattleMaster()
        : m_currentPhase(Phase::P1_Move)
        , m_gameMode(GameMode::VS_CPU)
        , m_ruleMode(RuleMode::CLASSIC)
        , m_mapGrid(80, Vector2(600, 120))
        , m_p1ZeroOneScore(0, 1), m_p2ZeroOneScore(0, 1), m_targetScore(501)
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

    bool BattleMaster::Is1PTurn() const {
        return (m_currentPhase == Phase::P1_Move || m_currentPhase == Phase::P1_Action);
    }
    UnitBase* BattleMaster::GetActiveUnit() const {
        if (Is1PTurn()) return m_player.get();
        return m_enemy.get();
    }
    UnitBase* BattleMaster::GetTargetUnit() const {
        if (Is1PTurn()) return m_enemy.get();
        return m_player.get();
    }

    void BattleMaster::Init() {
        auto* sm = SceneManager::GetInstance();
        m_gameMode = (sm->GetPlayerCount() == 1) ? GameMode::VS_CPU : GameMode::VS_PLAYER;
        m_ruleMode = (sm->GetGameMode() == 0) ? RuleMode::CLASSIC : RuleMode::ZERO_ONE;

        m_targetScore = sm->GetZeroOneScore();
        m_p1ZeroOneScore = Fraction(0, 1);
        m_p2ZeroOneScore = Fraction(0, 1);

        m_player = std::make_unique<Player>(IntVector2{ 1, 1 }, m_mapGrid.GetCellCenter(1, 1), 5, 5, 5);
        m_enemy = std::make_unique<Enemy>(IntVector2{ 7, 7 }, m_mapGrid.GetCellCenter(7, 7), 7, 5, 5);

        m_player->SetOp('\0');
        m_enemy->SetOp('\0');

        m_currentPhase = Phase::P1_Move;
        m_isPlayerSelected = false;
        m_enemyAIStarted = false;

        m_actionLog.clear();
        std::string pModeStr = (m_gameMode == GameMode::VS_CPU) ? "NPC対戦" : "対人対戦";
        std::string rModeStr = (m_ruleMode == RuleMode::CLASSIC) ? "クラシック" : "ゼロワン";

        AddLog(">>> バトル開始！ [" + pModeStr + " / " + rModeStr + "]");

        if (m_ruleMode == RuleMode::ZERO_ONE) {
            AddLog(">>> 目標：相手よりはやくスコアを【 " + std::to_string(m_targetScore) + " 】にしよう！");
        }
        else {
            AddLog(">>> 目標：相手の残機をゼロに！");
        }
    }

    bool BattleMaster::CanMove(int number, char op, IntVector2 start, IntVector2 target, int& outCost) const {
        if (start == target) return false;
        int dx = std::abs(target.x - start.x);
        int dy = std::abs(target.y - start.y);

        // ★数値が小さいほど移動力が高い（逆転のパラドックス）
        int maxDist = 3 - ((number - 1) % 3);

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
            if (dx == 0 && dy == 2) { isValidDirection = true; isJump = true; }
        }

        if (isValidDirection) {
            if (isJump) { outCost = 2; return true; }
            else if (dx <= maxDist && dy <= maxDist) { outCost = (std::max)(dx, dy); return true; }
        }
        return false;
    }

    void BattleMaster::HandleMoveInput(UnitBase& activeUnit, Phase nextPhase) {
        if (activeUnit.IsMoving()) return;
        auto& input = InputManager::GetInstance();
        if (input.IsMouseLeftTrg()) {
            IntVector2 pos = activeUnit.GetGridPos();
            if (!m_isPlayerSelected) {
                if (m_hoverGrid == pos) m_isPlayerSelected = true;
            }
            else {
                if (m_hoverGrid == pos) {
                    activeUnit.AddNumber(-1);
                    AddLog("【待機】 その場で数字の調整を行った (コスト: -1)");
                    m_isPlayerSelected = false;
                    m_currentPhase = nextPhase;
                }
                else if (m_mapGrid.IsWithinBounds(m_hoverGrid.x, m_hoverGrid.y)) {
                    int cost = 0;
                    if (CanMove(activeUnit.GetNumber(), activeUnit.GetOp(), pos, m_hoverGrid, cost)) {
                        activeUnit.AddNumber(-cost);
                        std::string name = (&activeUnit == m_player.get()) ? "1P" : (m_gameMode == GameMode::VS_CPU ? "敵" : "2P");
                        AddLog("【移動】 " + name + " (コスト: -" + std::to_string(cost) + ")");
                        std::queue<Vector2> autoPath;
                        int stepX = (m_hoverGrid.x > pos.x) ? 1 : (m_hoverGrid.x < pos.x) ? -1 : 0;
                        int stepY = (m_hoverGrid.y > pos.y) ? 1 : (m_hoverGrid.y < pos.y) ? -1 : 0;
                        IntVector2 curr = pos;
                        while (curr != m_hoverGrid) {
                            curr.x += stepX; curr.y += stepY;
                            autoPath.push(m_mapGrid.GetCellCenter(curr.x, curr.y));
                        }
                        activeUnit.StartMove(m_hoverGrid, autoPath);
                        m_isPlayerSelected = false;
                        m_currentPhase = nextPhase;
                    }
                    else { m_isPlayerSelected = false; }
                }
            }
        }
    }

    void BattleMaster::HandleActionInput(UnitBase& actor, UnitBase& targetUnit) {
        if (actor.IsMoving()) return;
        IntVector2 pos = actor.GetGridPos();
        char pickedItem = m_mapGrid.PickUpItem(pos.x, pos.y);
        if (pickedItem != '\0') {
            actor.SetOp(pickedItem);
            std::string name = (&actor == m_player.get()) ? "1P" : (m_gameMode == GameMode::VS_CPU ? "敵" : "2P");
            AddLog("【取得】 " + name + "が [" + std::string(1, pickedItem) + "] を取得！");
        }
        IntVector2 targetPos = targetUnit.GetGridPos();
        bool canAttack = (std::abs(pos.x - targetPos.x) + std::abs(pos.y - targetPos.y) == 1);
        bool hasOp = (actor.GetOp() != '\0');
        auto& input = InputManager::GetInstance();
        Vector2 mousePos = input.GetMousePos();

        if (input.IsMouseLeftTrg()) {
            Phase nextTurnPhase = (&actor == m_player.get()) ? Phase::P2_Move : Phase::P1_Move;
            if (canAttack && hasOp) {
                if (CheckButtonClick(600, 970, 220, 70, mousePos)) {
                    ExecuteBattle(actor, targetUnit, actor);
                    m_currentPhase = nextTurnPhase;
                    if (&actor != m_player.get()) m_mapGrid.UpdateTurn();
                }
                else if (CheckButtonClick(850, 970, 220, 70, mousePos)) {
                    ExecuteBattle(actor, targetUnit, targetUnit);
                    m_currentPhase = nextTurnPhase;
                    if (&actor != m_player.get()) m_mapGrid.UpdateTurn();
                }
                else if (CheckButtonClick(1100, 970, 220, 70, mousePos)) {
                    AddLog("【待機】 ターン終了");
                    m_currentPhase = nextTurnPhase;
                    if (&actor != m_player.get()) m_mapGrid.UpdateTurn();
                }
            }
            else {
                if (CheckButtonClick(750, 970, 420, 70, mousePos)) {
                    AddLog("【待機】 ターン終了");
                    m_currentPhase = nextTurnPhase;
                    if (&actor != m_player.get()) m_mapGrid.UpdateTurn();
                }
            }
        }
    }

    void BattleMaster::Update() {
        if (m_ruleMode == RuleMode::ZERO_ONE) {
            if (m_player && m_player->GetStocks() < m_player->GetMaxStocks()) {
                m_player->AddStocks(m_player->GetMaxStocks() - m_player->GetStocks());
            }
            if (m_enemy && m_enemy->GetStocks() < m_enemy->GetMaxStocks()) {
                m_enemy->AddStocks(m_enemy->GetMaxStocks() - m_enemy->GetStocks());
            }
        }

        auto& input = InputManager::GetInstance();
        input.Update();
        if (m_player) m_player->Update();
        if (m_enemy)  m_enemy->Update();
        m_hoverGrid = m_mapGrid.ScreenToGrid(input.GetMousePos());

        switch (m_currentPhase) {
        case Phase::P1_Move: HandleMoveInput(*m_player, Phase::P1_Action); break;
        case Phase::P1_Action: HandleActionInput(*m_player, *m_enemy); break;
        case Phase::P2_Move:
            if (m_gameMode == GameMode::VS_CPU) {
                if (!m_enemyAIStarted) { ExecuteEnemyAI(); m_enemyAIStarted = true; }
                else if (m_enemy && !m_enemy->IsMoving()) { m_currentPhase = Phase::P2_Action; }
            }
            else { HandleMoveInput(*m_enemy, Phase::P2_Action); }
            break;
        case Phase::P2_Action:
            if (m_gameMode == GameMode::VS_CPU) {
                IntVector2 eP = m_enemy->GetGridPos();
                char pickedItem = m_mapGrid.PickUpItem(eP.x, eP.y);
                if (pickedItem != '\0') {
                    m_enemy->SetOp(pickedItem);
                    AddLog("【取得】 敵が演算子 [" + std::string(1, pickedItem) + "] を取得");
                }
                IntVector2 pP = m_player->GetGridPos();
                if (std::abs(pP.x - eP.x) + std::abs(pP.y - eP.y) == 1) {
                    char eOp = m_enemy->GetOp();
                    if (eOp != '\0') {
                        if (m_ruleMode == RuleMode::ZERO_ONE) {
                            ExecuteBattle(*m_enemy, *m_player, *m_enemy);
                        }
                        else {
                            int eNum = m_enemy->GetNumber();
                            int pNum = m_player->GetNumber();
                            int res = 0;
                            if (eOp == '+') res = eNum + pNum; else if (eOp == '-') res = eNum - pNum; else if (eOp == '*') res = eNum * pNum; else if (eOp == '/') res = (pNum != 0) ? eNum / pNum : 0;
                            if (res > 9) ExecuteBattle(*m_enemy, *m_player, *m_enemy);
                            else ExecuteBattle(*m_enemy, *m_player, *m_player);
                        }
                    }
                    else { AddLog("【待機】 敵は演算子がないため様子を見ている"); }
                }
                m_mapGrid.UpdateTurn();
                m_currentPhase = Phase::P1_Move;
                m_enemyAIStarted = false;
            }
            else { HandleActionInput(*m_enemy, *m_player); }
            break;
        }
    }

    void BattleMaster::ExecuteEnemyAI() {
        if (!m_enemy || m_enemy->GetStocks() <= 0 || !m_player) {
            m_mapGrid.UpdateTurn(); m_currentPhase = Phase::P1_Move; return;
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
                if (target == ePos) { canMove = true; cost = 1; }
                else { canMove = CanMove(eNum, eOp, ePos, target, cost); }

                if (canMove) {
                    int score = 0;
                    int distToPlayer = std::abs(pPos.x - target.x) + std::abs(pPos.y - target.y);
                    if (distToPlayer == 1) score += 1000; else score -= distToPlayer * 10;
                    if (target != ePos && m_mapGrid.GetItemAt(target.x, target.y) != '\0') score += 50;
                    if (target == ePos) score += 5;
                    if (score > maxScore) { maxScore = score; bestTarget = target; bestCost = cost; }
                }
            }
        }
        if (bestTarget != ePos) {
            AddLog("【行動】 敵 (コスト: -" + std::to_string(bestCost) + ")");
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
            AddLog("【待機】 敵がその場で数字の調整を行った (コスト: -1)");
        }
    }

    void BattleMaster::ExecuteBattle(UnitBase& attacker, UnitBase& defender, UnitBase& target) {
        char aOp = attacker.GetOp();
        int aNum = attacker.GetNumber();
        int dNum = defender.GetNumber();

        Fraction resFrac;
        if (aOp == '+') resFrac = Fraction(aNum + dNum);
        else if (aOp == '-') resFrac = Fraction(aNum - dNum);
        else if (aOp == '*') resFrac = Fraction(aNum * dNum);
        else if (aOp == '/') resFrac = Fraction(aNum, dNum);

        int intRes = 0;
        if (aOp == '+') intRes = aNum + dNum;
        else if (aOp == '-') intRes = aNum - dNum;
        else if (aOp == '*') intRes = aNum * dNum;
        else if (aOp == '/') intRes = (dNum != 0) ? aNum / dNum : 0;

        std::string aName = (&attacker == m_player.get()) ? "1P" : (m_gameMode == GameMode::VS_CPU ? "敵" : "2P");
        AddLog(">> " + aName + " の計算！ [ 結果: " + resFrac.ToString() + " ]");

        ApplyBattleResult(target, resFrac, intRes);
        attacker.SetOp('\0');
    }

    void BattleMaster::ApplyBattleResult(UnitBase& unit, Fraction resultFrac, int intRes) {
        std::string name = (&unit == m_player.get()) ? "1P" : (m_gameMode == GameMode::VS_CPU ? "敵" : "2P");

        if (m_ruleMode == RuleMode::ZERO_ONE) {
            Fraction& currentScore = (&unit == m_player.get()) ? m_p1ZeroOneScore : m_p2ZeroOneScore;
            Fraction goalScore(m_targetScore);

            Fraction predictedScore = currentScore + resultFrac;

            if (predictedScore == goalScore) {
                currentScore = predictedScore;
                AddLog("【FINISH!!】 " + name + " が目標スコアに到達した！");
            }
            else if (predictedScore > goalScore) {
                AddLog("【BUST!!】 " + name + " は目標をオーバーした！(無効)");
            }
            else {
                currentScore = predictedScore;
                AddLog("【HIT】 " + name + " のスコアが [" + currentScore.ToString() + "] に変化。");
            }

            int cycleValue = (intRes - 1) % 9;
            if (cycleValue < 0) cycleValue += 9;
            int remain = cycleValue + 1;
            unit.SetNumber(remain);
        }
        else {
            int cycleValue = (intRes - 1) % 9;
            if (cycleValue < 0) cycleValue += 9;
            int remain = cycleValue + 1;

            if (intRes <= 0) {
                unit.AddStocks(-1);
                AddLog("【ー】 " + name + " の残機 -1！");
            }
            else if (intRes > 9) {
                unit.AddStocks(1);
                AddLog("【＋】 " + name + " の残機 +1！");
            }
            else {
                AddLog("【±】 " + name + " の数値に反映");
            }
            unit.SetNumber(remain);
            AddLog("【結果】 " + name + " の数値は [" + std::to_string(remain) + "] になった。");
        }
    }

    void BattleMaster::DrawEnemyDangerArea() {
        if (!m_enemy || m_enemy->GetStocks() <= 0 || m_enemy->IsMoving()) return;
        if (m_gameMode == GameMode::VS_PLAYER) return;
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
                    if (canMoveBase) DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, GetColor(200, 50, 50), TRUE);
                    else DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, GetColor(255, 100, 200), TRUE);
                }
            }
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    void BattleMaster::DrawMovableArea() {
        if (!m_isPlayerSelected) return;
        UnitBase* activeUnit = GetActiveUnit();
        if (!activeUnit || activeUnit->IsMoving()) return;
        IntVector2 pPos = activeUnit->GetGridPos();
        int pNum = activeUnit->GetNumber();
        char pOp = activeUnit->GetOp();

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
                    if (canMoveBase) DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, GetColor(100, 200, 255), TRUE);
                    else DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, GetColor(255, 180, 50), TRUE);
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
        // ==========================================
        // 1. ベース背景と左右パネル
        // ==========================================
        DrawBox(0, 0, 1920, 1080, GetColor(10, 12, 18), TRUE);

        DrawBox(0, 70, 580, 1080, GetColor(18, 18, 22), TRUE);
        DrawBox(576, 70, 580, 1080, GetColor(255, 165, 0), TRUE);
        DrawLine(575, 70, 575, 1080, GetColor(255, 220, 100), 1);

        DrawBox(1340, 70, 1920, 1080, GetColor(18, 18, 22), TRUE);
        DrawBox(1340, 70, 1344, 1080, GetColor(30, 50, 180), TRUE);
        DrawLine(1344, 70, 1344, 1080, GetColor(100, 150, 255), 1);

        // ==========================================
        // 2. マップとユニットの描画
        // ==========================================
        m_mapGrid.Draw();
        DrawEnemyDangerArea();
        DrawMovableArea();

        // アクティブユニットの点滅オーラ（★浮遊に同期させる修正！）
        UnitBase* blinkUnit = GetActiveUnit();
        if (blinkUnit && !blinkUnit->IsMoving()) {
            IntVector2 bPos = blinkUnit->GetGridPos();
            Vector2 bCenter = m_mapGrid.GetCellCenter(bPos.x, bPos.y);

            double time = GetNowCount() / 1000.0;
            // ユニットごとの浮遊（bobbing）計算を適用
            float bobbing = (blinkUnit == m_player.get()) ? (float)(sin(time * 2.5) * 5.0) : (float)(sin(time * 2.0) * 4.0);
            bCenter.y += bobbing; // ★影ではなく、本体の高さにオーラを合わせる

            int alpha = (int)(150 + 105 * sin(time * M_PI)); // 脈動
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
            unsigned int auraCol = (blinkUnit == m_player.get()) ? GetColor(255, 220, 50) : GetColor(60, 150, 255);

            for (int i = 0; i < 3; ++i) {
                DrawBox((int)bCenter.x - 39 + i, (int)bCenter.y - 39 + i, (int)bCenter.x + 39 - i, (int)bCenter.y + 39 - i, auraCol, FALSE);
            }
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha / 4);
            DrawBox((int)bCenter.x - 36, (int)bCenter.y - 36, (int)bCenter.x + 36, (int)bCenter.y + 36, auraCol, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        if (m_player) m_player->Draw();
        if (m_enemy)  m_enemy->Draw();

        // ==========================================
        // 3. 上部ヘッダー
        // ==========================================
        unsigned int phaseCol;
        const char* phaseName;

        if (m_currentPhase == Phase::P1_Move) { phaseCol = GetColor(180, 110, 0); phaseName = "1Pのターン (移動選択)"; }
        else if (m_currentPhase == Phase::P1_Action) { phaseCol = GetColor(180, 110, 0); phaseName = "1Pのターン (行動選択)"; }
        else if (m_currentPhase == Phase::P2_Move) { phaseCol = GetColor(30, 50, 120); phaseName = (m_gameMode == GameMode::VS_CPU) ? "敵のターン (思考中)" : "2Pのターン (移動選択)"; }
        else { phaseCol = GetColor(30, 50, 120); phaseName = (m_gameMode == GameMode::VS_CPU) ? "敵のターン (思考中)" : "2Pのターン (行動選択)"; }

        DrawBox(0, 0, 1920, 70, phaseCol, TRUE);
        DrawLine(0, 70, 1920, 70, GetColor(255, 255, 255), 2);

        SetFontSize(38);
        DrawFormatString(40, 16, GetColor(255, 255, 255), ">>> %s", phaseName);
        SetFontSize(24);
        DrawFormatString(800, 24, GetColor(220, 220, 220), "経過ターン: %d", m_mapGrid.GetTotalTurns());

        // ==========================================
        // 4. 左右のユニットステータスカード
        // ==========================================
        Vector2 mousePos = InputManager::GetInstance().GetMousePos();
        UnitBase* activeActor = GetActiveUnit();
        UnitBase* activeTarget = GetTargetUnit();

        IntVector2 aP = activeActor ? activeActor->GetGridPos() : IntVector2{ -1,-1 };
        IntVector2 tP = activeTarget ? activeTarget->GetGridPos() : IntVector2{ -1,-1 };
        bool canAttack = activeTarget && (std::abs(aP.x - tP.x) + std::abs(aP.y - tP.y) == 1);
        bool hasOp = (activeActor && activeActor->GetOp() != '\0');
        bool isHoverSelf = canAttack && hasOp && CheckButtonClick(600, 965, 220, 70, mousePos);
        bool isHoverEnemy = canAttack && hasOp && CheckButtonClick(850, 965, 220, 70, mousePos);

        auto drawUnitCard = [&](int x, int y, UnitBase* unit, bool is1P) {
            if (!unit) return;
            unsigned int baseCol = is1P ? GetColor(255, 165, 0) : GetColor(80, 120, 255);
            unsigned int darkBg = GetColor(14, 16, 20);

            std::string headerName = is1P ? "1P PLAYER" : (m_gameMode == GameMode::VS_CPU ? "ENEMY" : "2P PLAYER");

            SetFontSize(36);
            DrawString(x + 5, y + 5, headerName.c_str(), baseCol);
            DrawLine(x, y + 45, x + 500, y + 45, baseCol, 2);

            int scoreY = y + 70;
            DrawBox(x, scoreY, x + 500, scoreY + 200, darkBg, TRUE);

            if (m_ruleMode == RuleMode::ZERO_ONE) {
                // ゼロワンモードのスコア表示
                Fraction f = is1P ? m_p1ZeroOneScore : m_p2ZeroOneScore;
                long long whole = f.n / f.d;
                long long rem = std::abs(f.n % f.d);
                std::string wholeStr = std::to_string(whole);
                if (whole == 0 && f.n < 0) wholeStr = "-0";

                SetFontSize(140);
                DrawString(x + 24, scoreY + 14, wholeStr.c_str(), GetColor(0, 0, 0));
                DrawString(x + 20, scoreY + 10, wholeStr.c_str(), GetColor(255, 255, 255));

                if (f.d != 1 && rem != 0) {
                    int fx = x + GetDrawStringWidth(wholeStr.c_str(), (int)wholeStr.length()) + 30;
                    unsigned int fracCol = is1P ? GetColor(255, 220, 100) : GetColor(180, 220, 255);
                    SetFontSize(48);
                    DrawString(fx, scoreY + 60, (f.n < 0 && whole == 0) ? "-" : "+", fracCol);
                    SetFontSize(32);
                    DrawFormatString(fx + 40, scoreY + 30, fracCol, "%lld", rem);
                    DrawLine(fx + 35, scoreY + 68, fx + 85, scoreY + 68, fracCol, 3);
                    DrawFormatString(fx + 40, scoreY + 75, fracCol, "%lld", f.d);
                }

                SetFontSize(24);
                DrawBox(x, scoreY + 200, x + 500, scoreY + 235, is1P ? GetColor(60, 40, 0) : GetColor(20, 30, 80), TRUE);
                DrawFormatString(x + 15, scoreY + 206, GetColor(255, 255, 255), "TARGET SCORE : %d", m_targetScore);
            }
            else {
                // クラシックモードの自数値表示
                SetFontSize(160);
                DrawString(x + 24, scoreY + 14, std::to_string(unit->GetNumber()).c_str(), GetColor(0, 0, 0));
                DrawFormatString(x + 20, scoreY + 10, GetColor(255, 255, 255), "%d", unit->GetNumber());
            }

            // 予測ポップアップ（★クラシック対応）
            if (canAttack && hasOp) {
                bool isTargeted = (isHoverSelf && unit == activeActor) || (isHoverEnemy && unit == activeTarget);
                if (isTargeted) {
                    int aNum = activeActor->GetNumber(); int tNum = activeTarget->GetNumber(); char aOp = activeActor->GetOp();

                    int popX = x + 240, popY = scoreY + 140;
                    DrawBox(popX, popY, popX + 250, popY + 46, GetColor(10, 10, 10), TRUE);
                    DrawLine(popX, popY, popX + 250, popY, baseCol, 2);

                    if (m_ruleMode == RuleMode::ZERO_ONE) {
                        Fraction resFrac;
                        if (aOp == '+') resFrac = Fraction(aNum + tNum); else if (aOp == '-') resFrac = Fraction(aNum - tNum);
                        else if (aOp == '*') resFrac = Fraction(aNum * tNum); else if (aOp == '/') resFrac = Fraction(aNum, tNum);
                        Fraction current = (isHoverSelf && unit == activeActor) ? (is1P ? m_p1ZeroOneScore : m_p2ZeroOneScore) : (is1P ? m_p1ZeroOneScore : m_p2ZeroOneScore);
                        Fraction next = current + resFrac;

                        unsigned int diffCol = (resFrac.n >= 0) ? GetColor(100, 255, 100) : GetColor(255, 100, 100);
                        SetFontSize(28);
                        if (next > Fraction(m_targetScore)) DrawString(popX + 15, popY + 10, "!! BUST !!", GetColor(180, 180, 180));
                        else if (next == Fraction(m_targetScore)) DrawString(popX + 15, popY + 10, "FINISH !!", GetColor(255, 255, 0));
                        else DrawFormatString(popX + 15, popY + 10, diffCol, "%s%s", (resFrac.n >= 0) ? " +" : " ", resFrac.ToString().c_str());
                    }
                    else {
                        // ★ クラシックモードの予測ポップアップ（残機増減と変化）
                        int intRes = 0;
                        if (aOp == '+') intRes = aNum + tNum; else if (aOp == '-') intRes = aNum - tNum;
                        else if (aOp == '*') intRes = aNum * tNum; else if (aOp == '/') intRes = (tNum != 0) ? aNum / tNum : 0;

                        int cycleValue = (intRes - 1) % 9;
                        if (cycleValue < 0) cycleValue += 9;
                        int nextNum = cycleValue + 1;

                        SetFontSize(24);
                        if (intRes <= 0) DrawFormatString(popX + 10, popY + 12, GetColor(255, 80, 80), "→ 残機-1 ｜ 体力[%d]", nextNum);
                        else if (intRes > 9) DrawFormatString(popX + 10, popY + 12, GetColor(100, 255, 100), "→ 残機+1 ｜ 体力[%d]", nextNum);
                        else DrawFormatString(popX + 10, popY + 12, GetColor(200, 200, 200), "→ 体力が [%d] に", nextNum);
                    }
                }
            }

            int infoY = y + 330;
            DrawBox(x, infoY, x + 500, infoY + 120, darkBg, TRUE);
            SetFontSize(24);

            if (m_ruleMode == RuleMode::CLASSIC) {
                DrawString(x + 20, infoY + 15, "残機", GetColor(180, 180, 180));
                for (int i = 0; i < unit->GetMaxStocks(); ++i) {
                    unsigned int sCol = (i < unit->GetStocks()) ? baseCol : GetColor(40, 40, 50);
                    DrawBox(x + 80 + i * 22, infoY + 18, x + 96 + i * 22, infoY + 34, sCol, TRUE);
                }
            }

            int moveDist = 3 - ((unit->GetNumber() - 1) % 3);
            // ★ 文字被りを解消：文字を短くし、グリッド（ix）をさらに右へ離しました
            DrawFormatString(x + 20, infoY + 65, GetColor(220, 220, 220), "移動可能マス: %d マス", moveDist);

            // ★ グリッド位置修正
            int ix = x + 350, iy = infoY + 15;
            SetFontSize(20);
            DrawString(ix - 5, iy, "【移動範囲】", GetColor(150, 150, 150));
            for (int i = 0; i < 9; ++i) {
                int gx = i % 3, gy = i / 3;
                bool canGo = (i == 4) ? false : (unit->GetNumber() <= 3 ? (gx == 1 || gy == 1) : (unit->GetNumber() <= 6 ? (gx == gy || gx + gy == 2) : true));
                unsigned int dotCol = canGo ? baseCol : GetColor(40, 40, 50);
                DrawBox(ix + gx * 22, iy + 28 + gy * 22, ix + gx * 22 + 18, iy + 28 + gy * 22 + 18, dotCol, TRUE);
            }
            };

        drawUnitCard(40, 100, m_player.get(), true);
        drawUnitCard(1380, 100, m_enemy.get(), false);

        // ==========================================
        // 5. 下部のログとガイド表示
        // ==========================================
        int ly = 760;
        SetFontSize(22);

        DrawLine(40, ly, 540, ly, GetColor(60, 60, 70), 1);
        DrawString(40, ly + 10, "ACTION LOG", GetColor(150, 150, 160));
        SetFontSize(20);
        for (size_t i = 0; i < m_actionLog.size(); ++i) {
            DrawFormatString(40, ly + 45 + (int)i * 26, GetColor(180, 220, 160), "%s", m_actionLog[i].c_str());
        }

        SetFontSize(22);
        DrawLine(1380, ly, 1880, ly, GetColor(60, 60, 70), 1);
        DrawString(1380, ly + 10, "RULE GUIDE", GetColor(150, 150, 160));
        DrawString(1380, ly + 50, " 1, 4, 7 ＝ 3マス移動", GetColor(255, 200, 100));
        DrawString(1380, ly + 90, " 2, 5, 8 ＝ 2マス移動", GetColor(180, 180, 180));
        DrawString(1380, ly + 130, " 3, 6, 9 ＝ 1マス移動", GetColor(100, 150, 255));
        DrawString(1380, ly + 180, " ＋/－/＊/÷ ＝ 演算子アイテム\n取得で移動範囲拡大", GetColor(255, 255, 180));

        // ==========================================
        // 6. 中央下部のアクション＆計算エリア
        // ==========================================
        int py = 830;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
        DrawBox(600, py, 1320, 940, GetColor(5, 5, 8), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        unsigned int calcBorderCol = (m_currentPhase == Phase::P1_Action) ? GetColor(255, 165, 0) : GetColor(60, 100, 255);
        DrawLine(600, py, 1320, py, calcBorderCol, 2);

        if (canAttack && hasOp) {
            int aNum = activeActor->GetNumber();
            int tNum = activeTarget->GetNumber();
            char aOp = activeActor->GetOp();

            // ★ クラシックモードとゼロワンモードで結果の表示を分ける
            if (m_ruleMode == RuleMode::ZERO_ONE) {
                Fraction resFrac;
                if (aOp == '+') resFrac = Fraction(aNum + tNum); else if (aOp == '-') resFrac = Fraction(aNum - tNum);
                else if (aOp == '*') resFrac = Fraction(aNum * tNum); else if (aOp == '/') resFrac = Fraction(aNum, tNum);

                SetFontSize(64);
                DrawFormatString(672, py + 17, GetColor(0, 0, 0), "%d %c %d =", aNum, aOp, tNum);
                DrawFormatString(670, py + 15, GetColor(255, 255, 255), "%d %c %d =", aNum, aOp, tNum);

                unsigned int resColor = (resFrac.n <= 0) ? GetColor(255, 80, 80) : GetColor(255, 200, 0);
                long long rWhole = resFrac.n / resFrac.d;
                long long rRem = std::abs(resFrac.n % resFrac.d);
                int resX = 1010;

                if (resFrac.d == 1 || rRem == 0) {
                    DrawFormatString(resX + 2, py + 17, GetColor(0, 0, 0), "%lld", rWhole);
                    DrawFormatString(resX, py + 15, resColor, "%lld", rWhole);
                }
                else {
                    std::string rw = std::to_string(rWhole);
                    if (rWhole == 0 && resFrac.n < 0) rw = "-0";
                    DrawFormatString(resX + 2, py + 17, GetColor(0, 0, 0), "%s", rw.c_str());
                    DrawFormatString(resX, py + 15, resColor, "%s", rw.c_str());
                    int fnx = resX + GetDrawStringWidth(rw.c_str(), (int)rw.length()) + 20;
                    SetFontSize(32);
                    DrawFormatString(fnx + 5, py + 5, resColor, "%lld", rRem);
                    DrawLine(fnx, py + 43, fnx + 40, py + 43, resColor, 3);
                    DrawFormatString(fnx + 5, py + 48, resColor, "%lld", resFrac.d);
                }

                SetFontSize(22);
                if (isHoverSelf || isHoverEnemy) {
                    std::string targetName = isHoverSelf ? (activeActor == m_player.get() ? "1P" : "2P") : (activeTarget == m_player.get() ? "1P" : "2P");
                    DrawFormatString(650, py + 82, GetColor(255, 255, 255), "▼ %s のスコアにこの結果を反映", targetName.c_str());
                }
                else { DrawString(770, py + 82, "どちらに反映しますか", GetColor(120, 120, 130)); }
            }
            else {
                // ★ クラシックモードの明確な結果表示
                int intRes = 0;
                if (aOp == '+') intRes = aNum + tNum; else if (aOp == '-') intRes = aNum - tNum;
                else if (aOp == '*') intRes = aNum * tNum; else if (aOp == '/') intRes = (tNum != 0) ? aNum / tNum : 0;

                SetFontSize(64);
                DrawFormatString(672, py + 17, GetColor(0, 0, 0), "%d %c %d =", aNum, aOp, tNum);
                DrawFormatString(670, py + 15, GetColor(255, 255, 255), "%d %c %d =", aNum, aOp, tNum);

                unsigned int resColor = (intRes <= 0) ? GetColor(255, 80, 80) : (intRes > 9 ? GetColor(100, 255, 100) : GetColor(255, 200, 0));
                DrawFormatString(1012, py + 17, GetColor(0, 0, 0), "%d", intRes);
                DrawFormatString(1010, py + 15, resColor, "%d", intRes);

                SetFontSize(24);
                if (isHoverSelf || isHoverEnemy) {
                    std::string targetName = isHoverSelf ? (activeActor == m_player.get() ? "1P" : "2P") : (activeTarget == m_player.get() ? "1P" : "2P");
                    int nextNum = ((intRes - 1) % 9 + 9) % 9 + 1; // 安全なサイクル計算

                    if (intRes <= 0) {
                        DrawFormatString(620, py + 82, GetColor(255, 100, 100), "▼ 攻撃！ %s の【 残機を1減らし 】、体力を [%d] にします", targetName.c_str(), nextNum);
                    }
                    else if (intRes > 9) {
                        DrawFormatString(620, py + 82, GetColor(100, 255, 100), "▼ 回復！ %s の【 残機を1増やし 】、体力を [%d] にします", targetName.c_str(), nextNum);
                    }
                    else {
                        DrawFormatString(620, py + 82, GetColor(220, 220, 220), "▼ 変化！ %s の体力を [%d] に変更", targetName.c_str(), nextNum);
                    }
                }
                else { DrawString(770, py + 82, "適用する対象を選択してください", GetColor(120, 120, 130)); }
            }
        }
        else if (canAttack && !hasOp) {
            SetFontSize(30); DrawString(730, py + 40, "【 演算子アイテムが必要です 】", GetColor(255, 100, 100));
        }
        else {
            SetFontSize(28); DrawString(800, py + 40, "ターゲットが射程内にいません", GetColor(100, 110, 120));
        }

        // ==========================================
        // 7. アクション実行ボタン
        // ==========================================
        if (m_currentPhase == Phase::P1_Action || m_currentPhase == Phase::P2_Action) {
            if (m_currentPhase == Phase::P2_Action && m_gameMode == GameMode::VS_CPU) return;
            int by = 960;

            if (canAttack && hasOp) {
                auto drawBtn = [&](int x, int y, int w, int h, const char* text, unsigned int col, bool hover) {
                    if (hover) {
                        DrawBox(x, y, x + w, y + h, col, TRUE);
                        SetFontSize(26);
                        DrawFormatString(x + (w - GetDrawStringWidth(text, (int)strlen(text))) / 2, y + 22, GetColor(10, 10, 10), text);
                    }
                    else {
                        DrawBox(x, y, x + w, y + h, GetColor(20, 20, 25), TRUE);
                        DrawBox(x, y, x + w, y + h, col, FALSE);
                        SetFontSize(26);
                        DrawFormatString(x + (w - GetDrawStringWidth(text, (int)strlen(text))) / 2, y + 22, col, text);
                    }
                    };
                drawBtn(600, by, 220, 60, "自分", GetColor(100, 255, 150), isHoverSelf);
                drawBtn(850, by, 220, 60, "相手", GetColor(255, 100, 100), isHoverEnemy);
                drawBtn(1100, by, 220, 60, "何もしない", GetColor(150, 150, 160), CheckButtonClick(1100, by, 220, 60, mousePos));
            }
            else {
                bool hover = CheckButtonClick(750, by, 420, 60, mousePos);
                unsigned int btnCol = GetColor(180, 180, 200);
                if (hover) {
                    DrawBox(750, by, 1170, by + 60, btnCol, TRUE);
                    SetFontSize(28);
                    DrawString(875, by + 16, "ターン終了", GetColor(10, 10, 10));
                }
                else {
                    DrawBox(750, by, 1170, by + 60, GetColor(20, 20, 25), TRUE);
                    DrawBox(750, by, 1170, by + 60, btnCol, FALSE);
                    SetFontSize(28);
                    DrawString(875, by + 16, "ターン終了", btnCol);
                }
            }
        }
    }

    bool BattleMaster::CheckButtonClick(int x, int y, int w, int h, Vector2 m) {
        return (m.x >= (float)x && m.x <= (float)(x + w) && m.y >= (float)y && m.y <= (float)(y + h));
    }

    bool BattleMaster::IsGameOver() const {
        if (!m_player || !m_enemy) return false;

        if (m_ruleMode == RuleMode::ZERO_ONE) {
            Fraction goal(m_targetScore);
            if (m_p1ZeroOneScore == goal) return true;
            if (m_p2ZeroOneScore == goal) return true;
            return false;
        }

        if (m_player->GetStocks() <= 0) return true;
        if (m_enemy->GetStocks() <= 0) return true;
        return false;
    }

    bool BattleMaster::IsPlayerWin() const {
        if (m_ruleMode == RuleMode::ZERO_ONE) {
            Fraction goal(m_targetScore);
            if (m_p1ZeroOneScore == goal) return true;
            if (m_p2ZeroOneScore == goal) return false;
        }
        else {
            if (m_enemy && m_enemy->GetStocks() <= 0) return true;
        }
        return false;
    }
} // namespace App