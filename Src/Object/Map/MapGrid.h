#pragma once

#include <DxLib.h>
#include <vector>
#include "../../Common/Vector2.h"

namespace App {

    // 拡張ポイント1：演算子アイテムの発生地点（スポナー）を管理する構造体
    struct SpawnPoint {
        IntVector2 pos;              // 現在のマス座標
        std::vector<char> sequence;  // 変化するサイクルの順序（例: '+', '-', '*', '/'）
        int currentIndex;            // 現在の配列インデックス
        char currentSymbol;          // 現在拾える記号
        bool isAvailable;            // マスにアイテムが存在しているか
    };

    class MapGrid {
    private:
        static constexpr int WIDTH = 9;  // 10から9へ変更（中央マスを作るため）
        static constexpr int HEIGHT = 9;

        int m_tileSize;
        Vector2 m_offset;

        // 拡張ポイント2：マスごとの地形データ（0:平地, 1:進入不可の壁 など）
        int m_terrain[WIDTH][HEIGHT];

        // 演算子アイテムのリスト
        std::vector<SpawnPoint> m_spawnPoints;

        // ★追加：ターン管理用の変数
        int m_totalTurns;       // ゲーム開始からの総経過ターン
        int m_currentCycleTick; // 次の再生成までのカウント
        static constexpr int SPAWN_INTERVAL = 3; // 3ターンに1回湧く

        // 内部の初期化処理
        void InitializeSpawnPoints();

    public:
        explicit MapGrid(int tileSize = 80, Vector2 offset = Vector2(100, 100));
        ~MapGrid() = default;

        // ★追加：ターン情報のゲッター
        int GetTurnsUntilNextSpawn() const { return SPAWN_INTERVAL - m_currentCycleTick; }
        int GetTotalTurns() const { return m_totalTurns; }

        IntVector2 ScreenToGrid(const Vector2& pos) const;
        Vector2 GetCellCenter(int x, int y) const;
        bool IsWithinBounds(int x, int y) const;

        // 拡張ポイント3：ターン経過時の処理（ここでアイテムをサイクルさせる）
        void UpdateTurn();

        // 拡張ポイント4：指定マスのアイテムを拾う処理（拾うとマップから消える）
        char PickUpItem(int x, int y);

        // 指定マスにアイテムがあるか確認する用（UI表示や敵AIの判断用）
        char GetItemAt(int x, int y) const;

        void Draw() const;

        int GetWidth() const { return WIDTH; }
        int GetHeight() const { return HEIGHT; }
    };

} // namespace App