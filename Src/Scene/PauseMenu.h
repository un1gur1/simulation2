#pragma once
#include <DxLib.h>

namespace App {

    class PauseMenu {
    public:
        enum class Result { NONE, RESUME, TITLE, EXIT };

        PauseMenu();
        void Init();
        Result Update();
        void Draw();

    private:
        int m_cursor;
        float m_angle; // アスタリスク回転用

        // 演算子アイコン描画用ヘルパー
        void DrawOperatorIcon(int x, int y, int size, unsigned int color);
    };

} // namespace App