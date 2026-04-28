#define NOMINMAX
#include "BattleMaster.h"
#include <DxLib.h>
#include <cmath>
#include <random>
#include <algorithm>
#include <unordered_map>
#include "../Input/InputManager.h"
#include "../Scene/SceneManager.h" 
#include "../../CyberGrid.h" 
#include "ProceduralAudio.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ==========================================
// マジックナンバーを排除するための定数・色定義
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
	inline unsigned int COL_TEXT_OFF() { return GetColor(120, 120, 120); }

    std::mt19937 g_rng(std::random_device{}());

    // ========================================================
    // 超絶軽量化の要：フォントキャッシュシステム！
    // ========================================================
    int GetCachedFont(int size) {
        static std::unordered_map<int, int> s_fontCache;
        if (s_fontCache.find(size) == s_fontCache.end()) {
            s_fontCache[size] = CreateFontToHandle("BIZ UDゴシック", size, 2, DX_FONTTYPE_ANTIALIASING);
        }
        return s_fontCache[size];
    }
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

        if (m_actionLog.size() > 100) {
            m_actionLog.erase(m_actionLog.begin());
        }

        m_logScrollOffset = std::max(0, (int)m_actionLog.size() - 6);
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
        m_logScrollOffset = 0;
        m_uiCursorX_1P = 0.0f;
        m_uiCursorX_2P = 0.0f;

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
        std::string rModeStr = (m_ruleMode == RuleMode::CLASSIC) ? "ノーマル" : "カウント";
        AddLog(">>> バトル開始！ [1P:" + std::string(m_is1P_NPC ? "COM" : "PLAYER") + " vs 2P:" + std::string(m_is2P_NPC ? "COM" : "PLAYER") + " / " + rModeStr + "]");

        if (m_ruleMode == RuleMode::ZERO_ONE) AddLog(">>> 目標：相手よりはやくスコアを【 " + std::to_string(m_targetScore) + " 】にしよう！");
        else AddLog(">>> 目標：相手の残機をなくして、勝利を目指そう！");
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
            if (m_hoverGrid == pos) {
                m_isPlayerSelected = true;
                ProceduralAudio::GetInstance().PlayPowerSE(3);
            }
            return;
        }

        if (m_hoverGrid == pos) {
            activeUnit.AddNumber(-1);
            m_isPlayerSelected = false;
            m_currentPhase = nextPhase;
            ProceduralAudio::GetInstance().PlayPowerSE(1);
            return;
        }

        int cost = 0;
        if (!m_mapGrid.IsWithinBounds(m_hoverGrid.x, m_hoverGrid.y) ||
            !CanMove(activeUnit.GetNumber(), activeUnit.GetOp(), pos, m_hoverGrid, cost)) {
            m_isPlayerSelected = false;
            ProceduralAudio::GetInstance().PlayErrorSE();
            return;
        }

        // ==========================================
          // プレイヤー移動時のログ出力(HandleMoveInput内)
          // ==========================================
        ProceduralAudio::GetInstance().PlayPowerSE(5);
        activeUnit.AddNumber(-cost);
        std::queue<Vector2> autoPath;
        int dx = std::abs(m_hoverGrid.x - pos.x);
        int dy = std::abs(m_hoverGrid.y - pos.y);
        m_totalMoves += std::max(dx, dy);

        std::string myName = Is1PTurn() ? "1P" : "2P";

        if (activeUnit.HasWarpNode(m_hoverGrid) || (dx != dy && dx != 0 && dy != 0)) {
            autoPath.push(m_mapGrid.GetCellCenter(m_hoverGrid.x, m_hoverGrid.y));
            //詳細ログ
            AddLog("【跳躍】 " + myName + " が (" + std::to_string(pos.x + 1) + "," + std::to_string(9 - pos.y) + ") から (" + std::to_string(m_hoverGrid.x + 1) + "," + std::to_string(9 - m_hoverGrid.y) + ") へワープ！ (パワー -1)");
            ProceduralAudio::GetInstance().PlayPowerSE(8);
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
            
            AddLog("【移動】 " + myName + " が (" + std::to_string(m_hoverGrid.x + 1) + "," + std::to_string(9 - m_hoverGrid.y) + ") へ移動(パワー -" + std::to_string(cost) + ")");
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

            if (pickedItem == '+') ProceduralAudio::GetInstance().PlayPowerSE(5);
            else if (pickedItem == '-') ProceduralAudio::GetInstance().PlayPowerSE(2);
            else if (pickedItem == '*') ProceduralAudio::GetInstance().PlayPowerSE(7);
            else if (pickedItem == '/') ProceduralAudio::GetInstance().PlayPowerSE(9);
        }

        IntVector2 targetPos = targetUnit.GetGridPos();
        bool canAttack = (std::abs(pos.x - targetPos.x) + std::abs(pos.y - targetPos.y) == 1);
        bool hasOp = (actor.GetOp() != '\0');
        Phase nextTurnPhase = (&actor == m_player.get()) ? Phase::P2_Move : Phase::P1_Move;

        if (!canAttack || !hasOp) {
            AddLog("【待機】 行動を終了");
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
            ProceduralAudio::GetInstance().PlayPowerSE(6);
            ExecuteBattle(actor, targetUnit, actor); actionDone = true;
        }
        else if (CheckButtonClick(850, 960, 220, 60, mousePos) || hoverGrid == targetPos || (CheckButtonClick(40, 100, 500, 650, mousePos) && !is1P) || (CheckButtonClick(1380, 100, 500, 650, mousePos) && is1P)) {
            ProceduralAudio::GetInstance().PlayPowerSE(6);
            ExecuteBattle(actor, targetUnit, targetUnit); actionDone = true;
        }
        else if (CheckButtonClick(1100, 960, 220, 60, mousePos)) {
            ProceduralAudio::GetInstance().PlayPowerSE(2);
            AddLog("【待機】 行動を終了"); actionDone = true;
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

            if (pickedItem == '+') ProceduralAudio::GetInstance().PlayPowerSE(5);
            else if (pickedItem == '-') ProceduralAudio::GetInstance().PlayPowerSE(2);
            else if (pickedItem == '*') ProceduralAudio::GetInstance().PlayPowerSE(7);
            else if (pickedItem == '/') ProceduralAudio::GetInstance().PlayPowerSE(9);
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
                if (myOp == '/') targetMeIsBetter = true;
                else {
                    Fraction goal(m_targetScore);
                    Fraction myScoreNow = is1P ? m_p1ZeroOneScore : m_p2ZeroOneScore;
                    Fraction enScoreNow = is1P ? m_p2ZeroOneScore : m_p1ZeroOneScore;

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
            else {
                if (myOp == '/') targetMeIsBetter = true;
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

            ProceduralAudio::GetInstance().PlayPowerSE(6);
            if (targetMeIsBetter) ExecuteBattle(*me, *opp, *me);
            else ExecuteBattle(*me, *opp, *opp);
        }
        else {
            AddLog("【待機】 " + myName + " は行動を完了");
            ProceduralAudio::GetInstance().PlayPowerSE(2);
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

        auto updateCursor = [](float& currentX, int targetNum) {
            float targetX = 40.0f + (targetNum - 1) * 48.0f;
            if (currentX == 0.0f) currentX = targetX;
            currentX += (targetX - currentX) * 0.2f;
            };

        if (m_player) updateCursor(m_uiCursorX_1P, m_player->GetNumber());
        if (m_enemy)  updateCursor(m_uiCursorX_2P, m_enemy->GetNumber());

        auto& input = InputManager::GetInstance();
        if (m_player) m_player->Update();
        if (m_enemy)  m_enemy->Update();
        m_hoverGrid = m_mapGrid.ScreenToGrid(input.GetMousePos());

        // ==========================================
        //  ログパネルのホイールスクロール処理
        // ==========================================
        int wheel = GetMouseWheelRotVol(); // マウスホイールの回転量を取得
        if (wheel != 0) {
            Vector2 mPos = input.GetMousePos();
            // ログパネルの矩形範囲内にマウスがあるか判定
            if (mPos.x >= 40 && mPos.x <= 540 && mPos.y >= LOG_PANEL_Y && mPos.y <= LOG_PANEL_Y + 200) {
                m_logScrollOffset -= wheel; // 上スクロール(正)で過去に戻る

                int maxOffset = std::max(0, (int)m_actionLog.size() - 6);
                if (m_logScrollOffset < 0) m_logScrollOffset = 0;
                if (m_logScrollOffset > maxOffset) m_logScrollOffset = maxOffset;
            }
        }

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
        // ==========================================
        // 右上のPAUSEボタンのホバー・クリック判定
        // ==========================================
        int pauseBtnW = 160;
        int pauseBtnH = 50;
        int pauseBtnX = SCREEN_W - pauseBtnW - 20; // 右上
        int pauseBtnY = 10;

        if (CheckButtonClick(pauseBtnX, pauseBtnY, pauseBtnW, pauseBtnH, input.GetMousePos())) {
            static Vector2 prevM = input.GetMousePos();
            if (prevM.x != input.GetMousePos().x || prevM.y != input.GetMousePos().y) {
                ProceduralAudio::GetInstance().PlayPowerSE(2); // ホバー音
            }
            prevM = input.GetMousePos();

            if (input.IsMouseLeftTrg()) {
                SceneManager::GetInstance()->TogglePause(); // スイッチを押す！
                return; // 即閉じ防止のためUpdateを抜ける
            }
        }


        if (IsGameOver() && m_currentPhase != Phase::FINISH) {
            m_currentPhase = Phase::FINISH;
            m_finishTimer = 0;
            m_effectIntensity = 1.0f;
            ProceduralAudio::GetInstance().PlayPowerSE(9);
            ProceduralAudio::GetInstance().PlayErrorSE();
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
     
        int actualCost = isStay ? 1 : selectedCost;
        if (me->HasWarpNode(bestTarget)) actualCost = 1;

        if (me->HasWarpNode(bestTarget)) {
            AddLog("【跳躍】 " + myName + " がワープを起動し (" + std::to_string(bestTarget.x + 1) + "," + std::to_string(9 - bestTarget.y) + ") に移動！ (POWER -1)");
            ProceduralAudio::GetInstance().PlayPowerSE(8);
        }
        else if (isStay) {
            AddLog("【待機】 " + myName + " はその場で動かずパワーを消費しました。 (パワー -1)");
            ProceduralAudio::GetInstance().PlayPowerSE(1);
        }
        else {
            AddLog("【移動】 " + myName + " が (" + std::to_string(bestTarget.x + 1) + "," + std::to_string(9 - bestTarget.y) + ") へ移動(パワー -" + std::to_string(actualCost) + ")");
            ProceduralAudio::GetInstance().PlayPowerSE(5);
        }

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
            score += predictedStocks * 20000;
            score -= enemy.GetStocks() * 20000;
            score += predictedHp * 100;

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

        std::uniform_int_distribution<int> noiseDist(-500, 500);

        for (int x = 0; x < 9; ++x) {
            for (int y = 0; y < 9; ++y) {
                IntVector2 target{ x, y };
                int cost = 0;

                bool canGo = (target == myPos);
                if (!canGo) {
                    canGo = CanMove(currentNum, currentOp, myPos, target, cost);
                }
                else {
                    cost = 1;
                }

                if (canGo) {
                    int virtualNextNum = currentNum - cost;
                    while (virtualNextNum <= 0) virtualNextNum += 9;

                    me->StartMove(target, std::queue<Vector2>());
                    int eval = EvaluateBoard(*me, virtualNextNum, *opp, target, is1P);

                    int noise = noiseDist(g_rng);
                    eval += noise;

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
                ProceduralAudio::GetInstance().PlayPowerSE(9);
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

        // ==========================================
         // 計算式のログ出力(ExecuteBattle内)
         // ==========================================
        std::string aName = (&attacker == m_player.get()) ? "1P" : "2P";

        if (aOp != '/') {
            std::string eqStr = std::to_string(aNum) + " " + std::string(1, aOp) + " " + std::to_string(dNum) + " = " + std::to_string(intRes);
            AddLog("【計算】 " + aName + " が計算を実行！ [ " + eqStr + " ]");
            ProceduralAudio::GetInstance().PlayPowerSE(4);
        }
        else {
            ProceduralAudio::GetInstance().PlayPowerSE(4);
        }

        m_totalOps++;
        if (std::abs(intRes) > m_maxDamage) m_maxDamage = std::abs(intRes);

        ApplyBattleResult(target, resFrac, intRes, aOp);
        attacker.SetOp('\0');

        AddLog("----------------------------------------");
    }

    // ==========================================
    // 反映結果のログ出力
    // ==========================================
    void BattleMaster::ApplyBattleResult(UnitBase& unit, const Fraction& resultFrac, int intRes, char op) {
        std::string targetName = (&unit == m_player.get()) ? "1P" : "2P";


        if (m_ruleMode == RuleMode::ZERO_ONE) {// カウントバトル
            Fraction& currentScore = (&unit == m_player.get()) ? m_p1ZeroOneScore : m_p2ZeroOneScore;
            Fraction goalScore(m_targetScore);

            if (op != '/') {
                Fraction predictedScore = currentScore + resultFrac;
                std::string prevScoreStr = currentScore.ToString();

                if (predictedScore == goalScore) {
                    currentScore = predictedScore;
                    AddLog("【反映】 " + targetName + " のスコアに適用！ [" + prevScoreStr + " -> " + currentScore.ToString() + "]");
                    AddLog("【ぴったり!!】 " + targetName + " が目標スコアにピッタリ到達！！");
                }
                else if (predictedScore > goalScore) {
                    Fraction excess = predictedScore - goalScore;
                    currentScore = goalScore - excess;
                    AddLog("【反映】 " + targetName + " のスコアに適用！ [" + prevScoreStr + " -> " + predictedScore.ToString() + "]");
                    AddLog("【オーバー】 目標を超過！スコアが [" + currentScore.ToString() + "] までバウンス");
                }
                else {
                    currentScore = predictedScore;
                    AddLog("【反映】 " + targetName + " のスコアに適用！ [" + prevScoreStr + " -> " + currentScore.ToString() + "]");
                }
            }
            else {
                AddLog("【反映】 割り算のためスコア加算はスキップされました。");
            }

            int cycleValue = (intRes - 1) % 9;
            if (cycleValue < 0) cycleValue += 9;
            int finalNum = cycleValue + 1;

            AddLog("【反映】 " + targetName + " のパワーが [" + std::to_string(finalNum) + "] に再設定されました。");

            unit.SetNumber(finalNum);
            ProceduralAudio::GetInstance().PlayPowerSE(finalNum);
        }
        else { // ノーマルバトル
            if (op != '/') {
                int currentHp = unit.GetNumber();
                int newHpRaw = currentHp + intRes;
                int stockChange = 0;

                while (newHpRaw <= 0) { stockChange--; newHpRaw += 9; }
                while (newHpRaw > 9) { stockChange++; newHpRaw -= 9; }

                if (stockChange < 0) {
                    unit.AddStocks(stockChange);
                    AddLog("【反映】 " + targetName + " に攻撃！バッテリーが " + std::to_string(std::abs(stockChange)) + " 破壊！");
                    ProceduralAudio::GetInstance().PlayErrorSE();
                }
                else if (stockChange > 0) {
                    unit.AddStocks(stockChange);
                    AddLog("【反映】 " + targetName + " がバッテリーを " + std::to_string(stockChange) + " つ過剰(回復)！");
                    ProceduralAudio::GetInstance().PlayPowerSE(8);
                }
                else {
                    AddLog("【反映】 " + targetName + " のパワーに適用");
                }

                unit.SetNumber(newHpRaw);

                int soundNum = newHpRaw;
                while (soundNum <= 0) { soundNum += 9; }
                while (soundNum > 9) { soundNum -= 9; }

                AddLog("【着地】 " + targetName + " のパワーは [" + std::to_string(soundNum) + "] に");
                ProceduralAudio::GetInstance().PlayPowerSE(soundNum);
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

                DrawFormatStringToHandle((int)hc.x - 10, (int)hc.y - 20, COL_DANGER(), GetCachedFont(20), "-%d", displayCost);

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

                int font18 = GetCachedFont(18);
                int tw = GetDrawStringWidthToHandle(label, (int)strlen(label), font18);
                DrawStringToHandle((int)center.x - tw / 2, (int)center.y + 15, label, COL_TEXT_MAIN(), font18);
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
        else { phaseCol = COL_INFO(); phaseName = "終了！！"; }

        DrawBox(0, 0, SCREEN_W, HEADER_H, phaseCol, TRUE);
        DrawLine(0, HEADER_H, SCREEN_W, HEADER_H, COL_TEXT_MAIN(), 2);

        DrawFormatStringToHandle(40, 16, COL_TEXT_MAIN(), GetCachedFont(38), ">>> %s", phaseName);
        DrawFormatStringToHandle(800, 24, COL_TEXT_SUB(), GetCachedFont(24), "経過ターン: %d", m_mapGrid.GetTotalTurns());

        // ==========================================
        // 事前計算エリア (UI描画前にすべてのプレビュー値を計算)
        // ==========================================
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

        int intRes = 0;
        Fraction resFrac(0);
        bool isCleanDivide = true;

        if (showCalcPanel) {
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
        }

        // ==========================================
        // ユニットカードへのプレビュー値伝達
        // ==========================================
        int p1PreviewNum = -1; int p1PreviewStocks = m_player ? m_player->GetStocks() : 0;
        int p2PreviewNum = -1; int p2PreviewStocks = m_enemy ? m_enemy->GetStocks() : 0;

        Fraction p1PreviewScore = m_p1ZeroOneScore; bool p1HasScorePreview = false;
        Fraction p2PreviewScore = m_p2ZeroOneScore; bool p2HasScorePreview = false;

        // ① 移動によるプレビュー
        if (isMovePreview) {
            if (activeActor == m_player.get()) {
                int temp = m_player->GetNumber() - previewCost;
                p1PreviewStocks = m_player->GetStocks();
                while (temp <= 0) { p1PreviewStocks--; temp += 9; }
                p1PreviewNum = (p1PreviewStocks < 0) ? 0 : temp;
            }
            else if (activeActor == m_enemy.get()) {
                int temp = m_enemy->GetNumber() - previewCost;
                p2PreviewStocks = m_enemy->GetStocks();
                while (temp <= 0) { p2PreviewStocks--; temp += 9; }
                p2PreviewNum = (p2PreviewStocks < 0) ? 0 : temp;
            }
        }

        // ② 計算実行(ホバー時)によるプレビュー
        if (showCalcPanel && (isHoverSelf || isHoverEnemy)) {
            bool applyTo1P = (isHoverSelf && activeActor == m_player.get()) || (!isHoverSelf && activeTarget == m_player.get());

			if (m_ruleMode == RuleMode::CLASSIC) {// ノーマルバトルのプレビュー
                if (disp_aOp != '/') {
                    int targetCurrentHp = isHoverSelf ? disp_aNum : disp_tNum;
                    int currentStocks = applyTo1P ? m_player->GetStocks() : m_enemy->GetStocks();

                    int newHpRaw = targetCurrentHp + intRes;
                    int stockChange = 0;
                    int simulatedHp = newHpRaw;
                    while (simulatedHp <= 0) { stockChange--; simulatedHp += 9; }
                    while (simulatedHp > 9) { stockChange++; simulatedHp -= 9; }

                    int finalStocks = currentStocks + stockChange;
                    int finalNum = (finalStocks < 0) ? 0 : simulatedHp;

                    if (applyTo1P) { p1PreviewNum = finalNum; p1PreviewStocks = finalStocks; }
                    else { p2PreviewNum = finalNum; p2PreviewStocks = finalStocks; }
                }
            }
			else { // カウントバトルのプレビュー
                if (disp_aOp != '/') {
                    Fraction goal(m_targetScore);
                    Fraction currentScore = applyTo1P ? m_p1ZeroOneScore : m_p2ZeroOneScore;
                    Fraction nextScore = currentScore + resFrac;

                    if (nextScore > goal) nextScore = goal - (nextScore - goal); // オーバーバウンス処理

                    if (applyTo1P) { p1PreviewScore = nextScore; p1HasScorePreview = true; }
                    else { p2PreviewScore = nextScore; p2HasScorePreview = true; }

                    // POWER (数字) の着地も計算
                    int cycleValue = (intRes - 1) % 9;
                    if (cycleValue < 0) cycleValue += 9;
                    int finalNum = cycleValue + 1;

                    if (applyTo1P) p1PreviewNum = finalNum;
                    else p2PreviewNum = finalNum;
                }
            }
        }

        // ワープ設置の描画(背景サークル)
        char currentPreviewOp = (showCalcPanel && disp_aOp == '/') ? '/' : '\0';
        if (currentPreviewOp == '/') {
            int wx = disp_aNum - 1;
            int wy = 9 - disp_tNum;
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

            int f18 = GetCachedFont(18);
            const char* lbl = "結果";
            int tw = GetDrawStringWidthToHandle(lbl, (int)strlen(lbl), f18);
            DrawStringToHandle((int)center.x - tw / 2, (int)center.y + 15, lbl, COL_TEXT_MAIN(), f18);
        }

        // ==========================================
        // ユニットカード描画
        // ==========================================
        auto drawUnitCard = [&](int x, int y, UnitBase* unit, bool is1P, int p_previewNum, int p_previewStocks, Fraction p_previewScore, bool p_hasScorePreview) {
            if (!unit) return;
            unsigned int baseCol = is1P ? COL_P1() : COL_P2();
            std::string headerName = is1P ? (m_is1P_NPC ? "1P (COM)" : "1P PLAYER") : (m_is2P_NPC ? "2P (COM)" : "2P PLAYER");

            if ((is1P && Is1PTurn()) || (!is1P && !Is1PTurn() && m_currentPhase != Phase::FINISH)) {
                SetDrawBlendMode(DX_BLENDMODE_ADD, (int)(100 + 50 * sin(GetNowCount() / 200.0)));
                DrawBox(x - 5, y - 5, x + 505, y + 490, baseCol, FALSE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }

            DrawStringToHandle(x + 5, y + 5, headerName.c_str(), baseCol, GetCachedFont(36));
            DrawLine(x, y + 45, x + 500, y + 45, baseCol, 2);

            int scoreY = y + 70;

            if (m_ruleMode == RuleMode::ZERO_ONE) {
                DrawBox(x, scoreY, x + 500, scoreY + 140, COL_DARK_BG(), TRUE);
                Fraction f = is1P ? m_p1ZeroOneScore : m_p2ZeroOneScore;

                DrawStringToHandle(x + 10, scoreY + 5, "現在のスコア", COL_TEXT_SUB(), GetCachedFont(18));

                if (f.d == 1) {
                    int f100 = GetCachedFont(100);
                    std::string nStr = std::to_string(f.n);
                    DrawStringToHandle(x + 24, scoreY + 24, nStr.c_str(), COL_TEXT_DARK(), f100);
                    DrawStringToHandle(x + 20, scoreY + 20, nStr.c_str(), COL_TEXT_MAIN(), f100);
                }
                else {
                    unsigned int fracCol = is1P ? GetColor(255, 220, 100) : GetColor(180, 220, 255);
                    int f60 = GetCachedFont(60);
                    std::string nStr = std::to_string(f.n);
                    std::string dStr = std::to_string(f.d);
                    int nw = GetDrawStringWidthToHandle(nStr.c_str(), (int)nStr.length(), f60);
                    int dw = GetDrawStringWidthToHandle(dStr.c_str(), (int)dStr.length(), f60);
                    int maxW = (std::max)(nw, dw);
                    int cx = x + 100;

                    DrawFormatStringToHandle(cx - nw / 2 + 4, scoreY + 16, COL_TEXT_DARK(), f60, "%s", nStr.c_str());
                    DrawFormatStringToHandle(cx - nw / 2, scoreY + 12, fracCol, f60, "%s", nStr.c_str());
                    DrawLine(cx - maxW / 2 - 10, scoreY + 75, cx + maxW / 2 + 10, scoreY + 75, fracCol, 4);
                    DrawFormatStringToHandle(cx - dw / 2 + 4, scoreY + 84, COL_TEXT_DARK(), f60, "%s", dStr.c_str());
                    DrawFormatStringToHandle(cx - dw / 2, scoreY + 80, fracCol, f60, "%s", dStr.c_str());
                }

                // スコアプレビューのUI表示！
                if (p_hasScorePreview && !(p_previewScore.n == f.n && p_previewScore.d == f.d)) {
                    int f60 = GetCachedFont(60);
                    int f40 = GetCachedFont(40);
                    int f20 = GetCachedFont(20);
                    unsigned int previewCol = (p_previewScore.n == m_targetScore) ? COL_WARN() :
                        (p_previewScore.n < f.n ? COL_DANGER() : COL_SAFE());

                    int arrowX = x + 230; // 矢印のX座標

                    int blinkAlpha = (int)(150 + 100 * std::sin(GetNowCount() / 100.0));
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, blinkAlpha);
                    DrawStringToHandle(arrowX, scoreY + 45, "→", COL_TEXT_SUB(), f40);

                    if (p_previewScore.d == 1) {
                        std::string pStr = std::to_string(p_previewScore.n);
                        DrawStringToHandle(arrowX + 54, scoreY + 34, pStr.c_str(), COL_TEXT_DARK(), f60);
                        DrawStringToHandle(arrowX + 50, scoreY + 30, pStr.c_str(), previewCol, f60);
                    }
                    else {
                        std::string pnStr = std::to_string(p_previewScore.n);
                        std::string pdStr = std::to_string(p_previewScore.d);
                        int f30 = GetCachedFont(30);
                        DrawFormatStringToHandle(arrowX + 60, scoreY + 20, previewCol, f30, "%s", pnStr.c_str());
                        DrawLine(arrowX + 55, scoreY + 60, arrowX + 110, scoreY + 60, previewCol, 3);
                        DrawFormatStringToHandle(arrowX + 60, scoreY + 65, previewCol, f30, "%s", pdStr.c_str());
                    }
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

                    // 結果のテキスト表示
                    if (p_previewScore.n == m_targetScore) {
                        //DrawStringToHandle(arrowX + 50, scoreY + 100, "+", COL_WARN(), f20);
                    }
                    else if (p_previewScore.n < f.n) {
                        //DrawStringToHandle(arrowX + 50, scoreY + 100, "－", COL_DANGER(), f20);
                    }
                    else {
                        DrawFormatStringToHandle(arrowX + 50, scoreY + 100, COL_SAFE(), f20, "%+d 点", p_previewScore.n - f.n);
                    }
                }

                int targetBoxY = scoreY + 140;
                DrawBox(x, targetBoxY, x + 500, targetBoxY + 35, GetColor(20, 30, 40), TRUE);
                DrawBox(x, targetBoxY, x + 500, targetBoxY + 35, COL_INFO(), FALSE);

                DrawStringToHandle(x + 15, targetBoxY + 8, "目標スコア", COL_INFO(), GetCachedFont(20));

                int f32 = GetCachedFont(32);
                DrawFormatStringToHandle(x + 390, targetBoxY + 2, COL_TEXT_DARK(), f32, "000");
                DrawFormatStringToHandle(x + 390, targetBoxY + 2, COL_SAFE(), f32, "%03d", m_targetScore);
            }
            else {
                DrawBox(x, scoreY, x + 500, scoreY + 35, COL_DARK_BG(), TRUE);
                DrawStringToHandle(x + 15, scoreY + 8, "ノーマルバトル", COL_TEXT_SUB(), GetCachedFont(20));
            }

            int powerY = scoreY + (m_ruleMode == RuleMode::ZERO_ONE ? 190 : 50);
            DrawBox(x, powerY, x + 500, powerY + 120, COL_DARK_BG(), TRUE);

            DrawStringToHandle(x + 15, powerY + 10, "パワー", COL_WARN(), GetCachedFont(24));
            DrawLine(x + 100, powerY + 24, x + 490, powerY + 24, GetColor(60, 60, 70), 1);

            int currentNum = unit->GetNumber();
            int preview = (p_previewNum != -1) ? p_previewNum : currentNum;
            float cursorX = is1P ? m_uiCursorX_1P : m_uiCursorX_2P;
            if (cursorX == 0.0f) cursorX = 40.0f + (currentNum - 1) * 48.0f;

            DrawBox(x + 10, powerY + 40, x + 490, powerY + 90, GetColor(10, 10, 15), TRUE);
            DrawLine(x + 10, powerY + 65, x + 490, powerY + 65, GetColor(30, 30, 40), 1);

            bool isDeadPreview = (p_previewStocks < 0) && (m_ruleMode == RuleMode::CLASSIC);
            unsigned int frameCol = isDeadPreview ? COL_DANGER() : baseCol;

            int hx = x + (int)cursorX;
            int hy = powerY + 29;
            int fw = 56;
            int fh = 70;

            SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
            DrawBox(hx - fw / 2, hy, hx + fw / 2, hy + fh, frameCol, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            int cl = 12; int thick = 3;
            DrawBox(hx - fw / 2, hy, hx - fw / 2 + cl, hy + thick, frameCol, TRUE);
            DrawBox(hx - fw / 2, hy, hx - fw / 2 + thick, hy + cl, frameCol, TRUE);
            DrawBox(hx + fw / 2 - cl, hy, hx + fw / 2, hy + thick, frameCol, TRUE);
            DrawBox(hx + fw / 2 - thick, hy, hx + fw / 2, hy + cl, frameCol, TRUE);
            DrawBox(hx - fw / 2, hy + fh - thick, hx - fw / 2 + cl, hy + fh, frameCol, TRUE);
            DrawBox(hx - fw / 2, hy + fh - cl, hx - fw / 2 + thick, hy + fh, frameCol, TRUE);
            DrawBox(hx + fw / 2 - cl, hy + fh - thick, hx + fw / 2, hy + fh, frameCol, TRUE);
            DrawBox(hx + fw / 2 - thick, hy + fh - cl, hx + fw / 2, hy + fh, frameCol, TRUE);

            int f32Num = GetCachedFont(32);
            int f16Tri = GetCachedFont(16);

            for (int i = 1; i <= 9; ++i) {
                int px = x + 40 + (i - 1) * 48;
                int py = powerY + 43;
                std::string numStr = std::to_string(i);

                float targetX = 40.0f + (i - 1) * 48.0f;
                float distance = std::abs(cursorX - targetX);

                float currentYOff = 6.0f;
                unsigned int numCol = GetColor(70, 70, 80);

                if (distance < 48.0f && !isDeadPreview) {
                    float ratio = 1.0f - (distance / 48.0f);
                    ratio = std::sin(ratio * M_PI / 2.0f);
                    currentYOff = 6.0f - (20.0f * ratio);
                    if (ratio > 0.8f) numCol = COL_TEXT_MAIN();
                    else numCol = COL_TEXT_SUB();
                }

                if (i == preview && preview != currentNum && !isDeadPreview) {
                    currentYOff = -14.0f;
                    numCol = COL_SAFE();
                    int twTriangle = GetDrawStringWidthToHandle("▲", 2, f16Tri);
                    DrawStringToHandle(px - twTriangle / 2, py + 26, "▲", COL_SAFE(), f16Tri);
                }

                int tw = GetDrawStringWidthToHandle(numStr.c_str(), 1, f32Num);
                DrawStringToHandle(px - tw / 2, py + (int)currentYOff, numStr.c_str(), numCol, f32Num);
            }

            if (p_previewNum != -1 && (p_previewNum != currentNum || p_previewStocks != unit->GetStocks())) {
                int f20 = GetCachedFont(20);
                int textY = powerY + 130;
                if (m_ruleMode == RuleMode::CLASSIC) {
                    if (isDeadPreview) {
                        DrawFormatStringToHandle(x + 15, textY, COL_DANGER(), f20, "▼ 警告 : バッテリー枯渇【 破壊 】！");
                    }
                    else if (p_previewStocks != -1 && p_previewStocks < unit->GetStocks()) {
                        DrawFormatStringToHandle(x + 15, textY, COL_WARN(), f20, "▼ バッテリー消費 : %d → %d", currentNum, preview);
                    }
                    else if (p_previewStocks != -1 && p_previewStocks > unit->GetStocks()) {
                        DrawFormatStringToHandle(x + 15, textY, COL_SAFE(), f20, "▲ 超過でバッテリー回復！ : %d → %d", currentNum, preview);
                    }
                    else {
                        DrawFormatStringToHandle(x + 15, textY, COL_SAFE(), f20, "反映後プレビュー : %d → %d", currentNum, preview);
                    }
                }
                else {
                    DrawFormatStringToHandle(x + 15, textY, COL_SAFE(), f20, "反映後パワー : %d → %d", currentNum, preview);
                }
            }

            int infoY = powerY + 160;
            DrawBox(x, infoY, x + 500, infoY + 120, COL_DARK_BG(), TRUE);
            int f22 = GetCachedFont(22);

            if (m_ruleMode == RuleMode::CLASSIC) {
                DrawStringToHandle(x + 20, infoY + 18, "バッテリー", GetColor(180, 180, 180), f22);

                int currentStocks = unit->GetStocks();
                int previewStocks = (p_previewStocks != -1) ? p_previewStocks : currentStocks;

                for (int i = 0; i < unit->GetMaxStocks(); ++i) {
                    int sx = x + 130 + i * 55;
                    int sy = infoY + 8;
                    int sw = 40;
                    int sh = 38;

                    if (i < currentStocks && i >= previewStocks) {
                        int blinkAlpha = (int)(120 + 120 * std::sin(GetNowCount() / 50.0));
                        SetDrawBlendMode(DX_BLENDMODE_ADD, blinkAlpha);
                        DrawBox(sx - 4, sy - 4, sx + sw + 4, sy + sh + 4, COL_DANGER(), TRUE);
                        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

                        DrawBox(sx, sy, sx + sw, sy + sh, GetColor(200, 50, 50), TRUE);
                        DrawBox(sx + sw, sy + 8, sx + sw + 5, sy + sh - 8, GetColor(200, 50, 50), TRUE);
                        DrawStringToHandle(sx + 4, sy - 20, "消費!", COL_DANGER(), f16Tri);
                    }
                    else if (i >= currentStocks && i < previewStocks) {
                        int blinkAlpha = (int)(120 + 120 * std::sin(GetNowCount() / 50.0));
                        SetDrawBlendMode(DX_BLENDMODE_ADD, blinkAlpha);
                        DrawBox(sx - 4, sy - 4, sx + sw + 4, sy + sh + 4, COL_SAFE(), TRUE);
                        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

                        DrawBox(sx, sy, sx + sw, sy + sh, GetColor(50, 200, 100), TRUE);
                        DrawBox(sx + sw, sy + 8, sx + sw + 5, sy + sh - 8, GetColor(50, 200, 100), TRUE);
                        DrawStringToHandle(sx + 4, sy - 20, "回復!", COL_SAFE(), f16Tri);
                    }
                    else if (i < currentStocks) {
                        DrawBox(sx, sy, sx + sw, sy + sh, baseCol, TRUE);
                        DrawBox(sx + sw, sy + 8, sx + sw + 5, sy + sh - 8, baseCol, TRUE);
                        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
                        DrawBox(sx + 3, sy + 3, sx + sw - 3, sy + 12, GetColor(255, 255, 255), TRUE);
                        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                    }
                    else {
                        DrawBox(sx, sy, sx + sw, sy + sh, GetColor(25, 25, 30), TRUE);
                        DrawBox(sx, sy, sx + sw, sy + sh, GetColor(60, 60, 70), FALSE);
                        DrawBox(sx + sw, sy + 8, sx + sw + 5, sy + sh - 8, GetColor(60, 60, 70), FALSE);
                        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
                        DrawLine(sx + 6, sy + 6, sx + sw - 6, sy + sh - 6, GetColor(150, 50, 50), 3);
                        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                    }
                }
            }

            int moveDist = 3 - ((unit->GetNumber() - 1) % 3);
            if (p_previewNum != -1 && p_previewNum != unit->GetNumber()) {
                int nextMoveDist = 3 - ((p_previewNum - 1) % 3);
                DrawFormatStringToHandle(x + 20, infoY + 65, COL_TEXT_SUB(), f22, "移動可能マス: %d → %d マス", moveDist, nextMoveDist);
            }
            else {
                DrawFormatStringToHandle(x + 20, infoY + 65, COL_TEXT_SUB(), f22, "移動可能マス: %d マス", moveDist);
            }

            int ix = x + 350, iy = infoY + 15;
            DrawStringToHandle(ix - 5, iy, "【移動範囲】", COL_DISABLE(), GetCachedFont(18));
            int targetNumForGrid = (p_previewNum != -1) ? p_previewNum : unit->GetNumber();
            unsigned int gridBaseCol = (p_previewNum != -1) ? COL_SAFE() : baseCol;

            for (int i = 0; i < 9; ++i) {
                int gx = i % 3, gy = i / 3;
                bool canGo = (i == 4) ? false : (targetNumForGrid <= 3 ? (gx == 1 || gy == 1) : (targetNumForGrid <= 6 ? (gx == gy || gx + gy == 2) : true));
                unsigned int dotCol = canGo ? gridBaseCol : GetColor(40, 40, 50);
                DrawBox(ix + gx * 22, iy + 26 + gy * 22, ix + gx * 22 + 18, iy + 26 + gy * 22 + 18, dotCol, TRUE);
            }
            };

        // 実行：左の1Pと右の2Pを描画！
        drawUnitCard(40, 100, m_player.get(), true, p1PreviewNum, p1PreviewStocks, p1PreviewScore, p1HasScorePreview);
        drawUnitCard(1380, 100, m_enemy.get(), false, p2PreviewNum, p2PreviewStocks, p2PreviewScore, p2HasScorePreview);


        // ==========================================
         // ログパネル描画(スクロール＆スクロールバー対応)
         // ==========================================
        int f22 = GetCachedFont(22);
        int f20 = GetCachedFont(20);

        DrawLine(40, LOG_PANEL_Y, 540, LOG_PANEL_Y, GetColor(60, 60, 70), 1);
        DrawStringToHandle(40, LOG_PANEL_Y + 10, "ログ", COL_DISABLE(), f22);

        int maxLogOffset = std::max(0, (int)m_actionLog.size() - 6);

        // ログが6件以上ある場合はスクロールバーを描画
        if (maxLogOffset > 0) {
            DrawStringToHandle(85, LOG_PANEL_Y + 13, " (マウスホイールでスクロール)", GetColor(100, 100, 120), GetCachedFont(16));

            // スクロールバーの背景溝
            DrawBox(530, LOG_PANEL_Y + 45, 535, LOG_PANEL_Y + 195, GetColor(30, 30, 40), TRUE);

            // つまみの位置を計算して描画
            float scrollRatio = (float)m_logScrollOffset / maxLogOffset;
            int thumbY = LOG_PANEL_Y + 45 + (int)(scrollRatio * (150 - 30)); // 30はつまみの高さ
            DrawBox(530, thumbY, 535, thumbY + 30, GetColor(100, 150, 200), TRUE);
        }

        // オフセットから6件分のログを抽出して描画
        int startIdx = m_logScrollOffset;
        int endIdx = std::min((int)m_actionLog.size(), startIdx + 6);

        for (int i = startIdx; i < endIdx; ++i) {
            int drawY = LOG_PANEL_Y + 45 + (i - startIdx) * 26;
            DrawFormatStringToHandle(40, drawY, GetColor(180, 220, 160), f20, "%s", m_actionLog[i].c_str());
        }

        DrawLine(1380, LOG_PANEL_Y, 1880, LOG_PANEL_Y, GetColor(60, 60, 70), 1);
        DrawStringToHandle(1380, LOG_PANEL_Y + 10, "基本ルール", COL_DISABLE(), f22);
        DrawStringToHandle(1380, LOG_PANEL_Y + 50, " パワーが1, 4, 7 の時 3マス移動", GetColor(255, 200, 100), f22);
        DrawStringToHandle(1380, LOG_PANEL_Y + 90, " パワーが2, 5, 8 の時 2マス移動", GetColor(180, 180, 180), f22);
        DrawStringToHandle(1380, LOG_PANEL_Y + 130, " パワーが3, 6, 9 の時 1マス移動", GetColor(100, 150, 255), f22);
        DrawStringToHandle(1380, LOG_PANEL_Y + 180, "各演算子取得で移動方向追加", GetColor(255, 255, 180), f22);
        DrawStringToHandle(1380, LOG_PANEL_Y + 215, " ÷ は (分子,分母)のマスにワープ設置", COL_INFO(), f22);

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
        DrawBox(600, BOTTOM_PANEL_Y, 1320, 940, COL_BOTTOM_BG(), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        unsigned int calcBorderCol = Is1PTurn() ? COL_P1() : COL_P2();
        DrawLine(600, BOTTOM_PANEL_Y, 1320, BOTTOM_PANEL_Y, calcBorderCol, 2);

        // ==========================================
        // 下部パネルの計算式・プレビュー描画
        // ==========================================
        if (showCalcPanel) {
            int f64 = GetCachedFont(64);
            int f16 = GetCachedFont(16);
            int f20 = GetCachedFont(20);

            if (isPreviewMode) {
                int preY = BOTTOM_PANEL_Y - 26; // パネル枠線の少し上に描画
                if (willGetNewOp) DrawFormatStringToHandle(600, preY, GetColor(255, 200, 100), f20, "▼ 移動プレビュー(演算子 [%c] を取得してバトル)", disp_aOp);
                else DrawStringToHandle(600, preY, "▼ 移動プレビュー(バトル発生)", GetColor(255, 200, 100), f20);
            }

            // 左辺と右辺の「誰の数字か」ラベルを決定
            std::string leftLabel, rightLabel;
            unsigned int leftCol = COL_TEXT_SUB(), rightCol = COL_TEXT_SUB();

            if (disp_aOp == '/') {
                leftLabel = "X座標";  leftCol = COL_INFO();
                rightLabel = "Y座標"; rightCol = COL_INFO();
            }
            else {
                // 常に左が「自分(行動者)」、右が「相手(ターゲット)」
                leftLabel = "自分";
                leftCol = Is1PTurn() ? COL_P1() : COL_P2();

                rightLabel = "相手";
                rightCol = Is1PTurn() ? COL_P2() : COL_P1();
            }

            int calcY = BOTTOM_PANEL_Y + 22;

            if (m_ruleMode == RuleMode::ZERO_ONE) {
                DrawStringToHandle(672, calcY - 18, leftLabel.c_str(), leftCol, f16);
                DrawStringToHandle(760, calcY - 18, rightLabel.c_str(), rightCol, f16);

                DrawFormatStringToHandle(672, calcY, COL_TEXT_DARK(), f64, "%d %c %d =", disp_aNum, disp_aOp, disp_tNum);
                DrawFormatStringToHandle(670, calcY - 2, COL_TEXT_MAIN(), f64, "%d %c %d =", disp_aNum, disp_aOp, disp_tNum);

                unsigned int resColor = (resFrac.n < 0) ? COL_DANGER() : (!isCleanDivide ? COL_DISABLE() : COL_WARN());
                int resX = 1010;

                if (resFrac.d == 1 || !isCleanDivide) {
                    DrawFormatStringToHandle(resX + 2, calcY, COL_TEXT_DARK(), f64, "%lld", resFrac.n);
                    DrawFormatStringToHandle(resX, calcY - 2, resColor, f64, "%lld", resFrac.n);
                }

                if (isPreviewMode) {
                    Fraction goal(m_targetScore);
                    Fraction nextMy = (Is1PTurn() ? m_p1ZeroOneScore : m_p2ZeroOneScore) + resFrac;
                    if (nextMy > goal) nextMy = goal - (nextMy - goal);

                    Fraction nextEn = (Is1PTurn() ? m_p2ZeroOneScore : m_p1ZeroOneScore) + resFrac;
                    if (nextEn > goal) nextEn = goal - (nextEn - goal);

                    std::string strMy = (nextMy.d == 1) ? std::to_string(nextMy.n) : (std::to_string(nextMy.n) + "/" + std::to_string(nextMy.d));
                    std::string strEn = (nextEn.d == 1) ? std::to_string(nextEn.n) : (std::to_string(nextEn.n) + "/" + std::to_string(nextEn.d));

                    DrawStringToHandle(650, BOTTOM_PANEL_Y + 85, "【自分に反映】", COL_SAFE(), f22);
                    DrawFormatStringToHandle(790, BOTTOM_PANEL_Y + 85, COL_TEXT_SUB(), f22, "スコア: %s", strMy.c_str());

                    DrawStringToHandle(950, BOTTOM_PANEL_Y + 85, "【相手に反映】", COL_DANGER(), f22);
                    DrawFormatStringToHandle(1090, BOTTOM_PANEL_Y + 85, COL_TEXT_SUB(), f22, "スコア: %s", strEn.c_str());

                    if (disp_aOp == '/') {
                        int wx = disp_aNum - 1; int wy = 9 - disp_tNum;
                        if (isCleanDivide) {
                            DrawFormatStringToHandle(650, BOTTOM_PANEL_Y + 115, COL_SAFE(), f22, "座標(%d, %d)に【自分】のワープ設置！", wx, wy);
                            DrawFormatStringToHandle(950, BOTTOM_PANEL_Y + 115, COL_DANGER(), f22, "座標(%d, %d)に【相手】のワープ設置！", wx, wy);
                        }
                        else {
                            DrawFormatStringToHandle(650, BOTTOM_PANEL_Y + 115, COL_INFO(), f22, "座標(%d, %d)にワープ設置 (※割り切れないためスコア変動なし)", wx, wy);
                        }
                    }
                }
                else {
                    if (isHoverSelf || isHoverEnemy) {
                        std::string targetName = isHoverSelf ? (activeActor == m_player.get() ? "1P" : "2P") : (activeTarget == m_player.get() ? "1P" : "2P");
                        DrawFormatStringToHandle(650, BOTTOM_PANEL_Y + 82, COL_TEXT_MAIN(), f22, "▼ %s のスコアにこの結果を反映", targetName.c_str());
                        if (disp_aOp == '/') {
                            unsigned int warpCol = isHoverSelf ? COL_SAFE() : COL_DANGER();
                            if (isCleanDivide) DrawFormatStringToHandle(650, BOTTOM_PANEL_Y + 112, warpCol, f22, "さらに座標(%d, %d)に【%s】のワープ設置！", disp_aNum, disp_tNum, targetName.c_str());
                            else DrawFormatStringToHandle(650, BOTTOM_PANEL_Y + 112, warpCol, f22, "座標(%d, %d)に【%s】のワープ設置！ (※スコア変動なし)", disp_aNum, disp_tNum, targetName.c_str());
                        }
                    }
                    else {
                        DrawStringToHandle(770, BOTTOM_PANEL_Y + 82, "どちらに反映しますか", GetColor(120, 120, 130), f22);
                        if (disp_aOp == '/') DrawFormatStringToHandle(770, BOTTOM_PANEL_Y + 112, COL_INFO(), f22, "実行時、選択した対象にワープを設置！");
                    }
                }
            }
            else { // ノーマルバトル
                int calcY = BOTTOM_PANEL_Y + 22;

                DrawStringToHandle(672, calcY - 18, leftLabel.c_str(), leftCol, f16);
                DrawStringToHandle(760, calcY - 18, rightLabel.c_str(), rightCol, f16);

                DrawFormatStringToHandle(672, calcY, COL_TEXT_DARK(), f64, "%d %c %d =>", disp_aNum, disp_aOp, disp_tNum);
                DrawFormatStringToHandle(670, calcY - 2, COL_TEXT_MAIN(), f64, "%d %c %d =>", disp_aNum, disp_aOp, disp_tNum);

                unsigned int resColor = (intRes < 0) ? COL_DANGER() : (!isCleanDivide ? COL_DISABLE() : COL_SAFE());
                DrawFormatStringToHandle(1032, calcY, COL_TEXT_DARK(), f64, "%s%d", (intRes > 0 ? "+" : ""), intRes);
                DrawFormatStringToHandle(1030, calcY - 2, resColor, f64, "%s%d", (intRes > 0 ? "+" : ""), intRes);
       

                
                if (isPreviewMode) {
                    if (disp_aOp == '/' && !isCleanDivide) {
                        DrawStringToHandle(650, BOTTOM_PANEL_Y + 85, "【自分に反映】", COL_SAFE(), f22);
                        DrawFormatStringToHandle(650, BOTTOM_PANEL_Y + 115, COL_INFO(), f22, "座標(%d, %d)にワープ設置", disp_aNum, disp_tNum);
                        DrawStringToHandle(950, BOTTOM_PANEL_Y + 85, "【相手に反映】", COL_DANGER(), f22);
                        DrawFormatStringToHandle(950, BOTTOM_PANEL_Y + 115, COL_INFO(), f22, "座標(%d, %d)にワープ設置", disp_aNum, disp_tNum);
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

                        DrawStringToHandle(650, BOTTOM_PANEL_Y + 85, "【自分に反映】", COL_SAFE(), f22);
                        if (mySt <= 0 && myHp <= 0) DrawStringToHandle(790, BOTTOM_PANEL_Y + 85, "敗北", COL_DANGER(), f22);
                        else DrawFormatStringToHandle(790, BOTTOM_PANEL_Y + 85, COL_TEXT_SUB(), f22, "残機 %d / 体力 %d", mySt, myHp);

                        DrawStringToHandle(950, BOTTOM_PANEL_Y + 85, "【相手に反映】", COL_DANGER(), f22);
                        if (enSt <= 0 && enHp <= 0) DrawStringToHandle(1090, BOTTOM_PANEL_Y + 85, "勝利", COL_WARN(), f22);
                        else DrawFormatStringToHandle(1090, BOTTOM_PANEL_Y + 85, COL_TEXT_SUB(), f22, "残機 %d | 体力 %d", enSt, enHp);

                        if (disp_aOp == '/') {
                            DrawFormatStringToHandle(650, BOTTOM_PANEL_Y + 115, COL_SAFE(), f22, "座標(%d, %d)に【自分】のワープ設置！", disp_aNum, disp_tNum);
                            DrawFormatStringToHandle(950, BOTTOM_PANEL_Y + 115, COL_DANGER(), f22, "座標(%d, %d)に【相手】のワープ設置！", disp_aNum, disp_tNum);
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

                        if (stockChange < 0) DrawFormatStringToHandle(620, BOTTOM_PANEL_Y + 82, COL_DANGER(), f22, "▼ 攻撃！ %s の【 バッテリーを%d 】、パワーを [%d] にします", targetName.c_str(), stockChange, simulatedHp);
                        else if (stockChange > 0) DrawFormatStringToHandle(620, BOTTOM_PANEL_Y + 82, COL_SAFE(), f22, "▼ 回復！ %s の【 バッテリーを+%d 】、パワーを [%d] にします", targetName.c_str(), stockChange, simulatedHp);
                        else DrawFormatStringToHandle(620, BOTTOM_PANEL_Y + 82, COL_TEXT_SUB(), f22, "▼ 反映！ %s のパワーを [%d] に", targetName.c_str(), simulatedHp);

                        if (disp_aOp == '/') {
                            unsigned int warpCol = isHoverSelf ? COL_SAFE() : COL_DANGER();
                            if (isCleanDivide) DrawFormatStringToHandle(620, BOTTOM_PANEL_Y + 112, warpCol, f22, "さらに座標(%d, %d)に【%s】のワープ設置！", disp_aNum, disp_tNum, targetName.c_str());
                            else DrawFormatStringToHandle(620, BOTTOM_PANEL_Y + 112, warpCol, f22, "座標(%d, %d)に【%s】のワープ設置！", disp_aNum, disp_tNum, targetName.c_str());
                        }
                    }
                    else {
                        DrawStringToHandle(770, BOTTOM_PANEL_Y + 82, "反映する対象を選択してください", GetColor(120, 120, 130), f22);
                        if (disp_aOp == '/') DrawFormatStringToHandle(770, BOTTOM_PANEL_Y + 112, COL_INFO(), f22, "実行時、選択した対象にワープ設置！");
                    }
                }
            }
        }
        else if (m_currentPhase == Phase::P1_Action || m_currentPhase == Phase::P2_Action) {
            int f30 = GetCachedFont(30);
            int f28 = GetCachedFont(28);
            if (canAttack && !hasOp) { DrawStringToHandle(730, BOTTOM_PANEL_Y + 40, "【 演算子アイテムが必要です 】", COL_DANGER(), f30); }
            else { DrawStringToHandle(800, BOTTOM_PANEL_Y + 40, "ターゲットが射程内にいません", COL_DISABLE(), f28); }
        }
        else {
            DrawStringToHandle(760, BOTTOM_PANEL_Y + 40, "移動するマスを選択してください", COL_DISABLE(), GetCachedFont(28));
        }

        if (m_currentPhase == Phase::P1_Action || m_currentPhase == Phase::P2_Action) {
            if (m_currentPhase == Phase::P1_Action && m_is1P_NPC) return;
            if (m_currentPhase == Phase::P2_Action && m_is2P_NPC) return;

            int by = 960;
            int f26 = GetCachedFont(26);
            if (canAttack && hasOp) {
                auto drawBtn = [&](int x, int y, int w, int h, const char* text, unsigned int col, bool hover) {
                    if (hover) {
                        DrawBox(x, y, x + w, y + h, col, TRUE);
                        DrawFormatStringToHandle(x + (w - GetDrawStringWidthToHandle(text, (int)strlen(text), f26)) / 2, y + 22, COL_TEXT_DARK(), f26, text);
                    }
                    else {
                        DrawBox(x, y, x + w, y + h, GetColor(20, 20, 25), TRUE);
                        DrawBox(x, y, x + w, y + h, col, FALSE);
                        DrawFormatStringToHandle(x + (w - GetDrawStringWidthToHandle(text, (int)strlen(text), f26)) / 2, y + 22, col, f26, text);
                    }
                    };
                drawBtn(600, by, 220, 60, "自分", COL_SAFE(), isHoverSelf);
                drawBtn(850, by, 220, 60, "相手", COL_DANGER(), isHoverEnemy);
                drawBtn(1100, by, 220, 60, "何もしない", COL_DISABLE(), CheckButtonClick(1100, by, 220, 60, mousePos));
            }
            else {
                bool hover = CheckButtonClick(750, by, 420, 60, mousePos);
                unsigned int btnCol = GetColor(180, 180, 200);
                int f28 = GetCachedFont(28);
                if (hover) {
                    DrawBox(750, by, 1170, by + 60, btnCol, TRUE);
                    DrawStringToHandle(875, by + 16, "ターン終了", COL_TEXT_DARK(), f28);
                }
                else {
                    DrawBox(750, by, 1170, by + 60, GetColor(20, 20, 25), TRUE);
                    DrawBox(750, by, 1170, by + 60, btnCol, FALSE);
                    DrawStringToHandle(875, by + 16, "ターン終了", btnCol, f28);
                }
            }
        }
        // ==========================================
        // 右上の PAUSE(ポーズ)ボタン描画
        // ==========================================
        int pauseBtnW = 160;
        int pauseBtnH = 50;
        int pauseBtnX = SCREEN_W - pauseBtnW - 20;
        int pauseBtnY = 10;
        bool isPauseHover = CheckButtonClick(pauseBtnX, pauseBtnY, pauseBtnW, pauseBtnH, mousePos);
        unsigned int pauseCol = isPauseHover ? COL_WARN() : COL_TEXT_OFF();

        // 背景(半透明)
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, isPauseHover ? 200 : 120);
        DrawBox(pauseBtnX, pauseBtnY, pauseBtnX + pauseBtnW, pauseBtnY + pauseBtnH, COL_BG(), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 枠線
        DrawBox(pauseBtnX, pauseBtnY, pauseBtnX + pauseBtnW, pauseBtnY + pauseBtnH, pauseCol, FALSE);

        // テキスト
        int f22Btn = GetCachedFont(22);
        const char* pauseStr = "ポーズ (ESC)";
        int twPause = GetDrawStringWidthToHandle(pauseStr, (int)strlen(pauseStr), f22Btn);
        DrawStringToHandle(pauseBtnX + (pauseBtnW - twPause) / 2, pauseBtnY + 12, pauseStr, pauseCol, f22Btn);
        // ==========================================

        if (m_currentPhase == Phase::FINISH) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
            DrawBox(0, 0, SCREEN_W, SCREEN_H, COL_TEXT_DARK(), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            if (m_currentPhase == Phase::FINISH) {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
                DrawBox(0, 0, SCREEN_W, SCREEN_H, COL_TEXT_DARK(), TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

                int f160 = GetCachedFont(160);
                const char* finishText = "ゲーム終了 !!";
                int textW = GetDrawStringWidthToHandle(finishText, (int)strlen(finishText), f160);
                DrawStringToHandle(SCREEN_W / 2 - textW / 2, SCREEN_H / 2 - 100, finishText, COL_TEXT_MAIN(), f160);

                if (m_finishTimer > 30) {
                    int f40 = GetCachedFont(40);
                    const char* nextText = ">> クリックしてください <<";
                    int nw = GetDrawStringWidthToHandle(nextText, (int)strlen(nextText), f40);
                    DrawStringToHandle(SCREEN_W / 2 - nw / 2, SCREEN_H / 2 + 100, nextText, COL_TEXT_SUB(), f40);
                }
            }
        }
    }

    bool BattleMaster::IsGameOver() const {
        if (!m_player || !m_enemy) return false;
        if (m_ruleMode == RuleMode::ZERO_ONE) {
            Fraction goal(m_targetScore);
            return (m_p1ZeroOneScore == goal || m_p2ZeroOneScore == goal);
        }
        if (m_player->GetStocks() <= 0 && m_player->GetNumber() <= 0) return true;
        if (m_enemy->GetStocks() <= 0 && m_enemy->GetNumber() <= 0) return true;
        return false;
    }

    bool BattleMaster::IsPlayerWin() const {
        if (!m_player || !m_enemy) return false;
        if (m_ruleMode == RuleMode::ZERO_ONE) {
            Fraction goal(m_targetScore);
            return (m_p1ZeroOneScore == goal);
        }
        else {
            return (m_enemy->GetStocks() <= 0 && m_enemy->GetNumber() <= 0);
        }
    }

    bool BattleMaster::CheckButtonClick(int x, int y, int w, int h, const Vector2& mousePos) const {
        return (mousePos.x >= static_cast<float>(x) && mousePos.x <= static_cast<float>(x + w) &&
            mousePos.y >= static_cast<float>(y) && mousePos.y <= static_cast<float>(y + h));
    }
} // namespace App