#define NOMINMAX
#include "BattleUI.h"
#include "../Manager/BattleMaster.h"
#include "../Input/InputManager.h"
#include "../../CyberGrid.h"
#include "../Utility/AppConfig.h" 
#include <string>
#include"../Object/Map/MapGrid.h"
#include <unordered_map>
#include <cmath>
#include <algorithm>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace App {
    using namespace Config; // ★Config内のSCREEN_WやCOL_P1()を直接使えるようにする

    BattleUI::BattleUI()
        : m_psHandle(-1), m_cbHandle(-1), m_shaderTime(0.0f)
        , m_logScrollOffset(0), m_uiCursorX_1P(0.0f), m_uiCursorX_2P(0.0f) {
    }

    BattleUI::~BattleUI() {
        if (m_psHandle != -1) DeleteShader(m_psHandle);
        if (m_cbHandle != -1) DeleteShaderConstantBuffer(m_cbHandle);
    }

    void BattleUI::Init() {
        m_psHandle = LoadPixelShaderFromMem(g_ps_CyberGrid, sizeof(g_ps_CyberGrid));
        m_cbHandle = CreateShaderConstantBuffer(sizeof(float) * 4);
        m_shaderTime = 0.0f;
        m_logScrollOffset = 0;
        m_uiCursorX_1P = 0.0f;
        m_uiCursorX_2P = 0.0f;
        m_actionLog.clear();
    }

    void BattleUI::AddLog(const std::string& message) {
        m_actionLog.push_back(message);
        if (m_actionLog.size() > 100) m_actionLog.erase(m_actionLog.begin());
        m_logScrollOffset = std::max(0, (int)m_actionLog.size() - 6);
    }

    void BattleUI::ScrollLog(int wheelDelta, float mouseX, float mouseY) {
        if (mouseX >= 40 && mouseX <= 540 && mouseY >= LOG_PANEL_Y && mouseY <= LOG_PANEL_Y + 200) {
            m_logScrollOffset -= wheelDelta;
            int maxOffset = std::max(0, (int)m_actionLog.size() - 6);
            if (m_logScrollOffset < 0) m_logScrollOffset = 0;
            if (m_logScrollOffset > maxOffset) m_logScrollOffset = maxOffset;
        }
    }

    void BattleUI::Update(float effectIntensity, int p1Num, int p2Num) {
        auto updateCursor = [](float& currentX, int targetNum) {
            float targetX = 40.0f + (targetNum - 1) * 48.0f;
            if (currentX == 0.0f) currentX = targetX;
            currentX += (targetX - currentX) * 0.2f;
            };
        updateCursor(m_uiCursorX_1P, p1Num);
        updateCursor(m_uiCursorX_2P, p2Num);
        m_shaderTime += 0.0016f + (0.01f * effectIntensity);
    }



    void BattleUI::Draw(const BattleMaster& master) const {
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

        master.m_mapGrid.Draw();

        auto drawWarpNodes = [&](UnitBase* unit, unsigned int baseCol, const char* label) {
            if (!unit) return;
            for (auto pos : unit->GetWarpNodes()) {
                Vector2 center = master.m_mapGrid.GetCellCenter(pos.x, pos.y);
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

        if (master.Is1PTurn()) drawWarpNodes(master.m_player.get(), COL_P1(), "1P ST.");
        else drawWarpNodes(master.m_enemy.get(), COL_P2(), "2P ST.");

       DrawEnemyDangerArea(master);
       DrawMovableArea(master);

        UnitBase* blinkUnit = master.GetActiveUnit();
        if (blinkUnit && !blinkUnit->IsMoving()) {
            IntVector2 bPos = blinkUnit->GetGridPos();
            Vector2 bCenter = master.GetMapGrid().GetCellCenter(bPos.x, bPos.y);

            double time = GetNowCount() / 1000.0;
            float bobbing = (blinkUnit == master.m_player.get()) ? (float)(sin(time * 2.5) * 5.0) : (float)(sin(time * 2.0) * 4.0);
            bCenter.y += bobbing;

            int alpha = (int)(150 + 105 * sin(time * M_PI));
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
            unsigned int auraCol = (blinkUnit == master.m_player.get()) ? COL_WARN() : COL_P2();

            for (int i = 0; i < 3; ++i) {
                DrawBox((int)bCenter.x - 39 + i, (int)bCenter.y - 39 + i, (int)bCenter.x + 39 - i, (int)bCenter.y + 39 - i, auraCol, FALSE);
            }
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha / 4);
            DrawBox((int)bCenter.x - 36, (int)bCenter.y - 36, (int)bCenter.x + 36, (int)bCenter.y + 36, auraCol, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        if (master.m_player) master.m_player->Draw();
        if (master.m_enemy)  master.m_enemy->Draw();

        unsigned int phaseCol;
        const char* phaseName;
        if (master.m_currentPhase == BattleMaster::Phase::P1_Move) { phaseCol = GetColor(180, 110, 0); phaseName = master.m_is1P_NPC ? "1Pのターン (思考中)" : "1Pのターン (移動選択)"; }
        else if (master.m_currentPhase == BattleMaster::Phase::P1_Action) { phaseCol = GetColor(180, 110, 0); phaseName = master.m_is1P_NPC ? "1Pのターン (思考中)" : "1Pのターン (行動選択)"; }
        else if (master.m_currentPhase == BattleMaster::Phase::P2_Move) { phaseCol = GetColor(30, 50, 120); phaseName = master.m_is2P_NPC ? "2Pのターン (思考中)" : "2Pのターン (移動選択)"; }
        else if (master.m_currentPhase == BattleMaster::Phase::P2_Action) { phaseCol = GetColor(30, 50, 120); phaseName = master.m_is2P_NPC ? "2Pのターン (思考中)" : "2Pのターン (行動選択)"; }
        else { phaseCol = COL_INFO(); phaseName = "終了！！"; }

        DrawBox(0, 0, SCREEN_W, HEADER_H, phaseCol, TRUE);
        DrawLine(0, HEADER_H, SCREEN_W, HEADER_H, COL_TEXT_MAIN(), 2);

        DrawFormatStringToHandle(40, 16, COL_TEXT_MAIN(), GetCachedFont(38), ">>> %s", phaseName);
        DrawFormatStringToHandle(800, 24, COL_TEXT_SUB(), GetCachedFont(24), "経過ターン: %d", master.m_mapGrid.GetTotalTurns());

        // ==========================================
        // 事前計算エリア (UI描画前にすべてのプレビュー値を計算)
        // ==========================================
        Vector2 mousePos = InputManager::GetInstance().GetMousePos();
        UnitBase* activeActor = master.GetActiveUnit();
        UnitBase* activeTarget = master.GetTargetUnit();

        bool isMovePreview = false;
        int previewCost = 0;
        int previewNum = activeActor ? activeActor->GetNumber() : 0;
        bool isPreviewNextToEnemy = false;
        char previewOp = activeActor ? activeActor->GetOp() : '\0';

        if (master.m_isPlayerSelected && activeActor && !activeActor->IsMoving() &&
            (master.m_currentPhase == BattleMaster::Phase::P1_Move || master.m_currentPhase == BattleMaster::Phase::P2_Move)) {

            if (master.m_mapGrid.IsWithinBounds(master.m_hoverGrid.x, master.m_hoverGrid.y)) {
                if (master.m_hoverGrid == activeActor->GetGridPos()) {
                    isMovePreview = true;
                    previewCost = 1;
                }
                else if (master.CanMove(activeActor->GetNumber(), activeActor->GetOp(), activeActor->GetGridPos(), master.m_hoverGrid, previewCost)) {
                    isMovePreview = true;
                }

                if (isMovePreview) {
                    previewNum = activeActor->GetNumber() - previewCost;
                    while (previewNum <= 0) previewNum += 9;

                    char itemOnGrid = master.m_mapGrid.GetItemAt(master.m_hoverGrid.x, master.m_hoverGrid.y);
                    if (itemOnGrid != '\0') previewOp = itemOnGrid;

                    if (activeTarget) {
                        IntVector2 tP = activeTarget->GetGridPos();
                        if (std::abs(master.m_hoverGrid.x - tP.x) + std::abs(master.m_hoverGrid.y - tP.y) == 1) {
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
            bool is1PTurn = master.Is1PTurn();
            if (master.CheckButtonClick(600, 960, 220, 60, mousePos)) isHoverSelf = true;
            else if (master.CheckButtonClick(850, 960, 220, 60, mousePos)) isHoverEnemy = true;
            else if (master.CheckButtonClick(40, 100, 500, 650, mousePos)) {
                if (is1PTurn) isHoverSelf = true; else isHoverEnemy = true;
            }
            else if (master.CheckButtonClick(1380, 100, 500, 650, mousePos)) {
                if (is1PTurn) isHoverEnemy = true; else isHoverSelf = true;
            }
            else if (master.m_hoverGrid == aP) isHoverSelf = true;
            else if (master.m_hoverGrid == tP) isHoverEnemy = true;
        }

        bool showCalcPanel = false;
        int disp_aNum = 0; int disp_tNum = 0; char disp_aOp = '\0';
        bool isPreviewMode = false; bool willGetNewOp = false;

        if (master.m_currentPhase == BattleMaster::Phase::P1_Action || master.m_currentPhase == BattleMaster::Phase::P2_Action) {
            if (canAttack && hasOp) {
                showCalcPanel = true;
                disp_aNum = activeActor->GetNumber();
                disp_tNum = activeTarget->GetNumber();
                disp_aOp = activeActor->GetOp();
            }
        }
        else if (master.m_currentPhase == BattleMaster::Phase::P1_Move || master.m_currentPhase == BattleMaster::Phase::P2_Move) {
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
        int p1PreviewNum = -1; int p1PreviewStocks = master.m_player ? master.m_player->GetStocks() : 0;
        int p2PreviewNum = -1; int p2PreviewStocks = master.m_enemy ? master.m_enemy->GetStocks() : 0;

        Fraction p1PreviewScore = master.m_p1ZeroOneScore; bool p1HasScorePreview = false;
        Fraction p2PreviewScore = master.m_p2ZeroOneScore; bool p2HasScorePreview = false;
        // ① 移動によるプレビュー
        if (isMovePreview) {
            if (activeActor == master.m_player.get()) {
                int temp = master.m_player->GetNumber() - previewCost;
                p1PreviewStocks = master.m_player->GetStocks();
                while (temp <= 0) { p1PreviewStocks--; temp += 9; }
                p1PreviewNum = (p1PreviewStocks < 0) ? 0 : temp;
            }
            else if (activeActor == master.m_enemy.get()) {
                int temp = master.m_enemy->GetNumber() - previewCost;
                p2PreviewStocks = master.m_enemy->GetStocks();
                while (temp <= 0) { p2PreviewStocks--; temp += 9; }
                p2PreviewNum = (p2PreviewStocks < 0) ? 0 : temp;
            }
        }

        // ② 計算実行(ホバー時)によるプレビュー
        if (showCalcPanel && (isHoverSelf || isHoverEnemy)) {
            bool applyTo1P = (isHoverSelf && activeActor == master.m_player.get()) || (!isHoverSelf && activeTarget == master.m_player.get());

            if (master.m_ruleMode == BattleMaster::RuleMode::CLASSIC) {// ノーマルバトルのプレビュー
                if (disp_aOp != '/') {
                    int targetCurrentHp = isHoverSelf ? disp_aNum : disp_tNum;
                    int currentStocks = applyTo1P ? master.m_player->GetStocks() : master.m_enemy->GetStocks();

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
                    Fraction goal(master.m_targetScore);
                    Fraction currentScore = applyTo1P ? master.m_p1ZeroOneScore : master.m_p2ZeroOneScore;
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
            Vector2 center = master.m_mapGrid.GetCellCenter(wx, wy);

            double time = GetNowCount() / 1000.0;
            int alpha = (int)(100 + 100 * sin(time * M_PI * 5.0));

            unsigned int pCol = COL_INFO();
            if (isHoverSelf) pCol = master.Is1PTurn() ? COL_P1() : COL_P2();
            else if (isHoverEnemy) pCol = master.Is1PTurn() ? COL_P2() : COL_P1();

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
            std::string headerName = is1P ? (master.m_is1P_NPC ? "1P (COM)" : "1P PLAYER") : (master.m_is2P_NPC ? "2P (COM)" : "2P PLAYER");

            if ((is1P && master.Is1PTurn()) || (!is1P && !master.Is1PTurn() && master.m_currentPhase != BattleMaster::Phase::FINISH)) {
                SetDrawBlendMode(DX_BLENDMODE_ADD, (int)(100 + 50 * sin(GetNowCount() / 200.0)));
                DrawBox(x - 5, y - 5, x + 505, y + 490, baseCol, FALSE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }

            DrawStringToHandle(x + 5, y + 5, headerName.c_str(), baseCol, GetCachedFont(36));
            DrawLine(x, y + 45, x + 500, y + 45, baseCol, 2);

            int scoreY = y + 70;

            if (master.m_ruleMode == BattleMaster::RuleMode::ZERO_ONE) {
                DrawBox(x, scoreY, x + 500, scoreY + 140, COL_DARK_BG(), TRUE);
                Fraction f = is1P ? master.m_p1ZeroOneScore : master.m_p2ZeroOneScore;

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
                    unsigned int previewCol = (p_previewScore.n == master.m_targetScore) ? COL_WARN() :
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
                    if (p_previewScore.n == master.m_targetScore) {
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
                DrawFormatStringToHandle(x + 390, targetBoxY + 2, COL_SAFE(), f32, "%03d", master.m_targetScore);
            }
            else {
                DrawBox(x, scoreY, x + 500, scoreY + 35, COL_DARK_BG(), TRUE);
                DrawStringToHandle(x + 15, scoreY + 8, "ノーマルバトル", COL_TEXT_SUB(), GetCachedFont(20));
            }

            int powerY = scoreY + (master.m_ruleMode == BattleMaster::RuleMode::ZERO_ONE ? 190 : 50);
            DrawBox(x, powerY, x + 500, powerY + 120, COL_DARK_BG(), TRUE);

            DrawStringToHandle(x + 15, powerY + 10, "パワー", COL_WARN(), GetCachedFont(24));
            DrawLine(x + 100, powerY + 24, x + 490, powerY + 24, GetColor(60, 60, 70), 1);

            int currentNum = unit->GetNumber();
            int preview = (p_previewNum != -1) ? p_previewNum : currentNum;
            float cursorX = is1P ? m_uiCursorX_1P : m_uiCursorX_2P;
            if (cursorX == 0.0f) cursorX = 40.0f + (currentNum - 1) * 48.0f;

            DrawBox(x + 10, powerY + 40, x + 490, powerY + 90, GetColor(10, 10, 15), TRUE);
            DrawLine(x + 10, powerY + 65, x + 490, powerY + 65, GetColor(30, 30, 40), 1);

            bool isDeadPreview = (p_previewStocks < 0) && (master.m_ruleMode == BattleMaster::RuleMode::CLASSIC);
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
                if (master.m_ruleMode == BattleMaster::RuleMode::CLASSIC) {
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

            if (master.m_ruleMode == BattleMaster::RuleMode::CLASSIC) {
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
        drawUnitCard(40, 100, master.m_player.get(), true, p1PreviewNum, p1PreviewStocks, p1PreviewScore, p1HasScorePreview);
        drawUnitCard(1380, 100, master.m_enemy.get(), false, p2PreviewNum, p2PreviewStocks, p2PreviewScore, p2HasScorePreview);


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

        unsigned int calcBorderCol = master.Is1PTurn() ? COL_P1() : COL_P2();
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
                leftCol = master.Is1PTurn() ? COL_P1() : COL_P2();

                rightLabel = "相手";
                rightCol = master.Is1PTurn() ? COL_P2() : COL_P1();
            }

            int calcY = BOTTOM_PANEL_Y + 22;

            if (master.m_ruleMode == BattleMaster::RuleMode::ZERO_ONE) {
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
                    Fraction goal(master.m_targetScore);
                    Fraction nextMy = (master.Is1PTurn() ? master.m_p1ZeroOneScore : master.m_p2ZeroOneScore) + resFrac;
                    if (nextMy > goal) nextMy = goal - (nextMy - goal);

                    Fraction nextEn = (master.Is1PTurn() ? master.m_p2ZeroOneScore : master.m_p1ZeroOneScore) + resFrac;
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
                        std::string targetName = isHoverSelf ? (activeActor == master.m_player.get() ? "1P" : "2P") : (activeTarget == master.m_player.get() ? "1P" : "2P");
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
                        std::string targetName = isHoverSelf ? (activeActor == master.m_player.get() ? "1P" : "2P") : (activeTarget == master.m_player.get() ? "1P" : "2P");
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
        else if (master.m_currentPhase == BattleMaster::Phase::P1_Action || master.m_currentPhase == BattleMaster::Phase::P2_Action) {
            int f30 = GetCachedFont(30);
            int f28 = GetCachedFont(28);
            if (canAttack && !hasOp) { DrawStringToHandle(730, BOTTOM_PANEL_Y + 40, "【 演算子アイテムが必要です 】", COL_DANGER(), f30); }
            else { DrawStringToHandle(800, BOTTOM_PANEL_Y + 40, "ターゲットが射程内にいません", COL_DISABLE(), f28); }
        }
        else {
            DrawStringToHandle(760, BOTTOM_PANEL_Y + 40, "移動するマスを選択してください", COL_DISABLE(), GetCachedFont(28));
        }

        if (master.m_currentPhase == BattleMaster::Phase::P1_Action || master.m_currentPhase == BattleMaster::Phase::P2_Action) {
            if (master.m_currentPhase == BattleMaster::Phase::P1_Action && master.m_is1P_NPC) return;
            if (master.m_currentPhase == BattleMaster::Phase::P2_Action && master.m_is2P_NPC) return;

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
                drawBtn(1100, by, 220, 60, "何もしない", COL_DISABLE(), master.CheckButtonClick(1100, by, 220, 60, mousePos));
            }
            else {
                bool hover = master.CheckButtonClick(750, by, 420, 60, mousePos);
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
        bool isPauseHover = master.CheckButtonClick(pauseBtnX, pauseBtnY, pauseBtnW, pauseBtnH, mousePos);
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

        if (master.m_currentPhase == BattleMaster::Phase::FINISH) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
            DrawBox(0, 0, SCREEN_W, SCREEN_H, COL_TEXT_DARK(), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            if (master.m_currentPhase == BattleMaster::Phase::FINISH) {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
                DrawBox(0, 0, SCREEN_W, SCREEN_H, COL_TEXT_DARK(), TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

                int f160 = GetCachedFont(160);
                const char* finishText = "ゲーム終了 !!";
                int textW = GetDrawStringWidthToHandle(finishText, (int)strlen(finishText), f160);
                DrawStringToHandle(SCREEN_W / 2 - textW / 2, SCREEN_H / 2 - 100, finishText, COL_TEXT_MAIN(), f160);

                if (master.m_finishTimer > 30) {
                    int f40 = GetCachedFont(40);
                    const char* nextText = ">> クリックしてください <<";
                    int nw = GetDrawStringWidthToHandle(nextText, (int)strlen(nextText), f40);
                    DrawStringToHandle(SCREEN_W / 2 - nw / 2, SCREEN_H / 2 + 100, nextText, COL_TEXT_SUB(), f40);
                }
            }
        }
    }
   
    void BattleUI::DrawEnemyDangerArea(const BattleMaster& master) const{
        UnitBase* opp = master.GetTargetUnit();
        if (!opp || opp->GetStocks() <= 0 || opp->IsMoving()) return;

        IntVector2 ePos = opp->GetGridPos();
        int eNum = opp->GetNumber();
        char eOp = opp->GetOp();

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 80);
        for (int x = 0; x < master.m_mapGrid.GetWidth(); ++x) {
            for (int y = 0; y < master.m_mapGrid.GetHeight(); ++y) {
                IntVector2 target{ x, y };
                Vector2 center = master.m_mapGrid.GetCellCenter(x, y);

                if (target == ePos) {
                    DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, COL_DANGER(), TRUE);
                    continue;
                }

                int dummyCost = 0;
                bool canMoveBase = master.CanMove(eNum, '\0', ePos, target, dummyCost);
                bool canMoveCombined = master.CanMove(eNum, eOp, ePos, target, dummyCost);

                if (canMoveCombined) {
                    unsigned int areaCol = canMoveBase ? COL_DANGER_DIM() : COL_INFO();
                    DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, areaCol, TRUE);
                }
            }
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    void BattleUI::DrawMovableArea(const BattleMaster& master) const{
        if (!master.m_isPlayerSelected) return;
        UnitBase* activeUnit = master.GetActiveUnit();
        if (!activeUnit || activeUnit->IsMoving()) return;
        IntVector2 pPos = activeUnit->GetGridPos();
        int pNum = activeUnit->GetNumber();
        char pOp = activeUnit->GetOp();

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
        for (int x = 0; x < master.m_mapGrid.GetWidth(); ++x) {
            for (int y = 0; y < master.m_mapGrid.GetHeight(); ++y) {
                IntVector2 target{ x, y };
                Vector2 center = master.m_mapGrid.GetCellCenter(x, y);
                if (target == pPos) {
                    DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, COL_WARN(), TRUE);
                    continue;
                }
                int dummyCost = 0;
                bool canMoveBase = master.CanMove(pNum, '\0', pPos, target, dummyCost);
                bool canMoveCombined = master.CanMove(pNum, pOp, pPos, target, dummyCost);
                bool isWarpNode = activeUnit->HasWarpNode(target);

                if (canMoveCombined || isWarpNode) {
                    unsigned int col = isWarpNode ? COL_INFO() : (canMoveBase ? GetColor(100, 200, 255) : GetColor(255, 180, 50));
                    DrawBox((int)center.x - 35, (int)center.y - 35, (int)center.x + 35, (int)center.y + 35, col, TRUE);
                }
            }
        }
        if (master.m_mapGrid.IsWithinBounds(master.m_hoverGrid.x, master.m_hoverGrid.y)) {
            int hoverCost = 0;
            bool isHoverSelf = (master.m_hoverGrid == pPos);
            if (isHoverSelf || master.CanMove(pNum, pOp, pPos, master.m_hoverGrid, hoverCost)) {
                Vector2 hc = master.m_mapGrid.GetCellCenter(master.m_hoverGrid.x, master.m_hoverGrid.y);
                DrawBox((int)hc.x - 35, (int)hc.y - 35, (int)hc.x + 35, (int)hc.y + 35, COL_TEXT_MAIN(), FALSE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                int displayCost = isHoverSelf ? 1 : hoverCost;

                DrawFormatStringToHandle((int)hc.x - 10, (int)hc.y - 20, COL_DANGER(), GetCachedFont(20), "-%d", displayCost);

                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 100);
            }
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    int BattleUI::GetCachedFont(int size) {
        static std::unordered_map<int, int> s_fontCache;
        if (s_fontCache.find(size) == s_fontCache.end()) {
            s_fontCache[size] = CreateFontToHandle("BIZ UDゴシック", size, 2, DX_FONTTYPE_ANTIALIASING);
        }
        return s_fontCache[size];
    }
}