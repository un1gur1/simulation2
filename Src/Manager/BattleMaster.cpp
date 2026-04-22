#define NOMINMAX
#include "BattleMaster.h"
#include <DxLib.h>
#include <cmath>
#include <algorithm>
#include "../Input/InputManager.h"
#include "../Scene/SceneManager.h" 
#include "../../CyberGrid.h" 

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ==========================================
// ★ マジックナンバーを排除するための定数・色定義
// ==========================================
namespace {
    constexpr int SCREEN_W = 1920;
    constexpr int SCREEN_H = 1080;
    constexpr int HEADER_H = 70;
    constexpr int BOTTOM_PANEL_Y = 830;
    constexpr int LOG_PANEL_Y = 760;

    inline unsigned int COL_BG() { return GetColor(10, 12, 18); }
    inline unsigned int COL_PANEL_BG() { return GetColor(18, 18, 22); }
    inline unsigned int COL_DARK_BG() { return GetColor(14, 16, 20); }
    inline unsigned int COL_BOTTOM_BG() { return GetColor(5, 5, 8); }
    inline unsigned int COL_TEXT_MAIN() { return GetColor(255, 255, 255); }
    inline unsigned int COL_TEXT_SUB() { return GetColor(220, 220, 220); }
    inline unsigned int COL_TEXT_DARK() { return GetColor(0, 0, 0); }

    inline unsigned int COL_P1() { return GetColor(255, 165, 0); }
    inline unsigned int COL_P2() { return GetColor(60, 150, 255); }

    inline unsigned int COL_DANGER() { return GetColor(255, 100, 100); }
    inline unsigned int COL_DANGER_DIM() { return GetColor(200, 50, 50); }
    inline unsigned int COL_SAFE() { return GetColor(100, 255, 150); }
    inline unsigned int COL_WARN() { return GetColor(255, 255, 0); }
    inline unsigned int COL_INFO() { return GetColor(200, 100, 255); }
    inline unsigned int COL_DISABLE() { return GetColor(150, 150, 150); }
}

namespace App {

    BattleMaster::BattleMaster()
        : m_currentPhase(Phase::P1_Move)
        , m_gameMode(GameMode::VS_CPU)
        , m_ruleMode(RuleMode::CLASSIC)
        , m_mapGrid(80, Vector2(600, 120))
        , m_p1ZeroOneScore(0, 1), m_p2ZeroOneScore(0, 1), m_targetScore(53)
        , m_isPlayerSelected(false)
        , m_hoverGrid(-1, -1)
        , m_enemyAIStarted(false)
        , m_playerAIStarted(false)
        , m_is1P_NPC(false)
        , m_is2P_NPC(true)
    {
    }

    BattleMaster::~BattleMaster() {
        if (m_psHandle != -1) DeleteShader(m_psHandle);
        if (m_cbHandle != -1) DeleteShaderConstantBuffer(m_cbHandle);
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
    UnitBase* BattleMaster::GetActiveUnit()const {
        return Is1PTurn() ? static_cast<UnitBase*>(m_player.get()) : static_cast<UnitBase*>(m_enemy.get());
    }

    UnitBase* BattleMaster::GetTargetUnit() const {
        return Is1PTurn() ? static_cast<UnitBase*>(m_enemy.get()) : static_cast<UnitBase*>(m_player.get());
    }

    void BattleMaster::Init() {
        auto* sm = SceneManager::GetInstance();
        m_gameMode = (sm->GetPlayerCount() == 1) ? GameMode::VS_CPU : GameMode::VS_PLAYER;
        m_ruleMode = (sm->GetGameMode() == 0) ? RuleMode::CLASSIC : RuleMode::ZERO_ONE;

        int maxStocks = sm->GetMaxStocks();
        m_targetScore = sm->GetZeroOneScore();
        m_p1ZeroOneScore = Fraction(0, 1);
        m_p2ZeroOneScore = Fraction(0, 1);

        m_is1P_NPC = sm->Is1PNPC();
        m_is2P_NPC = sm->Is2PNPC();
        IntVector2 p1StartPos{ sm->Get1PStartX(), sm->Get1PStartY() };
        IntVector2 p2StartPos{ sm->Get2PStartX(), sm->Get2PStartY() };

        BattleRuleMode mapMode = (m_ruleMode == RuleMode::CLASSIC) ? BattleRuleMode::CLASSIC : BattleRuleMode::ZERO_ONE;

        m_player = std::make_unique<Player>(p1StartPos, m_mapGrid.GetCellCenter(p1StartPos.x, p1StartPos.y), sm->Get1PStartNum(), maxStocks, maxStocks);
        m_enemy = std::make_unique<Enemy>(p2StartPos, m_mapGrid.GetCellCenter(p2StartPos.x, p2StartPos.y), sm->Get2PStartNum(), maxStocks, maxStocks);

        m_player->SetOp('\0');
        m_enemy->SetOp('\0');

        m_currentPhase = Phase::P1_Move;
        m_isPlayerSelected = false;
        m_enemyAIStarted = false;
        m_playerAIStarted = false;
        m_finishTimer = 0;

        m_startTime = GetNowCount();
        m_totalMoves = 0; m_totalOps = 0; m_maxDamage = 0;

        m_psHandle = LoadPixelShaderFromMem(g_ps_CyberGrid, sizeof(g_ps_CyberGrid));
        m_cbHandle = CreateShaderConstantBuffer(sizeof(float) * 4);
        m_shaderTime = 0.0f;
        m_effectIntensity = 0.0f;

        g_aiStayCount1P = 0;
        g_aiStayCount2P = 0;

        int stageIdx = sm->GetStageIndex();      
        m_mapGrid.SetRuleModeAndStage(mapMode, stageIdx);
        m_actionLog.clear();
        std::string rModeStr = (m_ruleMode == RuleMode::CLASSIC) ? "クラシック" : "ゼロワン";
        AddLog(">>> バトル開始！ [1P:" + std::string(m_is1P_NPC ? "COM" : "PLAYER") + " vs 2P:" + std::string(m_is2P_NPC ? "COM" : "PLAYER") + " / " + rModeStr + "]");

        if (m_ruleMode == RuleMode::ZERO_ONE) AddLog(">>> 目標：相手よりはやくスコアを【 " + std::to_string(m_targetScore) + " 】にしよう！");
        else AddLog(">>> 目標：相手の残機をゼロに！");
    }

    bool BattleMaster::CanMove(int number, char op, IntVector2 start, IntVector2 target, int& outCost) const {
        if (start == target) return false;

        UnitBase* u = nullptr;
        if (m_player && m_player->GetGridPos() == start) u = m_player.get();
        else if (m_enemy && m_enemy->GetGridPos() == start) u = m_enemy.get();

        if (u && u->HasWarpNode(target)) { outCost = 1; return true; }

        int dx = std::abs(target.x - start.x);
        int dy = std::abs(target.y - start.y);
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
        if (!input.IsMouseLeftTrg()) return;

        IntVector2 pos = activeUnit.GetGridPos();

        if (!m_isPlayerSelected) {
            if (m_hoverGrid == pos) m_isPlayerSelected = true;
            return;
        }

        if (m_hoverGrid == pos) {
            activeUnit.AddNumber(-1); // 現在地クリックでコスト1消費
            m_isPlayerSelected = false;
            m_currentPhase = nextPhase;
            return;
        }

        if (!m_mapGrid.IsWithinBounds(m_hoverGrid.x, m_hoverGrid.y)) {
            m_isPlayerSelected = false;
            return;
        }

        int cost = 0;
        if (!CanMove(activeUnit.GetNumber(), activeUnit.GetOp(), pos, m_hoverGrid, cost)) {
            m_isPlayerSelected = false;
            return;
        }

        activeUnit.AddNumber(-cost);
        std::queue<Vector2> autoPath;
        int dx = std::abs(m_hoverGrid.x - pos.x);
        int dy = std::abs(m_hoverGrid.y - pos.y);
        m_totalMoves += std::max(dx, dy);

        if (activeUnit.HasWarpNode(m_hoverGrid) || (dx != dy && dx != 0 && dy != 0)) {
            autoPath.push(m_mapGrid.GetCellCenter(m_hoverGrid.x, m_hoverGrid.y));
            AddLog("【跳躍】 転送完了。");
        }
        else {
            int stepX = (m_hoverGrid.x > pos.x) ? 1 : (m_hoverGrid.x < pos.x) ? -1 : 0;
            int stepY = (m_hoverGrid.y > pos.y) ? 1 : (m_hoverGrid.y < pos.y) ? -1 : 0;
            IntVector2 curr = pos;
            int maxSteps = std::max(dx, dy);
            for (int i = 0; i < maxSteps; ++i) {
                curr.x += stepX;
                curr.y += stepY;
                autoPath.push(m_mapGrid.GetCellCenter(curr.x, curr.y));
            }
        }

        activeUnit.StartMove(m_hoverGrid, autoPath);
        m_isPlayerSelected = false;
        m_currentPhase = nextPhase;
    }

    void BattleMaster::HandleActionInput(UnitBase& actor, UnitBase& targetUnit) {
        if (actor.IsMoving()) return;

        IntVector2 pos = actor.GetGridPos();
        char pickedItem = m_mapGrid.PickUpItem(pos.x, pos.y);
        if (pickedItem != '\0') {
            actor.SetOp(pickedItem);
            std::string name = (&actor == m_player.get()) ? "1P" : "2P";
            AddLog("【取得】 " + name + "が [" + std::string(1, pickedItem) + "] を取得！");
        }

        IntVector2 targetPos = targetUnit.GetGridPos();
        bool canAttack = (std::abs(pos.x - targetPos.x) + std::abs(pos.y - targetPos.y) == 1);
        bool hasOp = (actor.GetOp() != '\0');
        Phase nextTurnPhase = (&actor == m_player.get()) ? Phase::P2_Move : Phase::P1_Move;

        if (!canAttack || !hasOp) {
            AddLog("【待機】 行動を終了します");
            m_currentPhase = nextTurnPhase;
            if (&actor != m_player.get()) m_mapGrid.UpdateTurn();
            return;
        }

        auto& input = InputManager::GetInstance();
        if (!input.IsMouseLeftTrg()) return;

        Vector2 mousePos = input.GetMousePos();
        IntVector2 hoverGrid = m_mapGrid.ScreenToGrid(mousePos);
        bool is1P = (&actor == m_player.get());
        bool actionDone = false;

        if (CheckButtonClick(600, 960, 220, 60, mousePos) || hoverGrid == pos || (CheckButtonClick(40, 100, 500, 650, mousePos) && is1P) || (CheckButtonClick(1380, 100, 500, 650, mousePos) && !is1P)) {
            ExecuteBattle(actor, targetUnit, actor); actionDone = true;
        }
        else if (CheckButtonClick(850, 960, 220, 60, mousePos) || hoverGrid == targetPos || (CheckButtonClick(40, 100, 500, 650, mousePos) && !is1P) || (CheckButtonClick(1380, 100, 500, 650, mousePos) && is1P)) {
            ExecuteBattle(actor, targetUnit, targetUnit); actionDone = true;
        }
        else if (CheckButtonClick(1100, 960, 220, 60, mousePos)) {
            AddLog("【待機】 行動を終了します"); actionDone = true;
        }

        if (actionDone) {
            m_currentPhase = nextTurnPhase;
            if (!is1P) m_mapGrid.UpdateTurn();
        }
    }

    void BattleMaster::ExecuteAIAction(UnitBase* me, UnitBase* opp, bool is1P) {
        if (!me || !opp || me->IsMoving()) return;

        std::string myName = is1P ? "1P" : "2P";
        IntVector2 myP = me->GetGridPos();
        char pickedItem = m_mapGrid.PickUpItem(myP.x, myP.y);

        if (pickedItem != '\0') {
            me->SetOp(pickedItem);
            AddLog("【取得】 " + myName + " が [" + std::string(1, pickedItem) + "] を取得");
        }

        IntVector2 oppP = opp->GetGridPos();
        bool canAttack = (std::abs(myP.x - oppP.x) + std::abs(myP.y - oppP.y) == 1);
        char myOp = me->GetOp();

        if (canAttack && myOp != '\0') {
            int aNum = me->GetNumber();
            int tNum = opp->GetNumber();
            int intRes = 0; Fraction resFrac(0); bool isCleanDivide = true;

            if (myOp == '+') { intRes = aNum + tNum; resFrac = Fraction(intRes); }
            else if (myOp == '-') { intRes = aNum - tNum; resFrac = Fraction(intRes); }
            else if (myOp == '*') { intRes = aNum * tNum; resFrac = Fraction(intRes); }
            else if (myOp == '/') {
                if (tNum != 0 && aNum % tNum == 0) { intRes = aNum / tNum; resFrac = Fraction(intRes); }
                else { intRes = 0; resFrac = Fraction(0); isCleanDivide = false; }
            }

            bool targetMeIsBetter = true;

            if (m_ruleMode == RuleMode::ZERO_ONE) {
                if (myOp == '/') {
                    targetMeIsBetter = true;
                }
                else {
                    Fraction goal(m_targetScore);
                    Fraction myScoreNow = is1P ? m_p1ZeroOneScore : m_p2ZeroOneScore;
                    Fraction enScoreNow = is1P ? m_p2ZeroOneScore : m_p1ZeroOneScore;

                    // ★ バウンスも含めた完全な未来予測
                    Fraction nextMy = myScoreNow + resFrac;
                    if (nextMy > goal) nextMy = goal - (nextMy - goal);

                    Fraction nextEn = enScoreNow + resFrac;
                    if (nextEn > goal) nextEn = goal - (nextEn - goal);

                    long long myDistNow = std::abs((goal - myScoreNow).n / (goal - myScoreNow).d);
                    long long enDistNow = std::abs((goal - enScoreNow).n / (goal - enScoreNow).d);
                    long long myDistNext = std::abs((goal - nextMy).n / (goal - nextMy).d);
                    long long enDistNext = std::abs((goal - nextEn).n / (goal - nextEn).d);

                    int scoreMe = (myDistNow - myDistNext) * 100;
                    if (nextMy == goal) scoreMe = 1000000;

                    int scoreEn = (enDistNext - enDistNow) * 100;
                    if (nextEn == goal) scoreEn = -1000000;

                    targetMeIsBetter = (scoreMe >= scoreEn);
                }
            }
            else { // CLASSIC
                if (myOp == '/') {
                    targetMeIsBetter = true;
                }
                else {
                    auto calcDmg = [](int currentHp, int currentStocks, int val, int& outHp, int& outStocks) {
                        outHp = currentHp + val; outStocks = currentStocks;
                        while (outHp <= 0) { outStocks--; outHp += 9; }
                        while (outHp > 9) { outStocks++; outHp -= 9; }
                        };

                    int myHpNext, myStNext, enHpNext, enStNext;
                    calcDmg(aNum, me->GetStocks(), intRes, myHpNext, myStNext);
                    calcDmg(tNum, opp->GetStocks(), intRes, enHpNext, enStNext);

                    int scoreMe = (myStNext - me->GetStocks()) * 10000 + (myHpNext - aNum) * 1000;
                    if (myStNext <= 0 && myHpNext <= 0) scoreMe = -1000000;

                    int scoreEn = (opp->GetStocks() - enStNext) * 15000 + (tNum - enHpNext) * 500;
                    if (enStNext <= 0 && enHpNext <= 0) scoreEn = 10000000;

                    targetMeIsBetter = (scoreMe >= scoreEn);
                }
            }

            if (targetMeIsBetter) ExecuteBattle(*me, *opp, *me);
            else ExecuteBattle(*me, *opp, *opp);
        }
        else {
            AddLog("【待機】 " + myName + " は行動を完了しました");
        }

        if (!is1P) m_mapGrid.UpdateTurn();
        m_currentPhase = is1P ? Phase::P2_Move : Phase::P1_Move;
        if (is1P) m_playerAIStarted = false; else m_enemyAIStarted = false;
    }

    void BattleMaster::Update() {
        if (m_currentPhase == Phase::FINISH) {
            m_finishTimer++;
            if (m_finishTimer > 60) {
                auto& input = InputManager::GetInstance();
                if (input.IsMouseLeftTrg() || input.IsTrgDown(KEY_INPUT_SPACE) || input.IsTrgDown(KEY_INPUT_RETURN)) {
                    int finalTimeMs = GetNowCount() - m_startTime;
                    BattleStats stats = { m_mapGrid.GetTotalTurns(), m_totalMoves, m_totalOps, finalTimeMs, m_maxDamage };
                    auto* sm = SceneManager::GetInstance();
                    sm->SetBattleResult(IsPlayerWin(), stats);
                    sm->ChangeScene(SceneManager::SCENE_ID::RESULT);
                }
            }
            return;
        }

        if (m_ruleMode == RuleMode::ZERO_ONE) {
            if (m_player && m_player->GetStocks() < m_player->GetMaxStocks()) m_player->AddStocks(m_player->GetMaxStocks() - m_player->GetStocks());
            if (m_enemy && m_enemy->GetStocks() < m_enemy->GetMaxStocks()) m_enemy->AddStocks(m_enemy->GetMaxStocks() - m_enemy->GetStocks());
        }

        auto& input = InputManager::GetInstance();
        if (m_player) m_player->Update();
        if (m_enemy)  m_enemy->Update();
        m_hoverGrid = m_mapGrid.ScreenToGrid(input.GetMousePos());

        m_shaderTime += 0.0016f + (0.01f * m_effectIntensity);
        if (m_effectIntensity > 0.0f) m_effectIntensity -= 0.05f;

        switch (m_currentPhase) {
        case Phase::P1_Move:
            if (m_is1P_NPC) {
                if (!m_playerAIStarted) { ExecuteAI(m_player.get(), m_enemy.get(), true); m_playerAIStarted = true; }
                else if (m_player && !m_player->IsMoving()) m_currentPhase = Phase::P1_Action;
            }
            else HandleMoveInput(*m_player, Phase::P1_Action);
            break;
        case Phase::P1_Action:
            if (m_is1P_NPC) ExecuteAIAction(m_player.get(), m_enemy.get(), true);
            else HandleActionInput(*m_player, *m_enemy);
            break;
        case Phase::P2_Move:
            if (m_is2P_NPC) {
                if (!m_enemyAIStarted) { ExecuteAI(m_enemy.get(), m_player.get(), false); m_enemyAIStarted = true; }
                else if (m_enemy && !m_enemy->IsMoving()) m_currentPhase = Phase::P2_Action;
            }
            else HandleMoveInput(*m_enemy, Phase::P2_Action);
            break;
        case Phase::P2_Action:
            if (m_is2P_NPC) ExecuteAIAction(m_enemy.get(), m_player.get(), false);
            else HandleActionInput(*m_enemy, *m_player);
            break;
        }

        if (IsGameOver() && m_currentPhase != Phase::FINISH) {
            m_currentPhase = Phase::FINISH;
            m_finishTimer = 0;
            m_effectIntensity = 1.0f;
        }
    }

    void BattleMaster::PerformAIMove(UnitBase* me, IntVector2 bestTarget, int selectedCost, bool is1P) {
        if (!me) return;
        IntVector2 myPos = me->GetGridPos();
        std::queue<Vector2> screenPath;
        std::string myName = is1P ? "1P" : "2P";

        bool isStay = (bestTarget == myPos);
        if (isStay && !me->HasWarpNode(bestTarget)) {
            if (is1P) g_aiStayCount1P++; else g_aiStayCount2P++;
        }
        else {
            if (is1P) g_aiStayCount1P = 0; else g_aiStayCount2P = 0;
        }

        // その場待機ならコストは1。それ以外は移動コスト。
        int actualCost = isStay ? 1 : selectedCost;
        if (me->HasWarpNode(bestTarget)) actualCost = 1;

        if (me->HasWarpNode(bestTarget)) AddLog("【転送】 " + myName + " がワープを起動 (-1)");
        else if (isStay) AddLog("【待機】 " + myName + " はその場でリソースを消費 (-1)");
        else AddLog("【進軍】 " + myName + " が移動を開始 (-" + std::to_string(actualCost) + ")");

        int dx = std::abs(bestTarget.x - myPos.x);
        int dy = std::abs(bestTarget.y - myPos.y);
        m_totalMoves += std::max(dx, dy);

        if (me->HasWarpNode(bestTarget) || isStay || (dx != dy && dx != 0 && dy != 0)) {
            screenPath.push(m_mapGrid.GetCellCenter(bestTarget.x, bestTarget.y));
        }
        else {
            int stepX = (bestTarget.x > myPos.x) ? 1 : (bestTarget.x < myPos.x) ? -1 : 0;
            int stepY = (bestTarget.y > myPos.y) ? 1 : (bestTarget.y < myPos.y) ? -1 : 0;
            IntVector2 curr = myPos;
            int maxSteps = std::max(dx, dy);
            for (int i = 0; i < maxSteps; ++i) {
                curr.x += stepX;
                curr.y += stepY;
                screenPath.push(m_mapGrid.GetCellCenter(curr.x, curr.y));
            }
        }
        // 確実に数値を減らす
        me->AddNumber(-actualCost);
        me->StartMove(bestTarget, screenPath);

        if (is1P) m_playerAIStarted = true; else m_enemyAIStarted = true;
    }

    
    int BattleMaster::EvaluateBoard(const UnitBase& me, int myVirtualNumber, const UnitBase& enemy, IntVector2 targetPos, bool is1P) const {
        int score = 0;
        IntVector2 currentPos = me.GetGridPos();
        bool isStay = (targetPos == currentPos);
        IntVector2 ePos = enemy.GetGridPos();
        bool canAttack = (std::abs(targetPos.x - ePos.x) + std::abs(targetPos.y - ePos.y) == 1);

        int moveCost = isStay ? 1 : std::max(std::abs(targetPos.x - currentPos.x), std::abs(targetPos.y - currentPos.y));
        if (me.HasWarpNode(targetPos)) moveCost = 1;

        int predictedStocks = me.GetStocks();
        int predictedHp = me.GetNumber() - moveCost;
        while (predictedHp <= 0) {
            predictedStocks--;
            predictedHp += 9;
        }

        if (predictedStocks <= 0 && predictedHp <= 0) {
            score -= 9000000;
        }

        int currentStayCount = is1P ? g_aiStayCount1P : g_aiStayCount2P;
        if (isStay) {
            score -= (currentStayCount * 20000);
            if (!canAttack) score -= 2000;
        }
        if (targetPos.x == 0 || targetPos.x == 8 || targetPos.y == 0 || targetPos.y == 8) score -= 50;

        char virtualOp = me.GetOp();
        char itemHere = m_mapGrid.GetItemAt(targetPos.x, targetPos.y);
        if (itemHere != '\0') virtualOp = itemHere;

        if (m_ruleMode == RuleMode::ZERO_ONE) {
            Fraction goal(m_targetScore);
            Fraction myScoreNow = is1P ? m_p1ZeroOneScore : m_p2ZeroOneScore;
            Fraction enScoreNow = is1P ? m_p2ZeroOneScore : m_p1ZeroOneScore;
            long long myDistNow = std::abs((goal - myScoreNow).n / (goal - myScoreNow).d);
            long long enDistNow = std::abs((goal - enScoreNow).n / (goal - enScoreNow).d);

            if (me.GetOp() == '\0') {
                int bestItemScore = -99999;
                for (int ix = 0; ix < 9; ++ix) {
                    for (int iy = 0; iy < 9; ++iy) {
                        char item = m_mapGrid.GetItemAt(ix, iy);
                        if (item != '\0') {
                            int distToItem = std::abs(targetPos.x - ix) + std::abs(targetPos.y - iy);
                            int itemVal = (20 - distToItem) * 300;

                            int eNum = enemy.GetNumber();
                            int hypRes = 0; bool isClean = true;
                            if (item == '+') hypRes = predictedHp + eNum;
                            else if (item == '-') hypRes = predictedHp - eNum;
                            else if (item == '*') hypRes = predictedHp * eNum;
                            else if (item == '/') {
                                if (eNum != 0 && predictedHp % eNum == 0) hypRes = predictedHp / eNum;
                                else isClean = false;
                            }

                            if (item == '/') {
                                if (isClean) itemVal += 1500;
                            }
                            else {
                                Fraction resF(hypRes);
                                Fraction nextMy = myScoreNow + resF;
                                if (nextMy > goal) nextMy = goal - (nextMy - goal);
                                long long myDistNext = std::abs((goal - nextMy).n / (goal - nextMy).d);
                                int benefitMe = (int)(myDistNow - myDistNext) * 200;
                                if (nextMy == goal) benefitMe = 100000;

                                Fraction nextEn = enScoreNow + resF;
                                if (nextEn > goal) nextEn = goal - (nextEn - goal);
                                long long enDistNext = std::abs((goal - nextEn).n / (goal - nextEn).d);
                                int benefitEn = (int)(enDistNext - enDistNow) * 200;
                                if (nextEn == goal) benefitEn = -100000;

                                itemVal += std::max(benefitMe, benefitEn);
                            }

                            if (itemVal > bestItemScore) bestItemScore = itemVal;
                        }
                    }
                }
                score += (bestItemScore != -99999) ? bestItemScore : 0;
            }

            if (virtualOp != '\0') {
                int distToEnemy = std::abs(targetPos.x - ePos.x) + std::abs(targetPos.y - ePos.y);
                score += (20 - distToEnemy) * 500;

                if (canAttack) {
                    int eNum = enemy.GetNumber();
                    int res = 0;
                    if (virtualOp == '+')      res = predictedHp + eNum;
                    else if (virtualOp == '-') res = predictedHp - eNum;
                    else if (virtualOp == '*') res = predictedHp * eNum;
                    else if (virtualOp == '/') res = (eNum != 0 && predictedHp % eNum == 0) ? predictedHp / eNum : 0;
                    Fraction resF(res);

                    Fraction nextMy = myScoreNow + resF; if (nextMy > goal) nextMy = goal - (nextMy - goal);
                    Fraction nextEn = enScoreNow + resF; if (nextEn > goal) nextEn = goal - (nextEn - goal);

                    long long myDistNext = std::abs((goal - nextMy).n / (goal - nextMy).d);
                    long long enDistNext = std::abs((goal - nextEn).n / (goal - nextEn).d);

                    int gainMe = (int)(myDistNow - myDistNext) * 200;
                    if (nextMy == goal) gainMe = 1000000;

                    int gainEn = (int)(enDistNext - enDistNow) * 200;
                    if (nextEn == goal) gainEn = -1000000;

                    score += std::max(gainMe, gainEn);
                }
            }
            if (canAttack && virtualOp == '\0' && enemy.GetOp() != '\0') score -= 5000;
        }
        else {
            // クラシックモード
            score += predictedStocks * 20000;
            score -= enemy.GetStocks() * 20000;
            score += predictedHp * 100;

            // ★ 修正：現在アイテムを持っていないなら「アイテム探し」を優先
            if (me.GetOp() == '\0') {
                int bestItemScore = -99999;
                for (int ix = 0; ix < 9; ++ix) {
                    for (int iy = 0; iy < 9; ++iy) {
                        char item = m_mapGrid.GetItemAt(ix, iy);
                        if (item != '\0') {
                            int distToItem = std::abs(targetPos.x - ix) + std::abs(targetPos.y - iy);
                            int itemVal = (20 - distToItem) * 500;
                            if (item == '-') itemVal += 5000;
                            else if (item == '*') itemVal += 3000;

                            if (predictedStocks == 0 && predictedHp <= 3 && item == '+') itemVal += 8000;

                            if (itemVal > bestItemScore) bestItemScore = itemVal;
                        }
                    }
                }
                score += (bestItemScore != -99999) ? bestItemScore : 0;
            }

            if (virtualOp != '\0') {
                int distToEnemy = std::abs(targetPos.x - ePos.x) + std::abs(targetPos.y - ePos.y);
                score += (20 - distToEnemy) * 1000;

                if (canAttack) {
                    int eNum = enemy.GetNumber();
                    int intRes = 0; bool isClean = true;
                    if (virtualOp == '+') intRes = predictedHp + eNum;
                    else if (virtualOp == '-') intRes = predictedHp - eNum;
                    else if (virtualOp == '*') intRes = predictedHp * eNum;
                    else if (virtualOp == '/') {
                        if (eNum != 0 && predictedHp % eNum == 0) intRes = predictedHp / eNum;
                        else isClean = false;
                    }

                    if (virtualOp == '/') {
                        if (isClean) score += 2000; else score -= 1000;
                    }
                    else {
                        auto calcDmg = [](int currentHp, int currentStocks, int val, int& outHp, int& outStocks) {
                            outHp = currentHp + val; outStocks = currentStocks;
                            while (outHp <= 0) { outStocks--; outHp += 9; }
                            while (outHp > 9) { outStocks++; outHp -= 9; }
                            };

                        int myHpNext, myStNext, enHpNext, enStNext;
                        calcDmg(predictedHp, predictedStocks, intRes, myHpNext, myStNext);
                        calcDmg(eNum, enemy.GetStocks(), intRes, enHpNext, enStNext);

                        int scoreMe = (myStNext - predictedStocks) * 10000 + (myHpNext - predictedHp) * 1000;
                        if (myStNext <= 0 && myHpNext <= 0) scoreMe -= 5000000;

                        int scoreEn = (enemy.GetStocks() - enStNext) * 15000 + (eNum - enHpNext) * 500;
                        if (enStNext <= 0 && enHpNext <= 0) scoreEn += 10000000;

                        score += std::max(scoreMe, scoreEn);
                    }
                }
            }
            if (canAttack && virtualOp == '\0' && enemy.GetOp() != '\0') score -= 5000;
        }

        return score;
    }
    void BattleMaster::ExecuteAI(UnitBase* me, UnitBase* opp, bool is1P) {
        if (!me) return;

        IntVector2 myPos = me->GetGridPos();
        int currentNum = me->GetNumber();
        char currentOp = me->GetOp();

        IntVector2 bestMove = myPos;
        int bestScore = -9999999;
        int selectedCost = 1;

        for (int x = 0; x < 9; ++x) {
            for (int y = 0; y < 9; ++y) {
                IntVector2 target{ x, y };
                int cost = 0;

                bool canGo = (target == myPos);
                if (!canGo) {
                    canGo = CanMove(currentNum, currentOp, myPos, target, cost);
                }
                else {
                    cost = 1; // 待機はコスト1
                }

                if (canGo) {
                    int virtualNextNum = currentNum - cost;
                    while (virtualNextNum <= 0) virtualNextNum += 9;

                    me->StartMove(target, std::queue<Vector2>());
                    int eval = EvaluateBoard(*me, virtualNextNum, *opp, target, is1P);

                    if (eval > bestScore) {
                        bestScore = eval;
                        bestMove = target;
                        selectedCost = cost;
                    }
                    me->StartMove(myPos, std::queue<Vector2>());
                }
            }
        }
        PerformAIMove(me, bestMove, selectedCost, is1P);
    }

    void BattleMaster::ExecuteBattle(UnitBase& attacker, UnitBase& defender, UnitBase& target) {
        char aOp = attacker.GetOp();
        int aNum = attacker.GetNumber();
        int dNum = defender.GetNumber();

        m_effectIntensity = 2.0f;

        if (aOp == '/' && dNum != 0) {
            int wx = aNum - 1;
            int wy = 9 - dNum;
            IntVector2 nodePos{ wx, wy };

            if (!target.HasWarpNode(nodePos)) {
                target.AddWarpNode(nodePos);
                std::string targetName = (&target == m_player.get()) ? "1P" : "2P";
                AddLog("【ワープ】 " + targetName + " が座標 (" + std::to_string(aNum) + ", " + std::to_string(dNum) + ") にワープ設置！");
            }
        }

        Fraction resFrac(0);
        int intRes = 0;
        bool isCleanDivide = true;

        if (aOp == '+') { intRes = aNum + dNum; resFrac = Fraction(intRes); }
        else if (aOp == '-') { intRes = aNum - dNum; resFrac = Fraction(intRes); }
        else if (aOp == '*') { intRes = aNum * dNum; resFrac = Fraction(intRes); }
        else if (aOp == '/') {
            if (dNum != 0 && aNum % dNum == 0) {
                intRes = aNum / dNum;
                resFrac = Fraction(intRes);
            }
            else {
                intRes = 0;
                resFrac = Fraction(0);
                isCleanDivide = false;
            }
        }

        std::string aName = (&attacker == m_player.get()) ? "1P" : "2P";
        if (aOp != '/') AddLog(">> " + aName + " の計算！ [ 値: " + std::to_string(intRes) + " ]");

        m_totalOps++;
        if (std::abs(intRes) > m_maxDamage) m_maxDamage = std::abs(intRes);

        ApplyBattleResult(target, resFrac, intRes, aOp);
        attacker.SetOp('\0');
    }

    void BattleMaster::ApplyBattleResult(UnitBase& unit, const Fraction& resultFrac, int intRes, char op) {
        std::string name = (&unit == m_player.get()) ? "1P" : "2P";

        if (m_ruleMode == RuleMode::ZERO_ONE) {
            Fraction& currentScore = (&unit == m_player.get()) ? m_p1ZeroOneScore : m_p2ZeroOneScore;
            Fraction goalScore(m_targetScore);

            if (op != '/') {
                Fraction predictedScore = currentScore + resultFrac;
                if (predictedScore == goalScore) {
                    currentScore = predictedScore;
                    AddLog("【FINISH!!】 " + name + " が目標スコアにピッタリ到達した！");
                }
                else if (predictedScore > goalScore) {
                    Fraction excess = predictedScore - goalScore;
                    currentScore = goalScore - excess;
                    AddLog("【COUNT BACK】 " + name + " が目標を超過！ [" + currentScore.ToString() + "] まで戻りました。");
                }
                else {
                    currentScore = predictedScore;
                    AddLog("【HIT】 " + name + " のスコアが [" + currentScore.ToString() + "] に変化。");
                }
            }
            else {
                AddLog("【NODE】 スコア加算なし。");
            }

            int cycleValue = (intRes - 1) % 9;
            if (cycleValue < 0) cycleValue += 9;
            unit.SetNumber(cycleValue + 1);
        }
        else {
            if (op != '/') {
                int currentHp = unit.GetNumber();
                int newHpRaw = currentHp + intRes;
                int stockChange = 0;

                while (newHpRaw <= 0) { stockChange--; newHpRaw += 9; }
                while (newHpRaw > 9) { stockChange++; newHpRaw -= 9; }

                if (stockChange < 0) {
                    unit.AddStocks(stockChange);
                    AddLog("【ダメージ】 " + name + " の残機 " + std::to_string(stockChange) + "！");
                }
                else if (stockChange > 0) {
                    unit.AddStocks(stockChange);
                    AddLog("【回復】 " + name + " の残機 +" + std::to_string(stockChange) + "！");
                }
                else {
                    AddLog("【反映】 " + name + " の数値にダメージ/回復を適用");
                }
                unit.SetNumber(newHpRaw);
                AddLog("【結果】 " + name + " の体力は [" + std::to_string(newHpRaw) + "] になった。");
            }
        }
    }

    void BattleMaster::DrawEnemyDangerArea() const {
        UnitBase* opp = GetTargetUnit();
        if (!opp || opp->GetStocks() <= 0 || opp->IsMoving()) return;

        IntVector2 ePos = opp->GetGridPos();
        int eNum = opp->GetNumber();
        char eOp = opp->GetOp();

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 80);
        for (int x = 0; x < m_mapGrid.GetWidth(); ++x) {
            for (int y = 0; y < m_mapGrid.GetHeight(); ++y) {
                IntVector2 target{ x, y };
                Vector2 center = m_mapGrid.GetCellCenter(x, y);

                if (target == ePos) {
                    DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, COL_DANGER(), TRUE);
                    continue;
                }

                int dummyCost = 0;
                bool canMoveBase = CanMove(eNum, '\0', ePos, target, dummyCost);
                bool canMoveCombined = CanMove(eNum, eOp, ePos, target, dummyCost);

                if (canMoveCombined) {
                    unsigned int areaCol = canMoveBase ? COL_DANGER_DIM() : COL_INFO();
                    DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, areaCol, TRUE);
                }
            }
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    void BattleMaster::DrawMovableArea() const {
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
                    DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, COL_WARN(), TRUE);
                    continue;
                }
                int dummyCost = 0;
                bool canMoveBase = CanMove(pNum, '\0', pPos, target, dummyCost);
                bool canMoveCombined = CanMove(pNum, pOp, pPos, target, dummyCost);
                bool isWarpNode = activeUnit->HasWarpNode(target);

                if (canMoveCombined || isWarpNode) {
                    unsigned int col = isWarpNode ? COL_INFO() : (canMoveBase ? GetColor(100, 200, 255) : GetColor(255, 180, 50));
                    DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, col, TRUE);
                }
            }
        }
        if (m_mapGrid.IsWithinBounds(m_hoverGrid.x, m_hoverGrid.y)) {
            int hoverCost = 0;
            bool isHoverSelf = (m_hoverGrid == pPos);
            if (isHoverSelf || CanMove(pNum, pOp, pPos, m_hoverGrid, hoverCost)) {
                Vector2 hc = m_mapGrid.GetCellCenter(m_hoverGrid.x, m_hoverGrid.y);
                DrawBox((int)hc.x - 35, (int)hc.y - 35, (int)hc.x + 35, (int)hc.y + 35, COL_TEXT_MAIN(), FALSE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                int displayCost = isHoverSelf ? 1 : hoverCost;
                DrawFormatString((int)hc.x - 10, (int)hc.y - 20, COL_DANGER(), "-%d", displayCost);
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
            }
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    void BattleMaster::Draw() const {
        if (m_psHandle != -1 && m_cbHandle != -1) {
            float* cb = (float*)GetBufferShaderConstantBuffer(m_cbHandle);
            cb[0] = m_shaderTime;
            cb[1] = SCREEN_W;
            cb[2] = SCREEN_H;
            cb[3] = 0.0f;
            UpdateShaderConstantBuffer(m_cbHandle);
            SetShaderConstantBuffer(m_cbHandle, DX_SHADERTYPE_PIXEL, 0);

            SetUsePixelShader(m_psHandle);
            VERTEX2DSHADER v[6];
            for (int i = 0; i < 6; ++i) {
                v[i].pos = VGet(0, 0, 0);
                v[i].rhw = 1.0f;
                v[i].dif = GetColorU8(255, 255, 255, 255);
                v[i].spc = GetColorU8(0, 0, 0, 0);
                v[i].u = 0.0f; v[i].v = 0.0f;
            }
            v[0].pos.x = 0;        v[0].pos.y = 0;        v[0].u = 0.0f; v[0].v = 0.0f;
            v[1].pos.x = SCREEN_W; v[1].pos.y = 0;        v[1].u = 1.0f; v[1].v = 0.0f;
            v[2].pos.x = 0;        v[2].pos.y = SCREEN_H; v[2].u = 0.0f; v[2].v = 1.0f;
            v[3].pos.x = SCREEN_W; v[3].pos.y = 0;        v[3].u = 1.0f; v[3].v = 0.0f;
            v[4].pos.x = SCREEN_W; v[4].pos.y = SCREEN_H; v[4].u = 1.0f; v[4].v = 1.0f;
            v[5].pos.x = 0;        v[5].pos.y = SCREEN_H; v[5].u = 0.0f; v[5].v = 1.0f;
            DrawPrimitive2DToShader(v, 6, DX_PRIMTYPE_TRIANGLELIST);
            SetUsePixelShader(-1);
        }
        else {
            DrawBox(0, 0, SCREEN_W, SCREEN_H, COL_BG(), TRUE);
        }

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
        DrawBox(0, HEADER_H, 580, SCREEN_H, COL_PANEL_BG(), TRUE);
        DrawBox(576, HEADER_H, 580, SCREEN_H, COL_P1(), TRUE);
        DrawLine(575, HEADER_H, 575, SCREEN_H, GetColor(255, 220, 100), 1);

        DrawBox(1340, HEADER_H, SCREEN_W, SCREEN_H, COL_PANEL_BG(), TRUE);
        DrawBox(1340, HEADER_H, 1344, SCREEN_H, GetColor(30, 50, 180), TRUE);
        DrawLine(1344, HEADER_H, 1344, SCREEN_H, GetColor(100, 150, 255), 1);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        m_mapGrid.Draw();

        auto drawWarpNodes = [&](UnitBase* unit, unsigned int baseCol, const char* label) {
            if (!unit) return;
            for (auto pos : unit->GetWarpNodes()) {
                Vector2 center = m_mapGrid.GetCellCenter(pos.x, pos.y);
                SetDrawBlendMode(DX_BLENDMODE_ADD, 150);
                DrawCircleAA(center.x, center.y, 42.0f, 64, baseCol, FALSE, 3.0f);
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 80);
                DrawCircleAA(center.x, center.y, 35.0f, 64, baseCol, TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

                SetFontSize(18);
                int tw = GetDrawStringWidth(label, (int)strlen(label));
                DrawString((int)center.x - tw / 2, (int)center.y + 15, label, COL_TEXT_MAIN());
            }
            };

        if (Is1PTurn()) drawWarpNodes(m_player.get(), COL_P1(), "1P ST.");
        else drawWarpNodes(m_enemy.get(), COL_P2(), "2P ST.");

        DrawEnemyDangerArea();
        DrawMovableArea();

        UnitBase* blinkUnit = GetActiveUnit();
        if (blinkUnit && !blinkUnit->IsMoving()) {
            IntVector2 bPos = blinkUnit->GetGridPos();
            Vector2 bCenter = m_mapGrid.GetCellCenter(bPos.x, bPos.y);

            double time = GetNowCount() / 1000.0;
            float bobbing = (blinkUnit == m_player.get()) ? (float)(sin(time * 2.5) * 5.0) : (float)(sin(time * 2.0) * 4.0);
            bCenter.y += bobbing;

            int alpha = (int)(150 + 105 * sin(time * M_PI));
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
            unsigned int auraCol = (blinkUnit == m_player.get()) ? COL_WARN() : COL_P2();

            for (int i = 0; i < 3; ++i) {
                DrawBox((int)bCenter.x - 39 + i, (int)bCenter.y - 39 + i, (int)bCenter.x + 39 - i, (int)bCenter.y + 39 - i, auraCol, FALSE);
            }
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha / 4);
            DrawBox((int)bCenter.x - 36, (int)bCenter.y - 36, (int)bCenter.x + 36, (int)bCenter.y + 36, auraCol, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        if (m_player) m_player->Draw();
        if (m_enemy)  m_enemy->Draw();

        unsigned int phaseCol;
        const char* phaseName;
        if (m_currentPhase == Phase::P1_Move) { phaseCol = GetColor(180, 110, 0); phaseName = m_is1P_NPC ? "1Pのターン (思考中)" : "1Pのターン (移動選択)"; }
        else if (m_currentPhase == Phase::P1_Action) { phaseCol = GetColor(180, 110, 0); phaseName = m_is1P_NPC ? "1Pのターン (思考中)" : "1Pのターン (行動選択)"; }
        else if (m_currentPhase == Phase::P2_Move) { phaseCol = GetColor(30, 50, 120); phaseName = m_is2P_NPC ? "2Pのターン (思考中)" : "2Pのターン (移動選択)"; }
        else if (m_currentPhase == Phase::P2_Action) { phaseCol = GetColor(30, 50, 120); phaseName = m_is2P_NPC ? "2Pのターン (思考中)" : "2Pのターン (行動選択)"; }
        else { phaseCol = COL_INFO(); phaseName = "GAME SET!!"; }

        DrawBox(0, 0, SCREEN_W, HEADER_H, phaseCol, TRUE);
        DrawLine(0, HEADER_H, SCREEN_W, HEADER_H, COL_TEXT_MAIN(), 2);

        SetFontSize(38);
        DrawFormatString(40, 16, COL_TEXT_MAIN(), ">>> %s", phaseName);
        SetFontSize(24);
        DrawFormatString(800, 24, COL_TEXT_SUB(), "経過ターン: %d", m_mapGrid.GetTotalTurns());

        Vector2 mousePos = InputManager::GetInstance().GetMousePos();
        UnitBase* activeActor = GetActiveUnit();
        UnitBase* activeTarget = GetTargetUnit();

        bool isMovePreview = false;
        int previewCost = 0;
        int previewNum = activeActor ? activeActor->GetNumber() : 0;
        bool isPreviewNextToEnemy = false;
        char previewOp = activeActor ? activeActor->GetOp() : '\0';

        if (m_isPlayerSelected && activeActor && !activeActor->IsMoving() &&
            (m_currentPhase == Phase::P1_Move || m_currentPhase == Phase::P2_Move)) {

            if (m_mapGrid.IsWithinBounds(m_hoverGrid.x, m_hoverGrid.y)) {
                if (m_hoverGrid == activeActor->GetGridPos()) {
                    isMovePreview = true;
                    previewCost = 1;
                }
                else if (CanMove(activeActor->GetNumber(), activeActor->GetOp(), activeActor->GetGridPos(), m_hoverGrid, previewCost)) {
                    isMovePreview = true;
                }

                if (isMovePreview) {
                    previewNum = activeActor->GetNumber() - previewCost;
                    while (previewNum <= 0) previewNum += 9;

                    char itemOnGrid = m_mapGrid.GetItemAt(m_hoverGrid.x, m_hoverGrid.y);
                    if (itemOnGrid != '\0') previewOp = itemOnGrid;

                    if (activeTarget) {
                        IntVector2 tP = activeTarget->GetGridPos();
                        if (std::abs(m_hoverGrid.x - tP.x) + std::abs(m_hoverGrid.y - tP.y) == 1) {
                            if (previewOp != '\0') isPreviewNextToEnemy = true;
                        }
                    }
                }
            }
        }

        IntVector2 aP = activeActor ? activeActor->GetGridPos() : IntVector2{ -1,-1 };
        IntVector2 tP = activeTarget ? activeTarget->GetGridPos() : IntVector2{ -1,-1 };
        bool canAttack = activeTarget && (std::abs(aP.x - tP.x) + std::abs(aP.y - tP.y) == 1);
        bool hasOp = (activeActor && activeActor->GetOp() != '\0');

        bool isHoverSelf = false;
        bool isHoverEnemy = false;

        if (canAttack && hasOp) {
            bool is1PTurn = Is1PTurn();
            if (CheckButtonClick(600, 960, 220, 60, mousePos)) isHoverSelf = true;
            else if (CheckButtonClick(850, 960, 220, 60, mousePos)) isHoverEnemy = true;
            else if (CheckButtonClick(40, 100, 500, 650, mousePos)) {
                if (is1PTurn) isHoverSelf = true; else isHoverEnemy = true;
            }
            else if (CheckButtonClick(1380, 100, 500, 650, mousePos)) {
                if (is1PTurn) isHoverEnemy = true; else isHoverSelf = true;
            }
            else if (m_hoverGrid == aP) isHoverSelf = true;
            else if (m_hoverGrid == tP) isHoverEnemy = true;
        }

        char currentPreviewOp = '\0';
        int preview_aNum = 0;
        int preview_tNum = 0;

        if (m_currentPhase == Phase::P1_Action || m_currentPhase == Phase::P2_Action) {
            if (canAttack && hasOp && activeActor->GetOp() == '/') {
                currentPreviewOp = '/';
                preview_aNum = activeActor->GetNumber();
                preview_tNum = activeTarget->GetNumber();
            }
        }
        else if (m_currentPhase == Phase::P1_Move || m_currentPhase == Phase::P2_Move) {
            if (isMovePreview && isPreviewNextToEnemy && activeTarget && previewOp == '/') {
                currentPreviewOp = '/';
                preview_aNum = previewNum;
                preview_tNum = activeTarget->GetNumber();
            }
        }

        if (currentPreviewOp == '/') {
            int wx = preview_aNum - 1;
            int wy = 9 - preview_tNum;
            Vector2 center = m_mapGrid.GetCellCenter(wx, wy);

            double time = GetNowCount() / 1000.0;
            int alpha = (int)(100 + 100 * sin(time * M_PI * 5.0));

            unsigned int pCol = COL_INFO();
            if (isHoverSelf) pCol = Is1PTurn() ? COL_P1() : COL_P2();
            else if (isHoverEnemy) pCol = Is1PTurn() ? COL_P2() : COL_P1();

            SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
            DrawCircleAA(center.x, center.y, 45.0f, 64, pCol, FALSE, 3.0f);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha / 3);
            DrawCircleAA(center.x, center.y, 40.0f, 64, pCol, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            SetFontSize(18);
            const char* lbl = "結果";
            int tw = GetDrawStringWidth(lbl, (int)strlen(lbl));
            DrawString((int)center.x - tw / 2, (int)center.y + 15, lbl, COL_TEXT_MAIN());
        }

        auto drawUnitCard = [&](int x, int y, UnitBase* unit, bool is1P, int p_previewNum = -1) {
            if (!unit) return;
            unsigned int baseCol = is1P ? COL_P1() : COL_P2();
            std::string headerName = is1P ? (m_is1P_NPC ? "1P (COM)" : "1P PLAYER") : (m_is2P_NPC ? "2P (COM)" : "2P PLAYER");

            if ((is1P && Is1PTurn()) || (!is1P && !Is1PTurn() && m_currentPhase != Phase::FINISH)) {
                SetDrawBlendMode(DX_BLENDMODE_ADD, (int)(100 + 50 * sin(GetNowCount() / 200.0)));
                DrawBox(x - 5, y - 5, x + 505, y + 450, baseCol, FALSE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }

            SetFontSize(36);
            DrawString(x + 5, y + 5, headerName.c_str(), baseCol);
            DrawLine(x, y + 45, x + 500, y + 45, baseCol, 2);

            int scoreY = y + 70;
            DrawBox(x, scoreY, x + 500, scoreY + 200, COL_DARK_BG(), TRUE);

            if (m_ruleMode == RuleMode::ZERO_ONE) {
                Fraction f = is1P ? m_p1ZeroOneScore : m_p2ZeroOneScore;

                if (f.d == 1) {
                    SetFontSize(140);
                    std::string nStr = std::to_string(f.n);
                    DrawString(x + 24, scoreY + 14, nStr.c_str(), COL_TEXT_DARK());
                    DrawString(x + 20, scoreY + 10, nStr.c_str(), COL_TEXT_MAIN());
                }
                else {
                    unsigned int fracCol = is1P ? GetColor(255, 220, 100) : GetColor(180, 220, 255);
                    SetFontSize(80);
                    std::string nStr = std::to_string(f.n);
                    std::string dStr = std::to_string(f.d);
                    int nw = GetDrawStringWidth(nStr.c_str(), (int)nStr.length());
                    int dw = GetDrawStringWidth(dStr.c_str(), (int)dStr.length());
                    int maxW = (std::max)(nw, dw);
                    int cx = x + 100;

                    DrawFormatString(cx - nw / 2 + 4, scoreY - 16, COL_TEXT_DARK(), "%s", nStr.c_str());
                    DrawFormatString(cx - nw / 2, scoreY - 20, fracCol, "%s", nStr.c_str());
                    DrawLine(cx - maxW / 2 - 10, scoreY + 85, cx + maxW / 2 + 10, scoreY + 85, fracCol, 6);
                    DrawFormatString(cx - dw / 2 + 4, scoreY + 94, COL_TEXT_DARK(), "%s", dStr.c_str());
                    DrawFormatString(cx - dw / 2, scoreY + 90, fracCol, "%s", dStr.c_str());
                }

                SetFontSize(24);
                DrawBox(x, scoreY + 200, x + 500, scoreY + 235, is1P ? GetColor(60, 40, 0) : GetColor(20, 30, 80), TRUE);
                DrawFormatString(x + 15, scoreY + 206, COL_TEXT_MAIN(), "目標値 : %d", m_targetScore);
            }
            else {
                if (p_previewNum != -1 && p_previewNum != unit->GetNumber()) {
                    SetFontSize(80);
                    DrawFormatString(x + 20, scoreY + 50, COL_DISABLE(), "%d", unit->GetNumber());
                    DrawString(x + 70, scoreY + 45, "→", COL_TEXT_SUB());
                    SetFontSize(160);
                    DrawFormatString(x + 130, scoreY + 10, COL_SAFE(), "%d", p_previewNum);
                }
                else {
                    SetFontSize(160);
                    DrawString(x + 24, scoreY + 14, std::to_string(unit->GetNumber()).c_str(), COL_TEXT_DARK());
                    DrawFormatString(x + 20, scoreY + 10, COL_TEXT_MAIN(), "%d", unit->GetNumber());
                }
            }

            int infoY = y + 330;
            DrawBox(x, infoY, x + 500, infoY + 120, COL_DARK_BG(), TRUE);
            SetFontSize(24);

            if (m_ruleMode == RuleMode::CLASSIC) {
                DrawString(x + 20, infoY + 15, "残機", GetColor(180, 180, 180));
                for (int i = 0; i < unit->GetMaxStocks(); ++i) {
                    unsigned int sCol = (i < unit->GetStocks()) ? baseCol : GetColor(40, 40, 50);
                    DrawBox(x + 80 + i * 22, infoY + 18, x + 96 + i * 22, infoY + 34, sCol, TRUE);
                }
            }

            int moveDist = 3 - ((unit->GetNumber() - 1) % 3);
            if (p_previewNum != -1 && p_previewNum != unit->GetNumber()) {
                int nextMoveDist = 3 - ((p_previewNum - 1) % 3);
                DrawFormatString(x + 20, infoY + 65, COL_TEXT_SUB(), "移動可能マス: %d → %d マス", moveDist, nextMoveDist);
            }
            else {
                DrawFormatString(x + 20, infoY + 65, COL_TEXT_SUB(), "移動可能マス: %d マス", moveDist);
            }

            int ix = x + 350, iy = infoY + 15;
            SetFontSize(20);
            DrawString(ix - 5, iy, "【移動範囲】", COL_DISABLE());
            int targetNumForGrid = (p_previewNum != -1) ? p_previewNum : unit->GetNumber();
            unsigned int gridBaseCol = (p_previewNum != -1) ? COL_SAFE() : baseCol;

            for (int i = 0; i < 9; ++i) {
                int gx = i % 3, gy = i / 3;
                bool canGo = (i == 4) ? false : (targetNumForGrid <= 3 ? (gx == 1 || gy == 1) : (targetNumForGrid <= 6 ? (gx == gy || gx + gy == 2) : true));
                unsigned int dotCol = canGo ? gridBaseCol : GetColor(40, 40, 50);
                DrawBox(ix + gx * 22, iy + 28 + gy * 22, ix + gx * 22 + 18, iy + 28 + gy * 22 + 18, dotCol, TRUE);
            }
            };

        int p1Preview = (isMovePreview && activeActor == m_player.get()) ? previewNum : -1;
        int p2Preview = (isMovePreview && activeActor == m_enemy.get()) ? previewNum : -1;

        drawUnitCard(40, 100, m_player.get(), true, p1Preview);
        drawUnitCard(1380, 100, m_enemy.get(), false, p2Preview);

        SetFontSize(22);
        DrawLine(40, LOG_PANEL_Y, 540, LOG_PANEL_Y, GetColor(60, 60, 70), 1);
        DrawString(40, LOG_PANEL_Y + 10, "ログ", COL_DISABLE());
        SetFontSize(20);
        for (size_t i = 0; i < m_actionLog.size(); ++i) {
            DrawFormatString(40, LOG_PANEL_Y + 45 + (int)i * 26, GetColor(180, 220, 160), "%s", m_actionLog[i].c_str());
        }

        SetFontSize(22);
        DrawLine(1380, LOG_PANEL_Y, 1880, LOG_PANEL_Y, GetColor(60, 60, 70), 1);
        DrawString(1380, LOG_PANEL_Y + 10, "基本ルール", COL_DISABLE());
        DrawString(1380, LOG_PANEL_Y + 50, " 1, 4, 7 ＝ 3マス移動", GetColor(255, 200, 100));
        DrawString(1380, LOG_PANEL_Y + 90, " 2, 5, 8 ＝ 2マス移動", GetColor(180, 180, 180));
        DrawString(1380, LOG_PANEL_Y + 130, " 3, 6, 9 ＝ 1マス移動", GetColor(100, 150, 255));
        DrawString(1380, LOG_PANEL_Y + 180, " ＋、－、＊、÷を取得することで移動可能方向追加", GetColor(255, 255, 180));
        DrawString(1380, LOG_PANEL_Y + 215, " ÷ ＝ (分子,分母)のマスにワープ設置", COL_INFO());

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
        DrawBox(600, BOTTOM_PANEL_Y, 1320, 940, COL_BOTTOM_BG(), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        unsigned int calcBorderCol = Is1PTurn() ? COL_P1() : COL_P2();
        DrawLine(600, BOTTOM_PANEL_Y, 1320, BOTTOM_PANEL_Y, calcBorderCol, 2);

        bool showCalcPanel = false;
        int disp_aNum = 0; int disp_tNum = 0; char disp_aOp = '\0';
        bool isPreviewMode = false; bool willGetNewOp = false;

        if (m_currentPhase == Phase::P1_Action || m_currentPhase == Phase::P2_Action) {
            if (canAttack && hasOp) {
                showCalcPanel = true;
                disp_aNum = activeActor->GetNumber();
                disp_tNum = activeTarget->GetNumber();
                disp_aOp = activeActor->GetOp();
            }
        }
        else if (m_currentPhase == Phase::P1_Move || m_currentPhase == Phase::P2_Move) {
            if (isMovePreview && isPreviewNextToEnemy && activeTarget && previewOp != '\0') {
                showCalcPanel = true;
                disp_aNum = previewNum;
                disp_tNum = activeTarget->GetNumber();
                disp_aOp = previewOp;
                isPreviewMode = true;
                if (activeActor->GetOp() != previewOp) willGetNewOp = true;
            }
        }

        if (showCalcPanel) {
            if (isPreviewMode) {
                SetFontSize(20);
                if (willGetNewOp) DrawFormatString(610, BOTTOM_PANEL_Y + 10, GetColor(255, 200, 100), "→ 移動プレビュー（演算子 [%c] を取得してバトル）", disp_aOp);
                else DrawString(610, BOTTOM_PANEL_Y + 10, "→ 移動プレビュー（バトル可能）", GetColor(255, 200, 100));
            }

            int intRes = 0;
            Fraction resFrac(0);
            bool isCleanDivide = true;

            if (disp_aOp == '+') { intRes = disp_aNum + disp_tNum; resFrac = Fraction(intRes); }
            else if (disp_aOp == '-') { intRes = disp_aNum - disp_tNum; resFrac = Fraction(intRes); }
            else if (disp_aOp == '*') { intRes = disp_aNum * disp_tNum; resFrac = Fraction(intRes); }
            else if (disp_aOp == '/') {
                if (disp_tNum != 0 && disp_aNum % disp_tNum == 0) {
                    intRes = disp_aNum / disp_tNum;
                    resFrac = Fraction(intRes);
                }
                else {
                    isCleanDivide = false;
                }
            }

            if (m_ruleMode == RuleMode::ZERO_ONE) {
                SetFontSize(64);
                int calcY = isPreviewMode ? BOTTOM_PANEL_Y + 27 : BOTTOM_PANEL_Y + 17;
                DrawFormatString(672, calcY, COL_TEXT_DARK(), "%d %c %d =", disp_aNum, disp_aOp, disp_tNum);
                DrawFormatString(670, calcY - 2, COL_TEXT_MAIN(), "%d %c %d =", disp_aNum, disp_aOp, disp_tNum);

                unsigned int resColor = (resFrac.n < 0) ? COL_DANGER() : (!isCleanDivide ? COL_DISABLE() : COL_WARN());
                int resX = 1010;

                if (resFrac.d == 1 || !isCleanDivide) {
                    DrawFormatString(resX + 2, calcY, COL_TEXT_DARK(), "%lld", resFrac.n);
                    DrawFormatString(resX, calcY - 2, resColor, "%lld", resFrac.n);
                }

                SetFontSize(22);
                if (isPreviewMode) {
                    Fraction goal(m_targetScore);
                    Fraction nextMy = (Is1PTurn() ? m_p1ZeroOneScore : m_p2ZeroOneScore) + resFrac;
                    if (nextMy > goal) nextMy = goal - (nextMy - goal);

                    Fraction nextEn = (Is1PTurn() ? m_p2ZeroOneScore : m_p1ZeroOneScore) + resFrac;
                    if (nextEn > goal) nextEn = goal - (nextEn - goal);

                    std::string strMy = (nextMy.d == 1) ? std::to_string(nextMy.n) : (std::to_string(nextMy.n) + "/" + std::to_string(nextMy.d));
                    std::string strEn = (nextEn.d == 1) ? std::to_string(nextEn.n) : (std::to_string(nextEn.n) + "/" + std::to_string(nextEn.d));

                    DrawString(650, BOTTOM_PANEL_Y + 85, "【自分に適用】", COL_SAFE());
                    DrawFormatString(790, BOTTOM_PANEL_Y + 85, COL_TEXT_SUB(), "スコア: %s", strMy.c_str());

                    DrawString(950, BOTTOM_PANEL_Y + 85, "【相手に適用】", COL_DANGER());
                    DrawFormatString(1090, BOTTOM_PANEL_Y + 85, COL_TEXT_SUB(), "スコア: %s", strEn.c_str());

                    if (disp_aOp == '/') {
                        int wx = disp_aNum - 1; int wy = 9 - disp_tNum;
                        if (isCleanDivide) {
                            DrawFormatString(650, BOTTOM_PANEL_Y + 115, COL_SAFE(), "座標(%d, %d)に【自分】のワープ設置！", wx, wy);
                            DrawFormatString(950, BOTTOM_PANEL_Y + 115, COL_DANGER(), "座標(%d, %d)に【相手】のワープ設置！", wx, wy);
                        }
                        else {
                            DrawFormatString(650, BOTTOM_PANEL_Y + 115, COL_INFO(), "座標(%d, %d)にワープ設置 (※割り切れないためスコア変動なし)", wx, wy);
                        }
                    }
                }
                else {
                    if (isHoverSelf || isHoverEnemy) {
                        std::string targetName = isHoverSelf ? (activeActor == m_player.get() ? "1P" : "2P") : (activeTarget == m_player.get() ? "1P" : "2P");
                        DrawFormatString(650, BOTTOM_PANEL_Y + 82, COL_TEXT_MAIN(), "▼ %s のスコアにこの結果を反映", targetName.c_str());
                        if (disp_aOp == '/') {
                            unsigned int warpCol = isHoverSelf ? COL_SAFE() : COL_DANGER();
                            if (isCleanDivide) DrawFormatString(650, BOTTOM_PANEL_Y + 112, warpCol, "さらに座標(%d, %d)に【%s】のワープ設置！", disp_aNum, disp_tNum, targetName.c_str());
                            else DrawFormatString(650, BOTTOM_PANEL_Y + 112, warpCol, "座標(%d, %d)に【%s】のワープ設置！ (※スコア変動なし)", disp_aNum, disp_tNum, targetName.c_str());
                        }
                    }
                    else {
                        DrawString(770, BOTTOM_PANEL_Y + 82, "どちらに反映しますか", GetColor(120, 120, 130));
                        if (disp_aOp == '/') DrawFormatString(770, BOTTOM_PANEL_Y + 112, COL_INFO(), "実行時、選択した対象にワープを設置！");
                    }
                }
            }
            else { // CLASSIC モード
                SetFontSize(64);
                int calcY = isPreviewMode ? BOTTOM_PANEL_Y + 27 : BOTTOM_PANEL_Y + 17;
                DrawFormatString(672, calcY, COL_TEXT_DARK(), "%d %c %d =>", disp_aNum, disp_aOp, disp_tNum);
                DrawFormatString(670, calcY - 2, COL_TEXT_MAIN(), "%d %c %d =>", disp_aNum, disp_aOp, disp_tNum);

                unsigned int resColor = (intRes < 0) ? COL_DANGER() : (!isCleanDivide ? COL_DISABLE() : COL_SAFE());
                DrawFormatString(1032, calcY, COL_TEXT_DARK(), "%s%d", (intRes > 0 ? "+" : ""), intRes);
                DrawFormatString(1030, calcY - 2, resColor, "%s%d", (intRes > 0 ? "+" : ""), intRes);

                SetFontSize(24);
                if (isPreviewMode) {
                    if (disp_aOp == '/' && !isCleanDivide) {
                        DrawString(650, BOTTOM_PANEL_Y + 85, "【自分に反映】", COL_SAFE());
                        DrawFormatString(650, BOTTOM_PANEL_Y + 115, COL_INFO(), "★ 座標(%d, %d)にワープ設置 (※体力変動なし)", disp_aNum, disp_tNum);
                        DrawString(950, BOTTOM_PANEL_Y + 85, "【相手に反映】", COL_DANGER());
                        DrawFormatString(950, BOTTOM_PANEL_Y + 115, COL_INFO(), "★ 座標(%d, %d)にワープ設置 (※体力変動なし)", disp_aNum, disp_tNum);
                    }
                    else {
                        auto calcDmg = [](int currentHp, int currentStocks, int val, int& outHp, int& outStocks) {
                            outHp = currentHp + val; outStocks = currentStocks;
                            while (outHp <= 0) { outStocks--; outHp += 9; }
                            while (outHp > 9) { outStocks++; outHp -= 9; }
                            };
                        int myHp, mySt, enHp, enSt;
                        calcDmg(disp_aNum, activeActor->GetStocks(), intRes, myHp, mySt);
                        calcDmg(disp_tNum, activeTarget->GetStocks(), intRes, enHp, enSt);

                        DrawString(650, BOTTOM_PANEL_Y + 85, "【自分に反映】", COL_SAFE());
                        if (mySt <= 0 && myHp <= 0) DrawString(790, BOTTOM_PANEL_Y + 85, "GAME OVER", COL_DANGER());
                        else DrawFormatString(790, BOTTOM_PANEL_Y + 85, COL_TEXT_SUB(), "残機 %d / 体力 %d", mySt, myHp);

                        DrawString(950, BOTTOM_PANEL_Y + 85, "【相手に反映】", COL_DANGER());
                        if (enSt <= 0 && enHp <= 0) DrawString(1090, BOTTOM_PANEL_Y + 85, "撃破", COL_WARN());
                        else DrawFormatString(1090, BOTTOM_PANEL_Y + 85, COL_TEXT_SUB(), "残機 %d | 体力 %d", enSt, enHp);

                        if (disp_aOp == '/') {
                            DrawFormatString(650, BOTTOM_PANEL_Y + 115, COL_SAFE(), "座標(%d, %d)に【自分】のワープ設置！", disp_aNum, disp_tNum);
                            DrawFormatString(950, BOTTOM_PANEL_Y + 115, COL_DANGER(), "座標(%d, %d)に【相手】のワープ設置！", disp_aNum, disp_tNum);
                        }
                    }
                }
                else {
                    if (isHoverSelf || isHoverEnemy) {
                        std::string targetName = isHoverSelf ? (activeActor == m_player.get() ? "1P" : "2P") : (activeTarget == m_player.get() ? "1P" : "2P");
                        int targetCurrentHp = isHoverSelf ? disp_aNum : disp_tNum;
                        int newHpRaw = targetCurrentHp + intRes;
                        int stockChange = 0;
                        int simulatedHp = newHpRaw;
                        while (simulatedHp <= 0) { stockChange--; simulatedHp += 9; }
                        while (simulatedHp > 9) { stockChange++; simulatedHp -= 9; }

                        if (stockChange < 0) DrawFormatString(620, BOTTOM_PANEL_Y + 82, COL_DANGER(), "▼ 攻撃！ %s の【 残機を%d 】、体力を [%d] にします", targetName.c_str(), stockChange, simulatedHp);
                        else if (stockChange > 0) DrawFormatString(620, BOTTOM_PANEL_Y + 82, COL_SAFE(), "▼ 回復！ %s の【 残機を+%d 】、体力を [%d] にします", targetName.c_str(), stockChange, simulatedHp);
                        else DrawFormatString(620, BOTTOM_PANEL_Y + 82, COL_TEXT_SUB(), "▼ 反映！ %s の体力を [%d] に", targetName.c_str(), simulatedHp);

                        if (disp_aOp == '/') {
                            unsigned int warpCol = isHoverSelf ? COL_SAFE() : COL_DANGER();
                            if (isCleanDivide) DrawFormatString(620, BOTTOM_PANEL_Y + 112, warpCol, "★ さらに座標(%d, %d)に【%s】のワープ設置！", disp_aNum, disp_tNum, targetName.c_str());
                            else DrawFormatString(620, BOTTOM_PANEL_Y + 112, warpCol, "座標(%d, %d)に【%s】のワープ設置！ (※体力変動なし)", disp_aNum, disp_tNum, targetName.c_str());
                        }
                    }
                    else {
                        DrawString(770, BOTTOM_PANEL_Y + 82, "適用する対象を選択してください", GetColor(120, 120, 130));
                        if (disp_aOp == '/') DrawFormatString(770, BOTTOM_PANEL_Y + 112, COL_INFO(), "実行時、選択した対象にワープ設置！");
                    }
                }
            }
        }
        else if (m_currentPhase == Phase::P1_Action || m_currentPhase == Phase::P2_Action) {
            if (canAttack && !hasOp) { SetFontSize(30); DrawString(730, BOTTOM_PANEL_Y + 40, "【 演算子アイテムが必要です 】", COL_DANGER()); }
            else { SetFontSize(28); DrawString(800, BOTTOM_PANEL_Y + 40, "ターゲットが射程内にいません", COL_DISABLE()); }
        }
        else {
            SetFontSize(28); DrawString(760, BOTTOM_PANEL_Y + 40, "移動するマスを選択してください", COL_DISABLE());
        }

        // ==========================================
        // 9. 行動選択ボタン (アクションフェーズのみ)
        // ==========================================
        if (m_currentPhase == Phase::P1_Action || m_currentPhase == Phase::P2_Action) {
            if (m_currentPhase == Phase::P1_Action && m_is1P_NPC) return;
            if (m_currentPhase == Phase::P2_Action && m_is2P_NPC) return;

            int by = 960;
            if (canAttack && hasOp) {
                auto drawBtn = [&](int x, int y, int w, int h, const char* text, unsigned int col, bool hover) {
                    if (hover) {
                        DrawBox(x, y, x + w, y + h, col, TRUE);
                        SetFontSize(26);
                        DrawFormatString(x + (w - GetDrawStringWidth(text, (int)strlen(text))) / 2, y + 22, COL_TEXT_DARK(), text);
                    }
                    else {
                        DrawBox(x, y, x + w, y + h, GetColor(20, 20, 25), TRUE);
                        DrawBox(x, y, x + w, y + h, col, FALSE);
                        SetFontSize(26);
                        DrawFormatString(x + (w - GetDrawStringWidth(text, (int)strlen(text))) / 2, y + 22, col, text);
                    }
                    };
                drawBtn(600, by, 220, 60, "自分", COL_SAFE(), isHoverSelf);
                drawBtn(850, by, 220, 60, "相手", COL_DANGER(), isHoverEnemy);
                drawBtn(1100, by, 220, 60, "何もしない", COL_DISABLE(), CheckButtonClick(1100, by, 220, 60, mousePos));
            }
            else {
                bool hover = CheckButtonClick(750, by, 420, 60, mousePos);
                unsigned int btnCol = GetColor(180, 180, 200);
                if (hover) {
                    DrawBox(750, by, 1170, by + 60, btnCol, TRUE);
                    SetFontSize(28);
                    DrawString(875, by + 16, "ターン終了", COL_TEXT_DARK());
                }
                else {
                    DrawBox(750, by, 1170, by + 60, GetColor(20, 20, 25), TRUE);
                    DrawBox(750, by, 1170, by + 60, btnCol, FALSE);
                    SetFontSize(28);
                    DrawString(875, by + 16, "ターン終了", btnCol);
                }
            }
        }

        // ==========================================
        // 10. GAME SET オーバーレイ
        // ==========================================
        if (m_currentPhase == Phase::FINISH) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
            DrawBox(0, 0, SCREEN_W, SCREEN_H, COL_TEXT_DARK(), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            SetFontSize(160);
            const char* finishText = "ゲーム終了 !!";
            int textW = GetDrawStringWidth(finishText, (int)strlen(finishText));
            DrawString(SCREEN_W / 2 - textW / 2, SCREEN_H / 2 - 100, finishText, COL_TEXT_MAIN());

            if (m_finishTimer > 30) {
                SetFontSize(40);
                const char* nextText = ">> クリックしてください <<";
                int nw = GetDrawStringWidth(nextText, (int)strlen(nextText));
                DrawString(SCREEN_W / 2 - nw / 2, SCREEN_H / 2 + 100, nextText, COL_TEXT_SUB());
            }
        }
    }

    // ==========================================
    // ★ ゲーム終了判定
    // ==========================================
    bool BattleMaster::IsGameOver() const {
        if (!m_player || !m_enemy) return false;

        // ゼロワンモード：どちらかが目標スコアに到達
        if (m_ruleMode == RuleMode::ZERO_ONE) {
            Fraction goal(m_targetScore);
            return (m_p1ZeroOneScore == goal || m_p2ZeroOneScore == goal);
        }

        // クラシックモード：残機0 かつ 体力0以下 で敗北
        if (m_player->GetStocks() <= 0 && m_player->GetNumber() <= 0) return true;
        if (m_enemy->GetStocks() <= 0 && m_enemy->GetNumber() <= 0) return true;

        return false;
    }

    // ==========================================
    // ★ プレイヤー勝利判定
    // ==========================================
    bool BattleMaster::IsPlayerWin() const {
        if (!m_player || !m_enemy) return false;

        if (m_ruleMode == RuleMode::ZERO_ONE) {
            Fraction goal(m_targetScore);
            return (m_p1ZeroOneScore == goal);
        }
        else {
            // クラシックモード：敵の残機と体力が尽きたら勝利
            return (m_enemy->GetStocks() <= 0 && m_enemy->GetNumber() <= 0);
        }
    }

    // ==========================================
    // ★ ボタンのクリック判定（矩形 vs 点）
    // ==========================================
    bool BattleMaster::CheckButtonClick(int x, int y, int w, int h, const Vector2& mousePos) const {
        return (mousePos.x >= static_cast<float>(x) &&
            mousePos.x <= static_cast<float>(x + w) &&
            mousePos.y >= static_cast<float>(y) &&
            mousePos.y <= static_cast<float>(y + h));
    }
} // namespace App