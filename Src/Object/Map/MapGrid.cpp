#define NOMINMAX
#include "MapGrid.h"
#include <DxLib.h>
#include <cmath>
#include <string>

namespace App {

    MapGrid::MapGrid(int tileSize, Vector2 offset)
        : m_tileSize(tileSize)
        , m_offset(offset)
        , m_totalTurns(0)
        , m_currentCycleTick(0)
    {
        InitializeSpawnPoints();
    }

    void MapGrid::InitializeSpawnPoints() {
        m_spawnPoints.clear();

        // ★共通の湧き順シーケンス
        std::vector<char> baseSeq = { '+', '-', '*', '/' };

        // ★配置場所の定義（9x9マップ）
        struct PointDef { int x, y, startIdx; };
        PointDef defs[] = {
            { 2, 2, 0 }, // 左上：＋ から
            { 6, 2, 1 }, // 右上：－ から
            { 2, 6, 2 }, // 左下：＊ から
            { 6, 6, 3    }, // 右下：／ から

            // --- 中央 ---
            { 4, 4, 1 }  // 中央：＋ から
        };

        // 配列の要素数分だけSpawnPointを作成
        for (const auto& d : defs) {
            SpawnPoint sp;
            sp.pos = { d.x, d.y };
            sp.sequence = baseSeq;
            sp.currentIndex = d.startIdx % baseSeq.size(); // 安全装置
            sp.currentSymbol = sp.sequence[sp.currentIndex];
            sp.isAvailable = true; // 最初は全て配置
            m_spawnPoints.push_back(sp);
        }
    }

    char MapGrid::PickUpItem(int x, int y) {
        for (auto& sp : m_spawnPoints) {
            if (sp.pos.x == x && sp.pos.y == y && sp.isAvailable) {
                char picked = sp.currentSymbol;
                sp.isAvailable = false; // 取られた状態にする

                // 次のアイテムへ進める
                sp.currentIndex = (sp.currentIndex + 1) % sp.sequence.size();
                sp.currentSymbol = sp.sequence[sp.currentIndex];

                return picked;
            }
        }
        return '\0';
    }

    void MapGrid::UpdateTurn() {
        m_totalTurns++;
        m_currentCycleTick++;

        if (m_currentCycleTick >= SPAWN_INTERVAL) {
            m_currentCycleTick = 0;

            for (auto& sp : m_spawnPoints) {
                // 取られている（空の）場所だけ、次を出現させる
                if (!sp.isAvailable) {
                    sp.isAvailable = true;
                }
            }
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
        return x >= 0 && x < 9 && y >= 0 && y < 9;
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
        // --- 1. 盤面描画 ---
        // 紺色ベースの画面に完全に馴染む、深く静かなマス目
        unsigned int lineCol = GetColor(40, 45, 60);
        unsigned int fillCol = GetColor(15, 18, 25);

        for (int y = 0; y < 9; y++) {
            for (int x = 0; x < 9; x++) {
                Vector2 pos = GetCellCenter(x, y);
                // 四角形はジャギーが出ないのでそのままDrawBoxでOK
                DrawBox((int)pos.x - 39, (int)pos.y - 39, (int)pos.x + 39, (int)pos.y + 39, lineCol, FALSE);
                DrawBox((int)pos.x - 38, (int)pos.y - 38, (int)pos.x + 38, (int)pos.y + 38, fillCol, TRUE);
            }
        }

        // --- 2. アイテム描画（高級盤面はめ込み ＆ ツルツルAA仕様） ---
        double time = GetNowCount() / 1000.0;
        // ★ floatにして滑らかに脈動させる
        float pulse = (float)(sin(time * 5.0) * 4.0);

        for (const auto& sp : m_spawnPoints) {
            Vector2 basePos = GetCellCenter(sp.pos.x, sp.pos.y);
            // ★ AA用に座標もfloatで保持
            float drawX = basePos.x;
            float drawY = basePos.y;

            if (!sp.isAvailable) {
                // ★ ホログラム（出現待ち）演出：ダークな盤面に浮かぶ電子予測
                SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
                // ★ DrawCircleAAに変更（第4引数はポリゴン分割数。64でかなり綺麗）
                DrawCircleAA(drawX, drawY, 22.0f + pulse / 2.0f, 64, GetColor(80, 100, 120), FALSE, 2.0f);

                SetFontSize(24);
                char waitStr[2] = { sp.currentSymbol, '\0' };
                int tw = GetDrawStringWidth(waitStr, 1);
                // 文字はボヤけないように int にキャスト
                DrawString((int)drawX - tw / 2, (int)drawY - 12, waitStr, GetColor(100, 140, 180));
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                continue;
            }

            // アイテムの色決定（宝石のようなリッチで深い色合い）
            unsigned int itemCol, addCol, darkCol;
            if (sp.currentSymbol == '+') {
                itemCol = GetColor(200, 30, 50);  // ルビー（赤）
                addCol = GetColor(255, 100, 100);
                darkCol = GetColor(80, 10, 20);
            }
            else if (sp.currentSymbol == '-') {
                itemCol = GetColor(30, 120, 220); // サファイア（青）
                addCol = GetColor(100, 180, 255);
                darkCol = GetColor(10, 30, 80);
            }
            else if (sp.currentSymbol == '*') {
                itemCol = GetColor(40, 180, 60);  // エメラルド（緑）
                addCol = GetColor(120, 255, 150);
                darkCol = GetColor(15, 60, 20);
            }
            else {
                itemCol = GetColor(180, 40, 200); // アメジスト（紫）
                addCol = GetColor(220, 100, 255);
                darkCol = GetColor(60, 10, 80);
            }

            // ① 盤面に漏れ出すエネルギー（オーラ）
            for (int i = 0; i < 2; ++i) {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 60 - i * 20);
                DrawCircleAA(drawX, drawY, 34.0f + i * 8.0f + pulse, 64, itemCol, TRUE);
            }
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            // ② アイテム本体（多層クリスタル構造）
            DrawCircleAA(drawX, drawY, 28.0f, 64, darkCol, TRUE); // 外郭の暗い色
            DrawCircleAA(drawX, drawY, 24.0f, 64, itemCol, TRUE); // メインカラー

            // ③ 加算合成で内部からの発光感
            SetDrawBlendMode(DX_BLENDMODE_ADD, 150);
            DrawCircleAA(drawX, drawY, 18.0f, 64, addCol, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            // ④ 盤面にはめ込まれているような硬質なリング
            DrawCircleAA(drawX, drawY, 26.0f, 64, GetColor(200, 220, 255), FALSE, 2.0f);

            // ⑤ 強い光沢
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
            DrawCircleAA(drawX - 8.0f, drawY - 8.0f, 6.0f, 32, GetColor(255, 255, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            // ⑥ 記号（ユニットに合わせて「白文字＋暗い影」に変更）
            SetFontSize(38);
            char symStr[2] = { sp.currentSymbol, '\0' };
            int tw = GetDrawStringWidth(symStr, 1);

            // 彫り込まれている感のあるドロップシャドウ
            DrawString((int)drawX - tw / 2 + 2, (int)drawY - 18 + 2, symStr, GetColor(20, 20, 30));
            // 本体文字（発光する白）
            DrawString((int)drawX - tw / 2, (int)drawY - 18, symStr, GetColor(255, 255, 255));
        }
    }

} // namespace App