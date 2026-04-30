#define NOMINMAX
#include "BattleMaster.h"
#include <DxLib.h>
#include <cmath>
#include <random>
#include <algorithm>
#include <unordered_map>
#include "../Input/InputManager.h"
#include "../Scene/SceneManager.h" 
#include "../Battle/BattleUI.h"
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

        m_ui = std::make_unique<BattleUI>();
        m_ui->Init();
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

    void BattleMaster::Draw() const {
        if (m_ui) {
            m_ui->Draw(*this);
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