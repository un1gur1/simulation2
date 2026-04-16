#include "PauseMenu.h"
#include "../Input/InputManager.h" 
#include <cmath>

namespace App {

    PauseMenu::PauseMenu()
        : m_cursor(0)
        , m_angle(0.0f)
    {
    }

    void PauseMenu::Init() {
        m_cursor = 0;
        m_angle = 0.0f;
    }

    PauseMenu::Result PauseMenu::Update() {
        m_angle += 0.05f;

        auto& input = InputManager::GetInstance();

        // ==========================================
        // 1. マウス操作
        // ==========================================
        int mx, my;
        GetMousePoint(&mx, &my);
        static int prevMx = 0, prevMy = 0;
        bool mouseMoved = (mx != prevMx || my != prevMy);
        prevMx = mx; prevMy = my;

        auto HoverBox = [&](int x, int y, int w, int h) {
            return (mx >= x && mx <= x + w && my >= y && my <= y + h);
            };

        int sw = 1920;

        // マウスホバーでカーソル移動（マウスが動いた時のみ）
        if (mouseMoved) {
            for (int i = 0; i < 3; ++i) {
                int ty = 520 + i * 100;
                if (HoverBox(sw / 2 - 450, ty - 10, 900, 80)) {
                    m_cursor = i;
                    break;
                }
            }
        }

        // マウスクリックで決定
        if (input.IsMouseLeftTrg()) {
            for (int i = 0; i < 3; ++i) {
                int ty = 520 + i * 100;
                if (HoverBox(sw / 2 - 450, ty - 10, 900, 80)) {
                    m_cursor = i;
                    goto DECIDE;
                }
            }
        }

        // ==========================================
        // 2. キーボード操作（十字キー ＆ WASD 対応）
        // ==========================================

        // 上移動：↑キー または Wキー
        if (input.IsTrgDown(KEY_INPUT_UP) || input.IsTrgDown(KEY_INPUT_W)) {
            m_cursor = (m_cursor - 1 + 3) % 3;
        }
        // 下移動：↓キー または Sキー
        if (input.IsTrgDown(KEY_INPUT_DOWN) || input.IsTrgDown(KEY_INPUT_S)) {
            m_cursor = (m_cursor + 1) % 3;
        }

        // 決定：Space または Enter
        if (input.IsTrgDown(KEY_INPUT_SPACE) || input.IsTrgDown(KEY_INPUT_RETURN)) {
            goto DECIDE;
        }

        return Result::NONE;

        // 決定処理
    DECIDE:
        if (m_cursor == 0) return Result::RESUME;
        if (m_cursor == 1) return Result::TITLE;
        if (m_cursor == 2) return Result::EXIT;

        return Result::NONE;
    }

    void PauseMenu::Draw() {
        int sw = 1920, sh = 1080;

        // 1. 背景オーバーレイ
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
        DrawBox(0, 0, sw, sh, GetColor(2, 5, 25), TRUE);

        // マス目（グリッド）描画
        for (int i = 0; i < sw; i += 80) DrawLine(i, 0, i, sh, GetColor(0, 80, 180), 1);
        for (int j = 0; j < sh; j += 80) DrawLine(0, j, sw, j, GetColor(0, 80, 180), 1);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 2. 演算子アイコン「＊」
        DrawOperatorIcon(sw / 2, 220, 60, GetColor(0, 255, 255));

        SetFontSize(80);
        DrawString(sw / 2 - 280, 300, "SYSTEM PAUSED", GetColor(200, 240, 255));

        // 3. メニュー描画
        SetFontSize(40);
        const char* items[] = {
            "[A-01] 演算再開 : RESUME PROCESS",
            "[A-02] タイトルへ : EXIT TO TITLE",
            "[A-03] システム終了 : SHUTDOWN"
        };

        for (int i = 0; i < 3; ++i) {
            int ty = 520 + i * 100;
            unsigned int col = (m_cursor == i) ? GetColor(255, 160, 0) : GetColor(70, 130, 180);

            if (m_cursor == i) {
                // 選択中のセル背景を光らせる
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 60);
                DrawBox(sw / 2 - 450, ty - 10, sw / 2 + 450, ty + 70, col, TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

                // 枠線
                DrawBox(sw / 2 - 450, ty - 10, sw / 2 + 450, ty + 70, col, FALSE);
                DrawString(sw / 2 - 520, ty + 5, ">>", col);
            }
            DrawString(sw / 2 - 400, ty + 5, items[i], col);
        }
    }

    void PauseMenu::DrawOperatorIcon(int x, int y, int size, unsigned int color) {
        for (int i = 0; i < 8; ++i) {
            float th = m_angle + (DX_PI_F * 2.0f / 8.0f) * i;
            int x2 = x + (int)(cosf(th) * size);
            int y2 = y + (int)(sinf(th) * size);
            DrawLine(x, y, x2, y2, color, 5);
            DrawBox(x2 - 5, y2 - 5, x2 + 5, y2 + 5, color, TRUE);
        }
    }

} // namespace App