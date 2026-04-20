#include "TitleScene.h"
#include <DxLib.h>
#include "SceneManager.h"
#include "../Input/InputManager.h" 
#include <string>
#include <cmath>
#include "../../CyberGrid.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ==========================================
// ★ UI・レイアウトの定数化（無名名前空間）
// ==========================================
namespace {

    constexpr double GOLDEN = 1.618;
    constexpr double SILVER = 1.414;
    // --- システム ---
    constexpr int WAIT_START_FRAMES = 10;
    constexpr int TARGET_SCORES[3] = { 53, 103, 223 };

    // --- レイアウト：ベース座標 ---
    constexpr int MENU_CENTER_OFFSET_Y = -130;
    constexpr int MENU_CUSTOM_OFFSET_X = -300;

    // --- レイアウト：メニュー項目 ---
    constexpr int MENU_ITEM_BASE_Y = 100;
    constexpr int MENU_ITEM_STEP_Y = 85;
    constexpr int MENU_BOX_HALF_W = 380;
    constexpr int MENU_BOX_H = 55;
    constexpr int MENU_BOX_OFFSET_Y = 10;

    // --- レイアウト：カスタム設定ボタン ---
    constexpr int CUSTOM_BTN_BASE_X = 140;
    constexpr int CUSTOM_BTN_OFFSET_X = 60;
    constexpr int CUSTOM_BTN_SIZE = 50;

    // --- レイアウト：ミニマップ ---
    constexpr int MAP_CELL_SIZE = 45;
    constexpr int MAP_BASE_OFFSET_X = 180;
    constexpr int MAP_BASE_OFFSET_Y = 40;

    // --- アニメーション ---
    constexpr double BLINK_SPEED = 3.0;
    constexpr int BLINK_BASE_ALPHA = 150;
    constexpr int BLINK_AMP_ALPHA = 105;

    // --- 色の定義 ---
    inline unsigned int COL_BG() { return GetColor(5, 10, 25); }
    inline unsigned int COL_GRID() { return GetColor(0, 150, 255); }
    inline unsigned int COL_P1() { return GetColor(255, 140, 0); }
    inline unsigned int COL_P2() { return GetColor(0, 150, 255); }
    inline unsigned int COL_TEXT_ON() { return GetColor(255, 180, 0); }
    inline unsigned int COL_TEXT_OFF() { return GetColor(50, 100, 150); }
    inline unsigned int COL_WHITE() { return GetColor(255, 255, 255); }
    inline unsigned int COL_BLACK() { return GetColor(0, 0, 0); }
    inline unsigned int COL_TITLE_MAIN() { return GetColor(220, 245, 255); }
    inline unsigned int COL_TITLE_SUB() { return GetColor(0, 120, 255); }
}

namespace App {

    TitleScene::TitleScene()
        : m_frameCount(0), m_state(MenuState::PRESS_START)
        , m_playerCursor(1), m_modeCursor(0), m_scoreCursor(1)
        , m_shouldQuit(false)
        , m_fontTitle(-1), m_fontMenu(-1), m_fontSmall(-1), m_fontNumber(-1)
        , m_psHandle(-1), m_cbHandle(-1), m_shaderTime(0.0f)
        , m_players()
        , m_stocksCursor(-1)
    {
    }

    TitleScene::~TitleScene() {}

    void TitleScene::Init() {
        m_frameCount = 0;
        m_state = MenuState::PRESS_START;
        m_playerCursor = 1;
        m_modeCursor = 0;
        m_scoreCursor = 1;
        m_stocksCursor = 0;

        // ★ 配列を使って1Pと2Pの設定をスッキリ初期化
        m_players[0] = { 0, 0, DEF_P1_HP, DEF_P1_X, DEF_P1_Y };
        m_players[1] = { 1, 0, DEF_P2_HP, DEF_P2_X, DEF_P2_Y };

        m_fontTitle = CreateFontToHandle("BIZ UD明朝 Medium", 100, 3, DX_FONTTYPE_ANTIALIASING);
        m_fontMenu = CreateFontToHandle("BIZ UDゴシック", 40, 2, DX_FONTTYPE_NORMAL);
        m_fontSmall = CreateFontToHandle("遊ゴシック", 24, 2, DX_FONTTYPE_NORMAL);
        m_fontNumber = CreateFontToHandle("HGP創英角ﾎﾟｯﾌﾟ体", 48, 2, DX_FONTTYPE_ANTIALIASING);

        m_psHandle = LoadPixelShaderFromMem(g_ps_CyberGrid, sizeof(g_ps_CyberGrid));

        m_cbHandle = CreateShaderConstantBuffer(sizeof(float) * 4);
        m_shaderTime = 0.0f;
    }

    void TitleScene::Load() {}
    void TitleScene::LoadEnd() {}

    void TitleScene::Update() {
        auto& input = InputManager::GetInstance();

        if (input.IsTrgDown(KEY_INPUT_ESCAPE)) {
            m_shouldQuit = true;
            return;
        }

        ++m_frameCount;
        if (m_frameCount < WAIT_START_FRAMES) return;

        m_shaderTime += 0.0016f;

        // ==========================================
        // 入力取得
        // ==========================================
        bool spaceTrg = input.IsTrgDown(KEY_INPUT_SPACE) || input.IsTrgDown(KEY_INPUT_RETURN);
        bool upTrg = input.IsTrgDown(KEY_INPUT_UP) || input.IsTrgDown(KEY_INPUT_W);
        bool downTrg = input.IsTrgDown(KEY_INPUT_DOWN) || input.IsTrgDown(KEY_INPUT_S);
        bool rightTrg = input.IsTrgDown(KEY_INPUT_RIGHT) || input.IsTrgDown(KEY_INPUT_D);
        bool leftTrg = input.IsTrgDown(KEY_INPUT_LEFT) || input.IsTrgDown(KEY_INPUT_A);
        bool bTrg = input.IsTrgDown(KEY_INPUT_B) || input.IsTrgDown(KEY_INPUT_BACK);

        int hitNum = -1;
        for (int i = 0; i < GRID_SIZE; ++i) {
            if (input.IsTrgDown(KEY_INPUT_1 + i) || input.IsTrgDown(KEY_INPUT_NUMPAD1 + i)) {
                hitNum = i + 1;
            }
        }

        Vector2 m = input.GetMousePos();
        bool mClick = input.IsMouseLeftTrg();

        static Vector2 prevM = m;
        bool mouseMoved = (m.x != prevM.x || m.y != prevM.y);
        prevM = m;

        auto HoverBox = [&](int x, int y, int w, int h) {
            return (m.x >= x && m.x <= x + w && m.y >= y && m.y <= y + h);
            };

        int sw, sh;
        GetDrawScreenSize(&sw, &sh);
        int CX = sw / 2;
        int CY = sh / 2;
        int menuStartY = CY + MENU_CENTER_OFFSET_Y;
        bool isCustomState = (m_state == MenuState::CUSTOM_P1_START || m_state == MenuState::CUSTOM_P2_START);
        int menuCX = isCustomState ? CX + MENU_CUSTOM_OFFSET_X : CX;

        // ★ 戻るボタンの座標定義（画面下部中央）
        int backBtnX = CX - 150;
        int backBtnY = sh - 150;
        int backBtnW = 300;
        int backBtnH = 60;

        // ==========================================
        // ステートごとの更新ロジック
        // ==========================================

        if (m_state == MenuState::PRESS_START) {
            // ★修正：CheckHitKeyAllだと「Bキーを押して戻った瞬間」に暴発して進んでしまうため、トリガー限定に変更！
            if (spaceTrg || mClick) {
                m_state = MenuState::SELECT_PLAYERS;
                m_frameCount = 0;
            }
        }
        else if (m_state == MenuState::SELECT_PLAYERS) {
            for (int i = 0; i < 2; ++i) {
                if (HoverBox(menuCX - MENU_BOX_HALF_W, menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y - MENU_BOX_OFFSET_Y, MENU_BOX_HALF_W * 2, MENU_BOX_H)) {
                    if (mouseMoved) m_playerCursor = i + 1;
                    if (mClick) { m_playerCursor = i + 1; spaceTrg = true; }
                }
            }
            if (upTrg || downTrg) m_playerCursor = (m_playerCursor == 1) ? 2 : 1;

            if (bTrg || (mClick && HoverBox(backBtnX, backBtnY, backBtnW, backBtnH))) {
                m_state = MenuState::PRESS_START;
                m_frameCount = 0;
            }
            else if (spaceTrg) {
                m_players[1].typeCursor = (m_playerCursor == 1) ? 1 : 0;
                m_state = MenuState::SELECT_MODE;
                m_frameCount = 0;
            }
        }
        else if (m_state == MenuState::SELECT_MODE) {
            for (int i = 0; i < 2; ++i) {
                if (HoverBox(menuCX - MENU_BOX_HALF_W, menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y - MENU_BOX_OFFSET_Y, MENU_BOX_HALF_W * 2, MENU_BOX_H)) {
                    if (mouseMoved) m_modeCursor = i;
                    if (mClick) { m_modeCursor = i; spaceTrg = true; }
                }
            }
            if (upTrg || downTrg) m_modeCursor = (m_modeCursor == 0) ? 1 : 0;

            if (bTrg || (mClick && HoverBox(backBtnX, backBtnY, backBtnW, backBtnH))) {
                m_state = MenuState::SELECT_PLAYERS;
                m_frameCount = 0;
            }
            else if (spaceTrg) {
                if (m_modeCursor == 0) m_state = MenuState::SELECT_CLASSIC_STOCKS;
                else m_state = MenuState::SELECT_SCORE;
                m_frameCount = 0;
            }
        }
        else if (m_state == MenuState::SELECT_CLASSIC_STOCKS) {
            for (int i = 0; i < 3; ++i) {
                if (HoverBox(menuCX - MENU_BOX_HALF_W, menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y - MENU_BOX_OFFSET_Y, MENU_BOX_HALF_W * 2, MENU_BOX_H)) {
                    if (mouseMoved) m_stocksCursor = i;
                    if (mClick) { m_stocksCursor = i; spaceTrg = true; }
                }
            }
            if (upTrg) { m_stocksCursor--; if (m_stocksCursor < 0) m_stocksCursor = 2; }
            if (downTrg) { m_stocksCursor++; if (m_stocksCursor > 2) m_stocksCursor = 0; }

            if (bTrg || (mClick && HoverBox(backBtnX, backBtnY, backBtnW, backBtnH))) {
                m_state = MenuState::SELECT_MODE;
                m_frameCount = 0;
            }
            else if (spaceTrg) {
                m_state = MenuState::SELECT_P1_TYPE;
                m_frameCount = 0;
            }
        }
        else if (m_state == MenuState::SELECT_SCORE) {
            for (int i = 0; i < 3; ++i) {
                if (HoverBox(menuCX - MENU_BOX_HALF_W, menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y - MENU_BOX_OFFSET_Y, MENU_BOX_HALF_W * 2, MENU_BOX_H)) {
                    if (mouseMoved) m_scoreCursor = i;
                    if (mClick) { m_scoreCursor = i; spaceTrg = true; }
                }
            }
            if (upTrg) { m_scoreCursor--; if (m_scoreCursor < 0) m_scoreCursor = 2; }
            if (downTrg) { m_scoreCursor++; if (m_scoreCursor > 2) m_scoreCursor = 0; }

            if (bTrg || (mClick && HoverBox(backBtnX, backBtnY, backBtnW, backBtnH))) {
                m_state = MenuState::SELECT_MODE;
                m_frameCount = 0;
            }
            else if (spaceTrg) {
                m_state = MenuState::SELECT_P1_TYPE;
                m_frameCount = 0;
            }
        }
        else if (m_state == MenuState::SELECT_P1_TYPE) {
            for (int i = 0; i < 2; ++i) {
                if (HoverBox(menuCX - MENU_BOX_HALF_W, menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y - MENU_BOX_OFFSET_Y, MENU_BOX_HALF_W * 2, MENU_BOX_H)) {
                    if (mouseMoved) m_players[0].typeCursor = i;
                    if (mClick) { m_players[0].typeCursor = i; spaceTrg = true; }
                }
            }
            if (upTrg || downTrg) m_players[0].typeCursor = (m_players[0].typeCursor == 0) ? 1 : 0;

            if (bTrg || (mClick && HoverBox(backBtnX, backBtnY, backBtnW, backBtnH))) {
                if (m_modeCursor == 0) m_state = MenuState::SELECT_CLASSIC_STOCKS;
                else m_state = MenuState::SELECT_SCORE;
                m_frameCount = 0;
            }
            else if (spaceTrg) {
                m_state = MenuState::SELECT_P2_TYPE;
                m_frameCount = 0;
            }
        }
        else if (m_state == MenuState::SELECT_P2_TYPE) {
            for (int i = 0; i < 2; ++i) {
                if (HoverBox(menuCX - MENU_BOX_HALF_W, menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y - MENU_BOX_OFFSET_Y, MENU_BOX_HALF_W * 2, MENU_BOX_H)) {
                    if (mouseMoved) m_players[1].typeCursor = i;
                    if (mClick) { m_players[1].typeCursor = i; spaceTrg = true; }
                }
            }
            if (upTrg || downTrg) m_players[1].typeCursor = (m_players[1].typeCursor == 0) ? 1 : 0;

            if (bTrg || (mClick && HoverBox(backBtnX, backBtnY, backBtnW, backBtnH))) {
                m_state = MenuState::SELECT_P1_TYPE;
                m_frameCount = 0;
            }
            else if (spaceTrg) {
                m_state = MenuState::CUSTOM_P1_START;
                m_frameCount = 0;
            }
        }
        else if (isCustomState) {
            bool is1P = (m_state == MenuState::CUSTOM_P1_START);
            int pIdx = is1P ? 0 : 1;
            auto& p = m_players[pIdx];

            for (int i = 0; i < 4; ++i) {
                int iy = menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y;
                int valX = menuCX + CUSTOM_BTN_BASE_X;

                if (HoverBox(menuCX - MENU_BOX_HALF_W, iy - MENU_BOX_OFFSET_Y, MENU_BOX_HALF_W * 2, MENU_BOX_H)) {
                    if (mouseMoved) p.customCursor = i;
                    if (mClick) {
                        p.customCursor = i;
                        if (i == 3) spaceTrg = true;
                    }
                }

                if (i < 3 && mClick) {
                    if (HoverBox(valX - CUSTOM_BTN_OFFSET_X, iy - 5, CUSTOM_BTN_SIZE, CUSTOM_BTN_SIZE)) { p.customCursor = i; leftTrg = true; }
                    else if (HoverBox(valX + CUSTOM_BTN_OFFSET_X, iy - 5, CUSTOM_BTN_SIZE, CUSTOM_BTN_SIZE)) { p.customCursor = i; rightTrg = true; }
                }
            }

            if (hitNum != -1 && p.customCursor < 3) {
                if (p.customCursor == 0) p.startNum = hitNum;
                else if (p.customCursor == 1) p.startX = hitNum;
                else if (p.customCursor == 2) p.startY = hitNum;
                p.customCursor++;
            }

            if (upTrg) { p.customCursor--; if (p.customCursor < 0) p.customCursor = 0; }
            if (downTrg) { p.customCursor++; if (p.customCursor > 3) p.customCursor = 3; }

            if (p.customCursor < 3) {
                if (rightTrg) {
                    if (p.customCursor == 0) { p.startNum++; if (p.startNum > GRID_SIZE) p.startNum = 1; }
                    if (p.customCursor == 1) { p.startX++; if (p.startX > GRID_SIZE) p.startX = 1; }
                    if (p.customCursor == 2) { p.startY++; if (p.startY > GRID_SIZE) p.startY = 1; }
                }
                if (leftTrg) {
                    if (p.customCursor == 0) { p.startNum--; if (p.startNum < 1) p.startNum = GRID_SIZE; }
                    if (p.customCursor == 1) { p.startX--; if (p.startX < 1) p.startX = GRID_SIZE; }
                    if (p.customCursor == 2) { p.startY--; if (p.startY < 1) p.startY = GRID_SIZE; }
                }
            }

            // ★修正：マウスの戻るボタンの判定をここにも追加
            if (bTrg || (mClick && HoverBox(backBtnX, backBtnY, backBtnW, backBtnH))) {
                if (p.customCursor > 0) p.customCursor--;
                else if (is1P) { m_state = MenuState::SELECT_P2_TYPE; m_frameCount = 0; }
                else { m_state = MenuState::CUSTOM_P1_START; m_players[0].customCursor = 3; m_frameCount = 0; }
            }

            if (spaceTrg) {
                if (p.customCursor < 3) {
                    p.customCursor++;
                }
                else {
                    if (is1P) {
                        m_state = MenuState::CUSTOM_P2_START;
                        m_players[1].customCursor = 0;
                        m_frameCount = 0;
                    }
                    else {
                        auto* sm = SceneManager::GetInstance();

                        if (m_modeCursor == 0) {
                            int maxStocks = (m_stocksCursor == 0) ? 1 : (m_stocksCursor == 1) ? 3 : 5;
                            sm->SetGameSettings(m_playerCursor, m_modeCursor, maxStocks);
                        }
                        else {
                            int finalScore = TARGET_SCORES[m_scoreCursor];
                            sm->SetGameSettings(m_playerCursor, m_modeCursor, finalScore);
                        }

                        sm->SetPlayer1Settings(m_players[0].typeCursor == 1, m_players[0].startNum, m_players[0].startX - 1, GRID_SIZE - m_players[0].startY);
                        sm->SetPlayer2Settings(m_players[1].typeCursor == 1, m_players[1].startNum, m_players[1].startX - 1, GRID_SIZE - m_players[1].startY);
                        sm->ChangeScene(SceneManager::SCENE_ID::GAME);
                    }
                }
            }
        }
    }

    void TitleScene::Draw() {
        int sw, sh;
        GetDrawScreenSize(&sw, &sh);
        int CX = sw / 2;
        int CY = sh / 2;

        auto& input = InputManager::GetInstance();
        Vector2 m = input.GetMousePos();
        auto HoverBox = [&](int x, int y, int w, int h) {
            return (m.x >= x && m.x <= x + w && m.y >= y && m.y <= y + h);
            };

        // 1. 背景描画
        DrawBox(0, 0, sw, sh, COL_BG(), TRUE);

        // ==========================================
        // ★HLSLによる豪華な背景描画
        // ==========================================
        if (m_psHandle != -1 && m_cbHandle != -1) {
            float* cb = (float*)GetBufferShaderConstantBuffer(m_cbHandle);
            cb[0] = m_shaderTime;
            cb[1] = (float)sw;
            cb[2] = (float)sh;
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
            v[0].pos.x = 0;   v[0].pos.y = 0;   v[0].u = 0.0f; v[0].v = 0.0f;
            v[1].pos.x = sw;  v[1].pos.y = 0;   v[1].u = 1.0f; v[1].v = 0.0f;
            v[2].pos.x = 0;   v[2].pos.y = sh;  v[2].u = 0.0f; v[2].v = 1.0f;
            v[3].pos.x = sw;  v[3].pos.y = 0;   v[3].u = 1.0f; v[3].v = 0.0f;
            v[4].pos.x = sw;  v[4].pos.y = sh;  v[4].u = 1.0f; v[4].v = 1.0f;
            v[5].pos.x = 0;   v[5].pos.y = sh;  v[5].u = 0.0f; v[5].v = 1.0f;

            DrawPrimitive2DToShader(v, 6, DX_PRIMTYPE_TRIANGLELIST);
            SetUsePixelShader(-1);
        }

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 30);
        for (int i = 0; i < sw; i += MAP_CELL_SIZE) DrawLine(i, 0, i, sh, COL_GRID(), 1);
        for (int j = 0; j < sh; j += MAP_CELL_SIZE) DrawLine(0, j, sw, j, COL_GRID(), 1);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawBox(0, 40, sw, 45, COL_P1(), TRUE);
        DrawBox(0, sh - 45, sw, sh - 40, COL_P1(), TRUE);

        // 2. タイトルロゴ（豪華版エフェクト）
        const char* titleText = "超計算マスBATTLE";
        int titleW = GetDrawStringWidthToHandle(titleText, (int)strlen(titleText), m_fontTitle);

        // --- アニメーション用パラメータ ---
        double t = GetNowCount() / 1000.0;
        float floatY = (float)sin(t * 2.0) * 8.0f;
        int titleX = CX - titleW / 2;
        int titleY = 100 + (int)floatY;

        SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
        for (int i = 0; i < 4; ++i) {
            int offset = i * 2;
            DrawStringToHandle(titleX - offset, titleY - offset, titleText, COL_TITLE_SUB(), m_fontTitle);
            DrawStringToHandle(titleX + offset, titleY + offset, titleText, COL_TITLE_SUB(), m_fontTitle);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        float glitchStrength = (float)sin(t * 10.0) * 3.0f;
        SetDrawBlendMode(DX_BLENDMODE_ADD, 150);
        DrawStringToHandle(titleX + (int)glitchStrength, titleY, titleText, GetColor(255, 0, 100), m_fontTitle);
        DrawStringToHandle(titleX - (int)glitchStrength, titleY, titleText, GetColor(0, 100, 255), m_fontTitle);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawStringToHandle(titleX, titleY, titleText, COL_TITLE_MAIN(), m_fontTitle);

        float scanPos = (float)fmod(t * 1.5, 2.0) - 1.0f;
        int shineX = titleX + (int)(scanPos * titleW * 1.5f);

        SetDrawArea(titleX, titleY, titleX + titleW, titleY + 110);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 180);
        for (int i = 0; i < 20; ++i) {
            DrawLine(shineX + i, titleY, shineX + i - 30, titleY + 100, GetColor(255, 255, 255));
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        SetDrawArea(0, 0, sw, sh);

        DrawLine(CX - 500, 230, CX + 500, 230, COL_TITLE_SUB(), 5);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 200);
        DrawLine(CX - 500, 230, CX + 500, 230, COL_TITLE_MAIN(), 2);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 3. 描画ヘルパー
        double time = GetNowCount() / 1000.0;
        int blinkAlpha = (int)(BLINK_BASE_ALPHA + BLINK_AMP_ALPHA * sin(time * M_PI * BLINK_SPEED));

        auto drawBracket = [&](int x, int y, int w, int h, unsigned int col) {
            int d = 15;
            DrawLine(x, y, x + d, y, col, 2); DrawLine(x, y, x, y + d, col, 2);
            DrawLine(x + w - d, y, x + w, y, col, 2); DrawLine(x + w, y, x + w, y + d, col, 2);
            DrawLine(x, y + h - d, x, y + h, col, 2); DrawLine(x, y + h, x + d, y + h, col, 2);
            DrawLine(x + w - d, y + h, x + w, y + h, col, 2); DrawLine(x + w, y + h - d, x + w, y + h, col, 2);
            };

        bool isCustomState = (m_state == MenuState::CUSTOM_P1_START || m_state == MenuState::CUSTOM_P2_START);
        int menuCX = isCustomState ? CX + MENU_CUSTOM_OFFSET_X : CX;
        int menuStartY = CY + MENU_CENTER_OFFSET_Y;

        auto drawSimpleMenu = [&](int cursor, const char* menuTitle, const char* m0, const char* m1, const char* m2 = nullptr) {
            int mtW = GetDrawStringWidthToHandle(menuTitle, (int)strlen(menuTitle), m_fontMenu);
            DrawStringToHandle(menuCX - mtW / 2, menuStartY, menuTitle, COL_GRID(), m_fontMenu);

            const char* items[3] = { m0, m1, m2 };
            for (int i = 0; i < (m2 ? 3 : 2); ++i) {
                int iy = menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y;
                unsigned int baseCol = (cursor == i) ? COL_TEXT_ON() : COL_TEXT_OFF();
                int tw = GetDrawStringWidthToHandle(items[i], (int)strlen(items[i]), m_fontMenu);

                if (cursor == i) {
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, blinkAlpha / 3);
                    DrawBox(menuCX - MENU_BOX_HALF_W, iy - MENU_BOX_OFFSET_Y, menuCX + MENU_BOX_HALF_W, iy + MENU_BOX_H - MENU_BOX_OFFSET_Y, baseCol, TRUE);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                    drawBracket(menuCX - MENU_BOX_HALF_W, iy - 15, MENU_BOX_HALF_W * 2, 70, baseCol);
                    DrawStringToHandle(menuCX - 380, iy, " >>", baseCol, m_fontMenu);
                    DrawStringToHandle(menuCX + 335, iy, "<< ", baseCol, m_fontMenu);
                }
                DrawStringToHandle(menuCX - tw / 2, iy, items[i], baseCol, m_fontMenu);
            }
            };

        // 4. 各ステートごとの描画
        if (m_state == MenuState::PRESS_START) {
            // ★修正：ANY KEYから変更した旨を案内する文言に変更
            const char* pushText = "- PRESS SPACE OR CLICK TO START -";
            int ptW = GetDrawStringWidthToHandle(pushText, (int)strlen(pushText), m_fontMenu);

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, blinkAlpha);
            DrawStringToHandle(CX - ptW / 2, CY + 100, pushText, COL_GRID(), m_fontMenu);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
        else if (m_state == MenuState::SELECT_PLAYERS) { drawSimpleMenu(m_playerCursor - 1, "【 バトル方式 】", "シングルバトル", "オフラインバトル"); }
        else if (m_state == MenuState::SELECT_MODE) { drawSimpleMenu(m_modeCursor, "【 プレイモード 】", "ノーマルバトル", "カウントバトル"); }
        else if (m_state == MenuState::SELECT_CLASSIC_STOCKS) { drawSimpleMenu(m_stocksCursor, "【 残機設定 】", "残機: 1", "残機: 3", "残機: 5"); }
        else if (m_state == MenuState::SELECT_SCORE) { drawSimpleMenu(m_scoreCursor, "【 目標スコア設定 】", "目標スコア: 53", "目標スコア: 103", "目標スコア: 223"); }
        else if (m_state == MenuState::SELECT_P1_TYPE) { drawSimpleMenu(m_players[0].typeCursor, "【 1P 操作設定 】", "プレイヤー", "NPC"); }
        else if (m_state == MenuState::SELECT_P2_TYPE) { drawSimpleMenu(m_players[1].typeCursor, "【 2P 操作設定 】", "プレイヤー", "NPC"); }
        else if (isCustomState) {
            bool is1P = (m_state == MenuState::CUSTOM_P1_START);
            int pIdx = is1P ? 0 : 1;
            auto& p = m_players[pIdx];

            std::string menuTitle = is1P ? "【 1P 初期設定 】" : "【 2P 初期設定 】";
            int mtW = GetDrawStringWidthToHandle(menuTitle.c_str(), (int)menuTitle.length(), m_fontMenu);
            DrawStringToHandle(menuCX - mtW / 2, menuStartY, menuTitle.c_str(), is1P ? COL_P1() : COL_P2(), m_fontMenu);

            const char* items[3] = { "初期体力", "初期Ｘ座標 (1-9)", "初期Ｙ座標 (1-9)" };
            int vals[3] = { p.startNum, p.startX, p.startY };

            for (int i = 0; i < 4; ++i) {
                int iy = menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y;
                unsigned int color = (p.customCursor == i) ? COL_TEXT_ON() : COL_TEXT_OFF();

                if (p.customCursor == i) {
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, blinkAlpha / 3);
                    DrawBox(menuCX - MENU_BOX_HALF_W, iy - MENU_BOX_OFFSET_Y, menuCX + MENU_BOX_HALF_W, iy + MENU_BOX_H - MENU_BOX_OFFSET_Y, color, TRUE);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                    drawBracket(menuCX - MENU_BOX_HALF_W, iy - 15, MENU_BOX_HALF_W * 2, 70, color);
                }

                if (i < 3) {
                    char nameBuf[128];
                    sprintf_s(nameBuf, "%-20s", items[i]);
                    DrawStringToHandle(menuCX - 320, iy, nameBuf, color, m_fontMenu);

                    int valX = menuCX + CUSTOM_BTN_BASE_X;

                    bool hoverL = HoverBox(valX - CUSTOM_BTN_OFFSET_X, iy - 5, CUSTOM_BTN_SIZE, CUSTOM_BTN_SIZE);
                    unsigned int colL = hoverL ? COL_WHITE() : color;
                    DrawBox(valX - CUSTOM_BTN_OFFSET_X, iy - 5, valX - 10, iy + 45, colL, FALSE);
                    DrawStringToHandle(valX - 45, iy, "<", colL, m_fontMenu);

                    DrawFormatStringToHandle(valX + 15, iy - 4, color, m_fontNumber, "%d", vals[i]);

                    bool hoverR = HoverBox(valX + CUSTOM_BTN_OFFSET_X, iy - 5, CUSTOM_BTN_SIZE, CUSTOM_BTN_SIZE);
                    unsigned int colR = hoverR ? COL_WHITE() : color;
                    DrawBox(valX + CUSTOM_BTN_OFFSET_X, iy - 5, valX + 110, iy + 45, colR, FALSE);
                    DrawStringToHandle(valX + 75, iy, ">", colR, m_fontMenu);
                }
                else {
                    const char* decideText = is1P ? "設定完了 (NEXT)" : "バトル開始 (GAME START)";
                    int tw = GetDrawStringWidthToHandle(decideText, (int)strlen(decideText), m_fontMenu);
                    DrawStringToHandle(menuCX - tw / 2, iy, decideText, color, m_fontMenu);

                    if (p.customCursor == i) {
                        DrawStringToHandle(menuCX - tw / 2 - 60, iy, " >>", color, m_fontMenu);
                        DrawStringToHandle(menuCX + tw / 2 + 20, iy, "<< ", color, m_fontMenu);
                    }
                }
            }

            // ミニマップ描画
            int mapBaseX = CX + MAP_BASE_OFFSET_X;
            int mapBaseY = menuStartY + MAP_BASE_OFFSET_Y;

            DrawStringToHandle(mapBaseX + 80, mapBaseY - 50, "【 配置プレビュー 】", COL_TITLE_MAIN(), m_fontSmall);

            DrawBox(mapBaseX - 5, mapBaseY - 5, mapBaseX + MAP_CELL_SIZE * GRID_SIZE + 5, mapBaseY + MAP_CELL_SIZE * GRID_SIZE + 5, COL_GRID(), FALSE);
            DrawStringToHandle(mapBaseX - 40, mapBaseY - 40, "Y", COL_GRID(), m_fontSmall);
            DrawStringToHandle(mapBaseX + MAP_CELL_SIZE * GRID_SIZE + 15, mapBaseY + MAP_CELL_SIZE * GRID_SIZE - 10, "X", COL_GRID(), m_fontSmall);

            for (int y = 0; y < GRID_SIZE; ++y) {
                for (int x = 0; x < GRID_SIZE; ++x) {
                    int drawX = mapBaseX + x * MAP_CELL_SIZE;
                    int drawY = mapBaseY + y * MAP_CELL_SIZE;
                    int uiX = x + 1;
                    int uiY = GRID_SIZE - y;

                    DrawBox(drawX, drawY, drawX + MAP_CELL_SIZE, drawY + MAP_CELL_SIZE, COL_BG(), TRUE);
                    DrawBox(drawX, drawY, drawX + MAP_CELL_SIZE, drawY + MAP_CELL_SIZE, COL_TITLE_SUB(), FALSE);

                    if (uiX == m_players[0].startX && uiY == m_players[0].startY) {
                        int alpha = (is1P) ? blinkAlpha : 150;
                        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
                        DrawBox(drawX + 2, drawY + 2, drawX + MAP_CELL_SIZE - 2, drawY + MAP_CELL_SIZE - 2, COL_P1(), TRUE);
                        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                        DrawFormatStringToHandle(drawX + 16, drawY + 10, COL_BLACK(), m_fontSmall, "%d", m_players[0].startNum);
                    }
                    if (uiX == m_players[1].startX && uiY == m_players[1].startY) {
                        int alpha = (!is1P) ? blinkAlpha : 150;
                        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
                        DrawBox(drawX + 2, drawY + 2, drawX + MAP_CELL_SIZE - 2, drawY + MAP_CELL_SIZE - 2, COL_P2(), TRUE);
                        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                        DrawFormatStringToHandle(drawX + 16, drawY + 10, COL_BLACK(), m_fontSmall, "%d", m_players[1].startNum);
                    }
                }
            }
        }

        // ==========================================
        // ★修正：マウス用の「戻るボタン」の描画（PRESS_START以外で常に表示）
        // ==========================================
        if (m_state != MenuState::PRESS_START) {
            int backBtnX = CX - 150;
            int backBtnY = sh - 150;
            int backBtnW = 300;
            int backBtnH = 60;
            bool isHover = HoverBox(backBtnX, backBtnY, backBtnW, backBtnH);
            unsigned int btnCol = isHover ? COL_TEXT_ON() : COL_TEXT_OFF();

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, isHover ? 200 : 120);
            DrawBox(backBtnX, backBtnY, backBtnX + backBtnW, backBtnY + backBtnH, COL_BG(), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawBox(backBtnX, backBtnY, backBtnX + backBtnW, backBtnY + backBtnH, btnCol, FALSE);

            const char* backStr = "戻る (B)";
            int bw = GetDrawStringWidthToHandle(backStr, (int)strlen(backStr), m_fontMenu);
            DrawStringToHandle(backBtnX + (backBtnW - bw) / 2, backBtnY + 10, backStr, btnCol, m_fontMenu);
        }

        // 5. 画面下部のオペレーションガイド
        DrawBox(0, sh - 70, sw, sh - 15, GetColor(10, 20, 40), TRUE);

        std::string guideText;
        if (m_state == MenuState::PRESS_START) { guideText = ""; }
        else if (m_state == MenuState::SELECT_PLAYERS) { guideText = "[↑][↓]/CLICK: 項目選択   |   [SPACE]: 決定"; }
        else if (isCustomState) { guideText = "[↑][↓]/CLICK: 項目選択   |   [<][>]/CLICK: 数値変更   |   [1-9]: 直接入力"; }
        else { guideText = "[↑][↓]/CLICK: 項目選択   |   [SPACE]: 決定"; }

        if (!guideText.empty()) {
            int gw = GetDrawStringWidthToHandle(guideText.c_str(), (int)guideText.length(), m_fontSmall);
            int guideX = isCustomState ? (CX - gw / 2 - 100) : (CX - gw / 2);
            DrawStringToHandle(guideX, sh - 55, guideText.c_str(), COL_TEXT_OFF(), m_fontSmall);
        }
    }

    void TitleScene::Release() {
        if (m_fontTitle != -1)  DeleteFontToHandle(m_fontTitle);
        if (m_fontMenu != -1)   DeleteFontToHandle(m_fontMenu);
        if (m_fontSmall != -1)  DeleteFontToHandle(m_fontSmall);
        if (m_fontNumber != -1) DeleteFontToHandle(m_fontNumber);

        if (m_psHandle != -1)   DeleteShader(m_psHandle);
        if (m_cbHandle != -1)   DeleteShaderConstantBuffer(m_cbHandle);
    }

} // namespace App