#pragma once

#include <DxLib.h>
#include "../../Common/Vector2.h"

namespace App {


    class MapGrid {
    private:
        static constexpr int WIDTH = 10;
        static constexpr int HEIGHT = 10;

        int m_tileSize;      // 1マスのサイズ
        Vector2 m_offset;    // 描画開始位置
        char m_symbols[WIDTH][HEIGHT]; // 各マスの演算子記号

    public:
        // コンストラクタ
        explicit MapGrid(int tileSize = 80, Vector2 offset = Vector2(100, 100));
        ~MapGrid() = default;

        // 座標変換
        IntVector2 ScreenToGrid(const Vector2& pos) const;
        Vector2 GetCellCenter(int x, int y) const;

        // 判定と取得
        bool IsWithinBounds(int x, int y) const;
        void SetSymbol(int x, int y, char sym);
        char GetSymbol(int x, int y) const;

        // 描画
        void Draw() const;

        // ゲッター
        int GetWidth() const { return WIDTH; }
        int GetHeight() const { return HEIGHT; }
    };

} // namespace App