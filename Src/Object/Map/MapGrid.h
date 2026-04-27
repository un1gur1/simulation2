#pragma once

#include <DxLib.h>
#include <vector>
#include "../../Common/Vector2.h"

namespace App {

    // ==========================================
    // BattleRuleMode: バトルルールの種類
    // ==========================================
    enum class BattleRuleMode {
        CLASSIC,    // ノーマルモード（残機制バトル）
        ZERO_ONE    // カウントモード（スコア制バトル）
    };

    // ==========================================
    // SpawnPoint: 演算子アイテムの出現地点
    // 用途: マップ上の特定マスに演算子(+,-,*,/)を定期的に出現させる
    // ==========================================
    struct SpawnPoint {
        IntVector2 pos;              // 出現位置（グリッド座標）
        std::vector<char> sequence;  // 出現順序（例: {'+', '-', '*', '/'}）
        int currentIndex;            // 現在の出現インデックス
        char currentSymbol;          // 現在表示中の演算子
        bool isAvailable;            // アイテムが配置されているか
    };

    // ==========================================
    // MapGrid: バトルマップの管理クラス
    // 用途: グリッド座標管理、アイテム配置、ターン更新
    // ==========================================
    class MapGrid {
    public:
        // ==========================================
        // コンストラクタ・デストラクタ
        // ==========================================
        explicit MapGrid(int tileSize = 80, Vector2 offset = Vector2(100, 100));
        ~MapGrid() = default;

        // ==========================================
        // 初期化・設定
        // ==========================================
        void SetRuleModeAndStage(BattleRuleMode mode, int stageIndex);

        // ==========================================
        // 座標変換
        // ==========================================
        IntVector2 ScreenToGrid(const Vector2& pos) const;     // 画面座標→グリッド座標
        Vector2 GetCellCenter(int x, int y) const;             // マス中心の画面座標取得
        bool IsWithinBounds(int x, int y) const;               // 範囲内判定

        // ==========================================
        // ターン管理
        // ==========================================
        void UpdateTurn();                                     // ターン経過処理（アイテム更新）
        int GetTurnsUntilNextSpawn() const { return m_spawnInterval - m_currentCycleTick; }  // 次の出現までのターン数
        int GetTotalTurns() const { return m_totalTurns; }     // 総ターン数

        // ==========================================
        // アイテム管理
        // ==========================================
        char PickUpItem(int x, int y);                         // アイテム取得（取得後消える）
        char GetItemAt(int x, int y) const;                    // アイテム確認（消えない）

        // ==========================================
        // 描画
        // ==========================================
        void Draw() const;

        // ==========================================
        // アクセサ
        // ==========================================
        int GetWidth() const { return WIDTH; }
        int GetHeight() const { return HEIGHT; }

    private:
        // ==========================================
        // 定数
        // ==========================================
        static constexpr int WIDTH = 9;      // マップ幅（9x9グリッド）
        static constexpr int HEIGHT = 9;     // マップ高さ

        // ==========================================
        // レイアウト設定
        // ==========================================
        int m_tileSize;         // マス1つのサイズ（ピクセル）
        Vector2 m_offset;       // 描画オフセット（画面左上からの位置）

        // ==========================================
        // 地形データ
        // ==========================================
        int m_terrain[WIDTH][HEIGHT];  // 地形配列（0:通常, 1:壁など）

        // ==========================================
        // アイテムシステム
        // ==========================================
        std::vector<SpawnPoint> m_spawnPoints;  // 出現地点のリスト

        // ==========================================
        // ルール設定
        // ==========================================
        BattleRuleMode m_ruleMode;   // 現在のバトルモード
        int m_stageIndex;            // ステージ番号（0=バランス, 1=マイナス, 2=カオス）
        int m_spawnInterval;         // アイテム出現間隔（クラシック:3, ゼロワン:2）

        // ==========================================
        // ターン管理
        // ==========================================
        int m_totalTurns;            // ゲーム開始からの総ターン数
        int m_currentCycleTick;      // 現在の出現サイクルカウント

        // ==========================================
        // 内部処理: 初期化
        // ==========================================
        void InitializeSpawnPoints();       // モードに応じた初期化
        void InitializeClassicSpawns();     // ノーマルモード用
        void InitializeZeroOneSpawns();     // カウントモード用
    };

} // namespace App