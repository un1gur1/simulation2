#pragma once
#include <DxLib.h>

namespace App {
    namespace Config {
        // 画面サイズ
        constexpr int SCREEN_W = 1920;
        constexpr int SCREEN_H = 1080;

        // UIレイアウト
        constexpr int HEADER_H = 70;
        constexpr int BOTTOM_PANEL_Y = 830;
        constexpr int LOG_PANEL_Y = 760;

        // 色の定義（インライン関数で高速化）
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
    }
}