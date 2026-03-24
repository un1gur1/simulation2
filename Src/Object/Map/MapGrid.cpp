#include "MapGrid.h"
#include <DxLib.h>

namespace App {

    MapGrid::MapGrid(int tileSize, Vector2 offset)
        : m_tileSize(tileSize)
        , m_offset(offset)
    {
        // 最初はすべてのマスを '+'（足し算）で初期化
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                m_symbols[x][y] = '+';
            }
        }
    }

    // 画面の座標（マウス位置など）から、どのマスにいるかを計算
    IntVector2 MapGrid::ScreenToGrid(const Vector2& pos) const {
        return {
            static_cast<int>((pos.x - m_offset.x) / m_tileSize),
            static_cast<int>((pos.y - m_offset.y) / m_tileSize)
        };
    }

    // マスの座標（0,0など）から、そのマスの中心の画面座標を計算
    Vector2 MapGrid::GetCellCenter(int x, int y) const {
        return {
            m_offset.x + x * m_tileSize + m_tileSize / 2.0f,
            m_offset.y + y * m_tileSize + m_tileSize / 2.0f
        };
    }

    // 指定された座標が盤面の中に収まっているかチェック
    bool MapGrid::IsWithinBounds(int x, int y) const {
        return x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT;
    }

    // マスの記号を書き換える（デバッグ用などで使用）
    void MapGrid::SetSymbol(int x, int y, char sym) {
        if (IsWithinBounds(x, y)) {
            m_symbols[x][y] = sym;
        }
    }

    // マスの記号を取得する
    char MapGrid::GetSymbol(int x, int y) const {
        if (!IsWithinBounds(x, y)) return '+';
        return m_symbols[x][y];
    }

    void MapGrid::Draw() const {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                Vector2 pos = GetCellCenter(x, y);

                // マスの外枠（少し青白い暗い色）
                DrawBox((int)pos.x - 39, (int)pos.y - 39, (int)pos.x + 39, (int)pos.y + 39, GetColor(60, 60, 80), FALSE);

                // マスの塗りつぶし（深い紺色）
                DrawBox((int)pos.x - 38, (int)pos.y - 38, (int)pos.x + 38, (int)pos.y + 38, GetColor(30, 30, 45), TRUE);

                // 演算子記号の描画（マスの中心に控えめに描画）
                DrawFormatString((int)pos.x - 6, (int)pos.y - 10, GetColor(70, 70, 90), "%c", m_symbols[x][y]);
            }
        }
    }

} // namespace App