#include "MapGrid.h"
#include <random>
#include <string>
#include <cmath>
#include <DxLib.h>

namespace App {

    MapGrid::MapGrid(int tileSize, Vector2 offset)
        : m_tileSize(tileSize)
        , m_offset(offset)
        , m_totalTurns(0)
        , m_currentCycleTick(0)
    {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                m_terrain[x][y] = 0;
            }
        }
        InitializeSpawnPoints();
    }

    void MapGrid::InitializeSpawnPoints() {
        struct SpawnArea { int minX, maxX, minY, maxY; };
        std::vector<SpawnArea> areas = {
            {1, 2, 1, 2}, {6, 7, 1, 2},
            {1, 2, 6, 7}, {6, 7, 6, 7},
            {4, 4, 4, 4}
        };

        std::random_device rd;
        std::mt19937 gen(rd());
        std::vector<char> baseSequence = { '+', '-', '*', '/' };

        for (const auto& area : areas) {
            std::uniform_int_distribution<> distX(area.minX, area.maxX);
            std::uniform_int_distribution<> distY(area.minY, area.maxY);
            std::uniform_int_distribution<> distSeq(0, baseSequence.size() - 1);

            SpawnPoint sp;
            sp.pos = { distX(gen), distY(gen) };
            sp.sequence = baseSequence;
            sp.currentIndex = distSeq(gen);
            sp.currentSymbol = sp.sequence[sp.currentIndex];
            sp.isAvailable = true;

            m_spawnPoints.push_back(sp);
        }
    }

    IntVector2 MapGrid::ScreenToGrid(const Vector2& pos) const {
        return {
            static_cast<int>((pos.x - m_offset.x) / m_tileSize),
            static_cast<int>((pos.y - m_offset.y) / m_tileSize)
        };
    }

    Vector2 MapGrid::GetCellCenter(int x, int y) const {
        return {
            m_offset.x + x * m_tileSize + m_tileSize / 2.0f,
            m_offset.y + y * m_tileSize + m_tileSize / 2.0f
        };
    }

    bool MapGrid::IsWithinBounds(int x, int y) const {
        return x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT;
    }

    void MapGrid::UpdateTurn() {
        m_totalTurns++;
        m_currentCycleTick++;

        if (m_currentCycleTick >= SPAWN_INTERVAL) {
            m_currentCycleTick = 0;
            for (auto& sp : m_spawnPoints) {
                sp.currentIndex = (sp.currentIndex + 1) % sp.sequence.size();
                sp.currentSymbol = sp.sequence[sp.currentIndex];
                sp.isAvailable = true;
            }
        }
    }

    char MapGrid::PickUpItem(int x, int y) {
        for (auto& sp : m_spawnPoints) {
            if (sp.pos.x == x && sp.pos.y == y && sp.isAvailable) {
                sp.isAvailable = false;
                return sp.currentSymbol;
            }
        }
        return '\0';
    }

    char MapGrid::GetItemAt(int x, int y) const {
        for (const auto& sp : m_spawnPoints) {
            if (sp.pos.x == x && sp.pos.y == y && sp.isAvailable) {
                return sp.currentSymbol;
            }
        }
        return '\0';
    }

    void MapGrid::Draw() const {
        // 1. 盤面の描画
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                Vector2 pos = GetCellCenter(x, y);
                DrawBox((int)pos.x - 39, (int)pos.y - 39, (int)pos.x + 39, (int)pos.y + 39, GetColor(60, 60, 80), FALSE);
                DrawBox((int)pos.x - 38, (int)pos.y - 38, (int)pos.x + 38, (int)pos.y + 38, GetColor(30, 30, 45), TRUE);
            }
        }

        // ★追加：アニメーション用の現在時刻を取得
        int nowTime = GetNowCount();

        // 2. アイテム（カード）の描画
        for (const auto& sp : m_spawnPoints) {
            Vector2 pos = GetCellCenter(sp.pos.x, sp.pos.y);

            // --- カードデザインの描画 ---
            if (sp.isAvailable) {
                // ★追加：ふわふわ（Y座標のオフセット）
                // 200.0fを小さくすると速く、5.0fを大きくすると揺れ幅が大きくなります
                float offsetY = std::sin(nowTime / 200.0f) * 5.0f;

                // 色と記号の事前判定
                int symbolColor = GetColor(20, 20, 20);
                std::string displaySym = "";

                if (sp.currentSymbol == '+') { symbolColor = GetColor(220, 50, 50);  displaySym = "+"; }
                else if (sp.currentSymbol == '-') { symbolColor = GetColor(50, 100, 220); displaySym = "-"; }
                else if (sp.currentSymbol == '*') { symbolColor = GetColor(50, 180, 50);  displaySym = "x"; }
                else if (sp.currentSymbol == '/') { symbolColor = GetColor(180, 50, 180); displaySym = "÷"; }

                // ★追加：ぽわぽわ（後光オーラの明滅）
                // 150.0fの周期で、透明度が 40 ～ 120 の間を滑らかに往復します
                int pulseAlpha = 80 + (int)(std::sin(nowTime / 150.0f) * 40);
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, pulseAlpha);
                // カードの後ろに、記号と同じ色でぼんやり光る円を描く
                DrawCircle((int)pos.x, (int)(pos.y - 8 + offsetY), 35, symbolColor, TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

                // カードの座標計算（offsetY を足して全体を揺らす）
                int cardW = 44;
                int cardH = 58;
                int cx1 = (int)pos.x - cardW / 2;
                int cy1 = (int)pos.y - cardH / 2 - 8 + (int)offsetY;
                int cx2 = (int)pos.x + cardW / 2;
                int cy2 = (int)pos.y + cardH / 2 - 8 + (int)offsetY;

                // ① 影の描画（立体感）
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
                DrawBox(cx1 + 5, cy1 + 5, cx2 + 5, cy2 + 5, GetColor(10, 10, 15), TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

                // ② カードの背景（白）
                DrawBox(cx1, cy1, cx2, cy2, GetColor(245, 245, 245), TRUE);
                // ③ カードの枠線
                DrawBox(cx1, cy1, cx2, cy2, GetColor(100, 100, 120), FALSE);

                // ④ 記号の描画
                SetFontSize(28); // 枠に収まるように少しフォントサイズ指定を追加
                DrawFormatString(cx1 + 12, cy1 + 16, symbolColor, "%s", displaySym.c_str());
            }

            // --- 予測情報の描画 ---
            char nextSym = sp.sequence[(sp.currentIndex + 1) % sp.sequence.size()];
            std::string nextDisplaySym = (nextSym == '/') ? "÷" : (nextSym == '*' ? "x" : std::string(1, nextSym));
            int nextTurn = GetTurnsUntilNextSpawn();

            int color = sp.isAvailable ? GetColor(120, 120, 120) : GetColor(100, 200, 255);
            DrawFormatString((int)pos.x - 36, (int)pos.y + 24, color, "Next:%s(%d)", nextDisplaySym.c_str(), nextTurn);
        }
    }

} // namespace App