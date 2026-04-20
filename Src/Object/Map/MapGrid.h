#pragma once

#include <DxLib.h>
#include <vector>
#include "../../Common/Vector2.h"

namespace App {

    // ★ バトルモードの追加
    enum class BattleRuleMode {
        CLASSIC,    // クラシックモード（ノーマルバトル）
        ZERO_ONE    // カウントバトル（ゼロワン）
    };

    // MapGrid.h の一部
    struct SpawnPoint {
        IntVector2 pos;
        std::vector<char> sequence;
        int currentIndex;   // 今に出現予定のアイテム番号
        char currentSymbol; // 現在見えているアイテム
        bool isAvailable;   // 今アイテムが配置されているか
    };

    class MapGrid {
    private:
        static constexpr int WIDTH = 9;  // 10から9へ変更（真ん中マス消すため）
        static constexpr int HEIGHT = 9;

        int m_tileSize;
        Vector2 m_offset;

        // トリポイント2：マスごとの地形データ（0:普通地, 1:進入不可の壁 など）
        int m_terrain[WIDTH][HEIGHT];

        // 演算子アイテムのリスト
        std::vector<SpawnPoint> m_spawnPoints;

        // ★ バトルモードを保持
        BattleRuleMode m_ruleMode;

        // 新追加：ターン管理用の変数
        int m_totalTurns;       // ゲーム開始からの総経過ターン
        int m_currentCycleTick; // 現在の再生成までのカウント

        // ★ モード別の湧き間隔（クラシック: 3, ゼロワン: 2）
        int m_spawnInterval;

        // ★ モード別の初期化処理
        void InitializeSpawnPoints();
        void InitializeClassicSpawns();
        void InitializeZeroOneSpawns();

    public:
        explicit MapGrid(int tileSize = 80, Vector2 offset = Vector2(100, 100));
        ~MapGrid() = default;

        // ★ バトルモードを設定するメソッド
        void SetRuleMode(BattleRuleMode mode);

        // 新追加：ターン数のゲッター
        int GetTurnsUntilNextSpawn() const { return m_spawnInterval - m_currentCycleTick; }
        int GetTotalTurns() const { return m_totalTurns; }

        IntVector2 ScreenToGrid(const Vector2& pos) const;
        Vector2 GetCellCenter(int x, int y) const;
        bool IsWithinBounds(int x, int y) const;

        // トリポイント3：ターン経過時の処理（毎ターンでアイテムをサイクル更新）
        void UpdateTurn();

        // トリポイント4：指定マスのアイテムを拾う処理（拾うとマップから消える）
        char PickUpItem(int x, int y);

        // 指定マスにアイテムがあるか確認する用（UI表示やAIの判定用）
        char GetItemAt(int x, int y) const;

        void Draw() const;

        int GetWidth() const { return WIDTH; }
        int GetHeight() const { return HEIGHT; }
    };

} // namespace App