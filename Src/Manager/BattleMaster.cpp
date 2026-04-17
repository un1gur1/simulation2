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
        , m_playerAIStarted(false)
        , m_is1P_NPC(false)
        , m_is2P_NPC(true)
        , m_finishTimer(0)
        , m_psHandle(-1)
        , m_cbHandle(-1)
        , m_shaderTime(0.0f)
        , m_effectIntensity(0.0f)
        , m_playFrames(0), m_totalMoves(0), m_totalOps(0), m_maxDamage(0) // ★初期化
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

        m_is1P_NPC = sm->Is1PNPC();
        m_is2P_NPC = sm->Is2PNPC();
        IntVector2 p1StartPos{ sm->Get1PStartX(), sm->Get1PStartY() };
        IntVector2 p2StartPos{ sm->Get2PStartX(), sm->Get2PStartY() };
        int p1Num = sm->Get1PStartNum();
        int p2Num = sm->Get2PStartNum();

        m_player = std::make_unique<Player>(p1StartPos, m_mapGrid.GetCellCenter(p1StartPos.x, p1StartPos.y), p1Num, 5, 5);
        m_enemy = std::make_unique<Enemy>(p2StartPos, m_mapGrid.GetCellCenter(p2StartPos.x, p2StartPos.y), p2Num, 5, 5);

        m_player->SetOp('\0');
        m_enemy->SetOp('\0');

        m_currentPhase = Phase::P1_Move;
        m_isPlayerSelected = false;
        m_enemyAIStarted = false;
        m_playerAIStarted = false;
        m_finishTimer = 0;

        // ★ 戦績カウントリセット
        m_playFrames = 0;
        m_totalMoves = 0;
        m_totalOps = 0;
        m_maxDamage = 0;

        m_psHandle = LoadPixelShaderFromMem(g_ps_CyberGrid, sizeof(g_ps_CyberGrid));
        m_cbHandle = CreateShaderConstantBuffer(sizeof(float) * 4);
        m_shaderTime = 0.0f;
        m_effectIntensity = 0.0f;

        m_actionLog.clear();
        std::string rModeStr = (m_ruleMode == RuleMode::CLASSIC) ? "クラシック" : "ゼロワン";
        std::string p1Type = m_is1P_NPC ? "COM" : "PLAYER";
        std::string p2Type = m_is2P_NPC ? "COM" : "PLAYER";

        AddLog(">>> バトル開始！ [1P:" + p1Type + " vs 2P:" + p2Type + " / " + rModeStr + "]");

        if (m_ruleMode == RuleMode::ZERO_ONE) {
            AddLog(">>> 目標：相手よりはやくスコアを【 " + std::to_string(m_targetScore) + " 】にしよう！");
        }
        else {
            AddLog(">>> 目標：相手の残機をゼロに！");
        }
    }

    bool BattleMaster::CanMove(int number, char op, IntVector2 start, IntVector2 target, int& outCost) const {
        if (start == target) return false;

        UnitBase* u = nullptr;
        if (m_player && m_player->GetGridPos() == start) u = m_player.get();
        else if (m_enemy && m_enemy->GetGridPos() == start) u = m_enemy.get();

        if (u && u->HasWarpNode(target)) {
            outCost = 1;
            return true;
        }

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

        if (input.IsMouseLeftTrg()) {
            IntVector2 pos = activeUnit.GetGridPos();

            if (!m_isPlayerSelected) {
                if (m_hoverGrid == pos) m_isPlayerSelected = true;
            }
            else {
                if (m_hoverGrid == pos) {
                    activeUnit.AddNumber(-1);
                    m_isPlayerSelected = false;
                    m_currentPhase = nextPhase;
                }
                else if (m_mapGrid.IsWithinBounds(m_hoverGrid.x, m_hoverGrid.y)) {
                    int cost = 0;
                    if (CanMove(activeUnit.GetNumber(), activeUnit.GetOp(), pos, m_hoverGrid, cost)) {
                        activeUnit.AddNumber(-cost);
                        std::queue<Vector2> autoPath;

                        int dx = std::abs(m_hoverGrid.x - pos.x);
                        int dy = std::abs(m_hoverGrid.y - pos.y);

                        // ★戦績：移動マス数を加算
                        m_totalMoves += std::max(dx, dy);

                        if (activeUnit.HasWarpNode(m_hoverGrid) || (dx != dy && dx != 0 && dy != 0)) {
                            autoPath.push(m_mapGrid.GetCellCenter(m_hoverGrid.x, m_hoverGrid.y));
                            AddLog("【跳躍】 陣地へ転送完了。");
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
                    else {
                        m_isPlayerSelected = false;
                    }
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
            std::string name = (&actor == m_player.get()) ? "1P" : "2P";
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

    void BattleMaster::ExecuteAIAction(UnitBase* me, UnitBase* opp, bool is1P) {
        if (!me || !opp || me->IsMoving()) return;

        IntVector2 myP = me->GetGridPos();
        char pickedItem = m_mapGrid.PickUpItem(myP.x, myP.y);
        std::string myName = is1P ? "1P" : "2P";

        if (pickedItem != '\0') {
            me->SetOp(pickedItem);
            AddLog("【取得】 " + myName + " が演算子 [" + std::string(1, pickedItem) + "] を取得");
        }

        IntVector2 oppP = opp->GetGridPos();
        if (std::abs(myP.x - oppP.x) + std::abs(myP.y - oppP.y) == 1) {
            char myOp = me->GetOp();
            if (myOp != '\0') {
                if (m_ruleMode == RuleMode::ZERO_ONE) {
                    ExecuteBattle(*me, *opp, *me);
                }
                else {
                    int aNum = me->GetNumber();
                    int tNum = opp->GetNumber();
                    int delta = 0;
                    if (myOp == '+') delta = aNum + tNum;
                    else if (myOp == '-') delta = aNum - tNum;
                    else if (myOp == '*') delta = aNum * tNum;

                    if (myOp == '/') {
                        ExecuteBattle(*me, *opp, *me);
                    }
                    else if (delta < 0) {
                        ExecuteBattle(*me, *opp, *opp);
                    }
                    else {
                        ExecuteBattle(*me, *opp, *me);
                    }
                }
            }
            else {
                AddLog("【待機】 " + myName + " は演算子がないため様子を見ている");
            }
        }

        if (!is1P) m_mapGrid.UpdateTurn();
        m_currentPhase = is1P ? Phase::P2_Move : Phase::P1_Move;

        if (is1P) m_playerAIStarted = false;
        else m_enemyAIStarted = false;
    }

    void BattleMaster::Update() {
        // ==========================================
        // ★ フィニッシュ（決着）演出フェーズ
        // ==========================================
        if (m_currentPhase == Phase::FINISH) {
            m_finishTimer++;
            m_effectIntensity = std::min(m_effectIntensity + 0.1f, 5.0f);
            m_shaderTime += 0.01f * m_effectIntensity;

            // 1秒経ったら入力を受け付けて、ResultSceneへ遷移する！
            if (m_finishTimer > 60) {
                auto& input = InputManager::GetInstance();
                if (input.IsMouseLeftTrg() || input.IsTrgDown(KEY_INPUT_SPACE) || input.IsTrgDown(KEY_INPUT_RETURN)) {

                    // ★戦績をまとめて、SceneManagerに渡す！
                    BattleStats stats = {
                        m_mapGrid.GetTotalTurns(),
                        m_totalMoves,
                        m_totalOps,
                        m_playFrames,
                        m_maxDamage
                    };
                    auto* sm = SceneManager::GetInstance();
                    sm->SetBattleResult(IsPlayerWin(), stats);

                    // シーン遷移！
                    sm->ChangeScene(SceneManager::SCENE_ID::RESULT);
                }
            }
            return;
        }

        // ==========================================
        // ★ 通常フェーズ
        // ==========================================
        m_playFrames++; // ★戦績：プレイ時間を毎フレーム加算

        if (m_ruleMode == RuleMode::ZERO_ONE) {
            if (m_player && m_player->GetStocks() < m_player->GetMaxStocks()) {
                m_player->AddStocks(m_player->GetMaxStocks() - m_player->GetStocks());
            }
            if (m_enemy && m_enemy->GetStocks() < m_enemy->GetMaxStocks()) {
                m_enemy->AddStocks(m_enemy->GetMaxStocks() - m_enemy->GetStocks());
            }
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
                else if (m_player && !m_player->IsMoving()) { m_currentPhase = Phase::P1_Action; }
            }
            else { HandleMoveInput(*m_player, Phase::P1_Action); }
            break;
        case Phase::P1_Action:
            if (m_is1P_NPC) { ExecuteAIAction(m_player.get(), m_enemy.get(), true); }
            else { HandleActionInput(*m_player, *m_enemy); }
            break;
        case Phase::P2_Move:
            if (m_is2P_NPC) {
                if (!m_enemyAIStarted) { ExecuteAI(m_enemy.get(), m_player.get(), false); m_enemyAIStarted = true; }
                else if (m_enemy && !m_enemy->IsMoving()) { m_currentPhase = Phase::P2_Action; }
            }
            else { HandleMoveInput(*m_enemy, Phase::P2_Action); }
            break;
        case Phase::P2_Action:
            if (m_is2P_NPC) { ExecuteAIAction(m_enemy.get(), m_player.get(), false); }
            else { HandleActionInput(*m_enemy, *m_player); }
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

        IntVector2 ePos = me->GetGridPos();
        std::queue<Vector2> screenPath;
        std::string myName = is1P ? "1P" : "2P";

        if (me->HasWarpNode(bestTarget)) {
            AddLog("【行動】 " + myName + " が陣地へワープ！ (コスト: -1)");
        }
        else if (bestTarget == ePos) {
            AddLog("【待機】 " + myName + " がその場で数字を調整 (コスト: -1)");
        }
        else {
            AddLog("【行動】 " + myName + " が進軍 (コスト: -" + std::to_string(selectedCost) + ")");
        }

        int dx = std::abs(bestTarget.x - ePos.x);
        int dy = std::abs(bestTarget.y - ePos.y);

        // ★戦績：移動マス数を加算
        m_totalMoves += std::max(dx, dy);

        if (me->HasWarpNode(bestTarget) || bestTarget == ePos || (dx != dy && dx != 0 && dy != 0)) {
            screenPath.push(m_mapGrid.GetCellCenter(bestTarget.x, bestTarget.y));
        }
        else {
            int stepX = (bestTarget.x > ePos.x) ? 1 : (bestTarget.x < ePos.x) ? -1 : 0;
            int stepY = (bestTarget.y > ePos.y) ? 1 : (bestTarget.y < ePos.y) ? -1 : 0;
            IntVector2 curr = ePos;

            int maxSteps = std::max(dx, dy);
            for (int i = 0; i < maxSteps; ++i) {
                curr.x += stepX;
                curr.y += stepY;
                screenPath.push(m_mapGrid.GetCellCenter(curr.x, curr.y));
            }
        }

        me->AddNumber(-selectedCost);
        me->StartMove(bestTarget, screenPath);

        if (is1P) m_playerAIStarted = true;
        else m_enemyAIStarted = true;
    }

    int BattleMaster::EvaluateBoard(UnitBase& me, int myVirtualNumber, UnitBase& enemy, IntVector2 targetPos, bool is1P) {
        // ... (EvaluateBoardの中身は既存のままでOKです) ...
        int score = 0;
        char myOp = me.GetOp();
        IntVector2 ePos = enemy.GetGridPos();
        bool canAttack = (std::abs(targetPos.x - ePos.x) + std::abs(targetPos.y - ePos.y) == 1);

        if (m_ruleMode == RuleMode::ZERO_ONE) {
            Fraction goal(m_targetScore);
            Fraction myScore = is1P ? m_p1ZeroOneScore : m_p2ZeroOneScore;
            Fraction enemyScore = is1P ? m_p2ZeroOneScore : m_p1ZeroOneScore;

            long long myDist = std::abs((goal - myScore).n / (goal - myScore).d);
            long long enemyDist = std::abs((goal - enemyScore).n / (goal - enemyScore).d);

            if (myOp == '\0') {
                char itemHere = m_mapGrid.GetItemAt(targetPos.x, targetPos.y);
                if (itemHere != '\0') {
                    score += 5000;
                    if (myDist <= 5) {
                        if (itemHere == '-') score += 3000;
                        if (itemHere == '+') score -= 2000;
                    }
                }
                else {
                    int minDist = 999;
                    for (int ix = 0; ix < 9; ++ix) {
                        for (int iy = 0; iy < 9; ++iy) {
                            char item = m_mapGrid.GetItemAt(ix, iy);
                            if (item != '\0') {
                                int dist = std::abs(targetPos.x - ix) + std::abs(targetPos.y - iy);
                                if (myDist <= 5) {
                                    if (item == '-') dist -= 5;
                                    if (item == '+') dist += 5;
                                }
                                if (dist < minDist) minDist = dist;
                            }
                        }
                    }
                    score += (100 - minDist) * 20;
                }
            }
            else {
                if (canAttack) {
                    int eNum = enemy.GetNumber();

                    if (myOp == '/') {
                        int wx = myVirtualNumber - 1;
                        int wy = 9 - eNum;
                        IntVector2 nodePos{ wx, wy };

                        if (wy >= 0 && wy <= 8 && wx >= 0 && wx <= 8) {
                            if (!me.HasWarpNode(nodePos)) {
                                score += 1500;
                                if (myDist > 100) score += 1000;
                            }
                            else {
                                score -= 500;
                            }
                        }
                    }
                    else {
                        Fraction resFrac;
                        if (myOp == '+') resFrac = Fraction(myVirtualNumber + eNum);
                        else if (myOp == '-') resFrac = Fraction(myVirtualNumber - eNum);
                        else if (myOp == '*') resFrac = Fraction(myVirtualNumber * eNum);

                        Fraction predicted = myScore + resFrac;

                        if (predicted == goal) return 1000000;

                        if (predicted > goal) {
                            Fraction excess = predicted - goal;
                            predicted = goal - excess;
                        }

                        long long predictedDist = std::abs((goal - predicted).n / (goal - predicted).d);
                        long long progress = myDist - predictedDist;

                        if (progress > 0) {
                            score += (int)progress * 50;
                            score += (1000 - (int)predictedDist) * 2;
                        }
                        else {
                            if (myDist <= 3) score += 2000;
                            else score -= (int)std::abs(progress) * 50;
                        }
                    }
                }
                else {
                    int distToEnemy = std::abs(targetPos.x - ePos.x) + std::abs(targetPos.y - ePos.y);
                    score += (100 - distToEnemy) * 15;
                }
            }

            if (canAttack) {
                if (myOp == '\0' && enemy.GetOp() != '\0') score -= 4000;
                if (myOp != '\0' && enemyDist <= 27) score += 800;
            }
        }
        else {
            score += me.GetStocks() * 10000;
            score -= enemy.GetStocks() * 10000;

            if (myVirtualNumber <= 2) score -= 1000;

            if (myOp == '\0') {
                char itemHere = m_mapGrid.GetItemAt(targetPos.x, targetPos.y);
                if (itemHere != '\0') {
                    score += 5000;
                    if (itemHere == '-') score += 2000;
                }
                else {
                    int minDist = 999;
                    for (int ix = 0; ix < 9; ++ix) {
                        for (int iy = 0; iy < 9; ++iy) {
                            char item = m_mapGrid.GetItemAt(ix, iy);
                            if (item != '\0') {
                                int dist = std::abs(targetPos.x - ix) + std::abs(targetPos.y - iy);
                                if (item == '-') dist -= 2;
                                if (dist < minDist) minDist = dist;
                            }
                        }
                    }
                    score += (100 - minDist) * 20;
                }
            }
            else {
                if (canAttack) {
                    int eNum = enemy.GetNumber();

                    if (myOp == '/') {
                        int wx = myVirtualNumber - 1;
                        int wy = 9 - eNum;
                        IntVector2 nodePos{ wx, wy };
                        if (wy >= 0 && wy <= 8 && wx >= 0 && wx <= 8) {
                            if (!me.HasWarpNode(nodePos)) score += 1500;
                            else score -= 500;
                        }
                    }
                    else {
                        int delta = 0;
                        if (myOp == '+') delta = myVirtualNumber + eNum;
                        else if (myOp == '-') delta = myVirtualNumber - eNum;
                        else if (myOp == '*') delta = myVirtualNumber * eNum;

                        if (delta < 0) {
                            int newHpRaw = eNum + delta;
                            int sChange = 0;
                            int simHp = newHpRaw;
                            while (simHp <= 0) { sChange--; simHp += 9; }

                            if (sChange < 0) return 1000000;
                            score += std::abs(delta) * 100;
                        }
                        if (delta > 0) {
                            int newHpRaw = myVirtualNumber + delta;
                            int sChange = 0;
                            int simHp = newHpRaw;
                            while (simHp > 9) { sChange++; simHp -= 9; }

                            if (sChange > 0) {
                                score += sChange * 5000;
                            }
                            else {
                                score += delta * 50;
                            }
                        }
                    }
                }
                else {
                    int distToEnemy = std::abs(targetPos.x - ePos.x) + std::abs(targetPos.y - ePos.y);
                    score += (100 - distToEnemy) * 15;
                }
            }

            if (canAttack && myOp == '\0' && enemy.GetOp() != '\0') {
                score -= 3000;
            }
        }

        if (targetPos.x == 0 || targetPos.x == 8 || targetPos.y == 0 || targetPos.y == 8) {
            score -= 200;
        }

        return score;
    }

    void BattleMaster::ExecuteAI(UnitBase* me, UnitBase* opp, bool is1P) {
        if (!me || me->GetStocks() <= 0) return;

        IntVector2 ePos = me->GetGridPos();
        int currentNum = me->GetNumber();
        char currentOp = me->GetOp();

        IntVector2 bestMove = ePos;
        int bestScore = -9999999;
        int selectedCost = 1;

        for (int x = 0; x < 9; ++x) {
            for (int y = 0; y < 9; ++y) {
                IntVector2 target{ x, y };
                int cost = 0;

                bool canGo = (target == ePos);
                if (!canGo) {
                    canGo = CanMove(currentNum, currentOp, ePos, target, cost);
                }
                else {
                    cost = 1;
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

                    me->StartMove(ePos, std::queue<Vector2>());
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

        if (aOp == '/') {
            if (dNum != 0) {
                int wx = aNum - 1;
                int wy = 9 - dNum;
                IntVector2 nodePos{ wx, wy };

                if (!attacker.HasWarpNode(nodePos)) {
                    attacker.AddWarpNode(nodePos);
                    std::string aName = (&attacker == m_player.get()) ? "1P" : "2P";
                    AddLog("【陣地構築】 " + aName + " が座標 (" + std::to_string(aNum) + ", " + std::to_string(dNum) + ") に駅を設置！");
                }
            }
        }

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

        std::string aName = (&attacker == m_player.get()) ? "1P" : "2P";

        if (aOp != '/') {
            AddLog(">> " + aName + " の計算！ [ 値: " + std::to_string(intRes) + " ]");
        }

        // ★戦績：使用回数と最大ダメージの記録
        m_totalOps++;
        if (std::abs(intRes) > m_maxDamage) {
            m_maxDamage = std::abs(intRes);
        }

        ApplyBattleResult(target, resFrac, intRes, aOp);
        attacker.SetOp('\0');
    }

    void BattleMaster::ApplyBattleResult(UnitBase& unit, Fraction resultFrac, int intRes, char op) {
        // ... (ApplyBattleResultの中身は既存のままでOKです) ...
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
                AddLog("【NODE】 スコア加算なし。座標系への干渉に成功。");
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

                while (newHpRaw <= 0) {
                    stockChange--;
                    newHpRaw += 9;
                }
                while (newHpRaw > 9) {
                    stockChange++;
                    newHpRaw -= 9;
                }

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

                bool isWarpNode = activeUnit->HasWarpNode(target);

                if (canMoveCombined || isWarpNode) {
                    unsigned int col = isWarpNode ? GetColor(255, 100, 255) : (canMoveBase ? GetColor(100, 200, 255) : GetColor(255, 180, 50));
                    DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, col, TRUE);
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
        if (m_psHandle != -1 && m_cbHandle != -1) {
            float* cb = (float*)GetBufferShaderConstantBuffer(m_cbHandle);
            cb[0] = m_shaderTime;
            cb[1] = 1920.0f;
            cb[2] = 1080.0f;
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
            v[0].pos.x = 0;    v[0].pos.y = 0;    v[0].u = 0.0f; v[0].v = 0.0f;
            v[1].pos.x = 1920; v[1].pos.y = 0;    v[1].u = 1.0f; v[1].v = 0.0f;
            v[2].pos.x = 0;    v[2].pos.y = 1080; v[2].u = 0.0f; v[2].v = 1.0f;
            v[3].pos.x = 1920; v[3].pos.y = 0;    v[3].u = 1.0f; v[3].v = 0.0f;
            v[4].pos.x = 1920; v[4].pos.y = 1080; v[4].u = 1.0f; v[4].v = 1.0f;
            v[5].pos.x = 0;    v[5].pos.y = 1080; v[5].u = 0.0f; v[5].v = 1.0f;

            DrawPrimitive2DToShader(v, 6, DX_PRIMTYPE_TRIANGLELIST);
            SetUsePixelShader(-1);
        }
        else {
            DrawBox(0, 0, 1920, 1080, GetColor(10, 12, 18), TRUE);
        }

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
        DrawBox(0, 70, 580, 1080, GetColor(18, 18, 22), TRUE);
        DrawBox(576, 70, 580, 1080, GetColor(255, 165, 0), TRUE);
        DrawLine(575, 70, 575, 1080, GetColor(255, 220, 100), 1);

        DrawBox(1340, 70, 1920, 1080, GetColor(18, 18, 22), TRUE);
        DrawBox(1340, 70, 1344, 1080, GetColor(30, 50, 180), TRUE);
        DrawLine(1344, 70, 1344, 1080, GetColor(100, 150, 255), 1);
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
                DrawString((int)center.x - tw / 2, (int)center.y + 15, label, GetColor(255, 255, 255));
            }
            };

        if (Is1PTurn()) {
            drawWarpNodes(m_player.get(), GetColor(255, 165, 0), "1P ST.");
        }
        else {
            drawWarpNodes(m_enemy.get(), GetColor(60, 150, 255), "2P ST.");
        }

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

        unsigned int phaseCol;
        const char* phaseName;

        if (m_currentPhase == Phase::P1_Move) { phaseCol = GetColor(180, 110, 0); phaseName = m_is1P_NPC ? "1Pのターン (思考中)" : "1Pのターン (移動選択)"; }
        else if (m_currentPhase == Phase::P1_Action) { phaseCol = GetColor(180, 110, 0); phaseName = m_is1P_NPC ? "1Pのターン (思考中)" : "1Pのターン (行動選択)"; }
        else if (m_currentPhase == Phase::P2_Move) { phaseCol = GetColor(30, 50, 120); phaseName = m_is2P_NPC ? "2Pのターン (思考中)" : "2Pのターン (移動選択)"; }
        else if (m_currentPhase == Phase::P2_Action) { phaseCol = GetColor(30, 50, 120); phaseName = m_is2P_NPC ? "2Pのターン (思考中)" : "2Pのターン (行動選択)"; }
        else { phaseCol = GetColor(150, 0, 255); phaseName = "GAME SET!!"; }

        DrawBox(0, 0, 1920, 70, phaseCol, TRUE);
        DrawLine(0, 70, 1920, 70, GetColor(255, 255, 255), 2);

        SetFontSize(38);
        DrawFormatString(40, 16, GetColor(255, 255, 255), ">>> %s", phaseName);
        SetFontSize(24);
        DrawFormatString(800, 24, GetColor(220, 220, 220), "経過ターン: %d", m_mapGrid.GetTotalTurns());

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
            DrawBox(x, scoreY, x + 500, scoreY + 200, darkBg, TRUE);

            if (m_ruleMode == RuleMode::ZERO_ONE) {
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
                SetFontSize(160);
                DrawString(x + 24, scoreY + 14, std::to_string(unit->GetNumber()).c_str(), GetColor(0, 0, 0));
                DrawFormatString(x + 20, scoreY + 10, GetColor(255, 255, 255), "%d", unit->GetNumber());
            }

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
                        if (next == Fraction(m_targetScore)) DrawString(popX + 15, popY + 10, "FINISH !!", GetColor(255, 255, 0));
                        else if (next > Fraction(m_targetScore)) DrawString(popX + 15, popY + 10, "OVER !!", GetColor(255, 150, 0));
                        else DrawFormatString(popX + 15, popY + 10, diffCol, "%s%s", (resFrac.n >= 0) ? " +" : " ", resFrac.ToString().c_str());
                    }
                    else {
                        int intRes = 0;
                        if (aOp == '+') intRes = aNum + tNum; else if (aOp == '-') intRes = aNum - tNum;
                        else if (aOp == '*') intRes = aNum * tNum; else if (aOp == '/') intRes = 0;

                        if (aOp != '/') {
                            int targetCurrentHp = isHoverSelf ? aNum : tNum;
                            int newHpRaw = targetCurrentHp + intRes;
                            int stockChange = 0;
                            int simulatedHp = newHpRaw;
                            while (simulatedHp <= 0) { stockChange--; simulatedHp += 9; }
                            while (simulatedHp > 9) { stockChange++; simulatedHp -= 9; }

                            SetFontSize(24);
                            if (stockChange < 0) {
                                DrawFormatString(popX + 10, popY + 12, GetColor(255, 80, 80), "→ 残機%d ｜ 体力[%d]", stockChange, simulatedHp);
                            }
                            else if (stockChange > 0) {
                                DrawFormatString(popX + 10, popY + 12, GetColor(100, 255, 100), "→ 残機+%d ｜ 体力[%d]", stockChange, simulatedHp);
                            }
                            else {
                                DrawFormatString(popX + 10, popY + 12, GetColor(200, 200, 200), "→ 体力が [%d] に", simulatedHp);
                            }
                        }
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
            DrawFormatString(x + 20, infoY + 65, GetColor(220, 220, 220), "移動可能マス: %d マス", moveDist);

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
        DrawString(1380, ly + 180, " ＋/－/＊ ＝ 計算方向へジャンプ", GetColor(255, 255, 180));
        DrawString(1380, ly + 215, " ÷ ＝ (分子,分母)のマスを【陣地】化", GetColor(255, 100, 255));

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

                    if (aOp == '/') {
                        DrawFormatString(650, py + 112, GetColor(255, 100, 255), "★ さらに座標(%d, %d)を【陣地】として登録", aNum, tNum);
                    }
                }
                else { DrawString(770, py + 82, "どちらに反映しますか", GetColor(120, 120, 130)); }
            }
            else {
                int intRes = 0;
                if (aOp == '+') intRes = aNum + tNum; else if (aOp == '-') intRes = aNum - tNum;
                else if (aOp == '*') intRes = aNum * tNum; else if (aOp == '/') intRes = 0;

                SetFontSize(64);
                DrawFormatString(672, py + 17, GetColor(0, 0, 0), "%d %c %d =>", aNum, aOp, tNum);
                DrawFormatString(670, py + 15, GetColor(255, 255, 255), "%d %c %d =>", aNum, aOp, tNum);

                unsigned int resColor = (intRes < 0) ? GetColor(255, 80, 80) : (intRes > 0 ? GetColor(100, 255, 100) : GetColor(255, 200, 0));
                DrawFormatString(1032, py + 17, GetColor(0, 0, 0), "%s%d", (intRes > 0 ? "+" : ""), intRes);
                DrawFormatString(1030, py + 15, resColor, "%s%d", (intRes > 0 ? "+" : ""), intRes);

                SetFontSize(24);
                if (isHoverSelf || isHoverEnemy) {
                    std::string targetName = isHoverSelf ? (activeActor == m_player.get() ? "1P" : "2P") : (activeTarget == m_player.get() ? "1P" : "2P");

                    int targetCurrentHp = isHoverSelf ? aNum : tNum;
                    int newHpRaw = targetCurrentHp + intRes;
                    int stockChange = 0;
                    int simulatedHp = newHpRaw;
                    while (simulatedHp <= 0) { stockChange--; simulatedHp += 9; }
                    while (simulatedHp > 9) { stockChange++; simulatedHp -= 9; }

                    if (stockChange < 0) {
                        DrawFormatString(620, py + 82, GetColor(255, 100, 100), "▼ 攻撃！ %s の【 残機を%d 】、体力を [%d] にします", targetName.c_str(), stockChange, simulatedHp);
                    }
                    else if (stockChange > 0) {
                        DrawFormatString(620, py + 82, GetColor(100, 255, 100), "▼ 回復！ %s の【 残機を+%d 】、体力を [%d] にします", targetName.c_str(), stockChange, simulatedHp);
                    }
                    else {
                        DrawFormatString(620, py + 82, GetColor(220, 220, 220), "▼ 変化！ %s の体力を [%d] に変更", targetName.c_str(), simulatedHp);
                    }

                    if (aOp == '/') {
                        DrawFormatString(620, py + 112, GetColor(255, 100, 255), "★ さらに座標(%d, %d)を【陣地】として登録", aNum, tNum);
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

        if (m_currentPhase == Phase::P1_Action || m_currentPhase == Phase::P2_Action) {
            if (m_currentPhase == Phase::P1_Action && m_is1P_NPC) return;
            if (m_currentPhase == Phase::P2_Action && m_is2P_NPC) return;

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

        // ==========================================
        // ★ 決着時の GAME SET 演出
        // ==========================================
        if (m_currentPhase == Phase::FINISH) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
            DrawBox(0, 0, 1920, 1080, GetColor(0, 0, 0), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            SetFontSize(160);
            const char* finishText = "GAME SET !!";
            int textW = GetDrawStringWidth(finishText, (int)strlen(finishText));
            DrawString(1920 / 2 - textW / 2, 1080 / 2 - 100, finishText, GetColor(255, 255, 255));

            if (m_finishTimer > 60) {
                SetFontSize(40);
                const char* nextText = ">> CLICK TO NEXT <<";
                int nw = GetDrawStringWidth(nextText, (int)strlen(nextText));
                DrawString(1920 / 2 - nw / 2, 1080 / 2 + 100, nextText, GetColor(200, 200, 200));
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