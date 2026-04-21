#pragma once
#include "SceneBase.h"

namespace App {

    // ==========================================
    // TitleScene: タイトル画面のシーン
    // 用途: ゲーム設定（モード・プレイヤー・ステージ選択）
    // 継承: SceneBaseのライフサイクルを実装
    // ==========================================
    class TitleScene : public SceneBase {
    public:
        // ==========================================
        // MenuState: メニュー階層の状態
        // タイトル画面の多段階メニューを管理
        // ==========================================
        enum class MenuState {
            PRESS_START,            // [1] スタート画面
            SELECT_PLAYERS,         // [2] プレイヤー数選択（シングル/2P）
            SELECT_MODE,            // [3] モード選択（クラシック/ゼロワン）
            SELECT_CLASSIC_STOCKS,  // [4a] 残機設定（クラシック専用）
            SELECT_SCORE,           // [4b] 目標スコア設定（ゼロワン専用）
            SELECT_P1_TYPE,         // [5] 1P操作設定（プレイヤー/NPC）
            SELECT_P2_TYPE,         // [6] 2P操作設定（プレイヤー/NPC）
            SELECT_STAGE,           // [7] ステージ選択
            CUSTOM_P1_START,        // [8] 1Pカスタム設定（体力・座標）
            CUSTOM_P2_START         // [9] 2Pカスタム設定→ゲーム開始
        };

        // ==========================================
        // コンストラクタ・デストラクタ
        // ==========================================
        TitleScene();
        ~TitleScene() override;

        // ==========================================
        // シーンライフサイクル
        // ==========================================
        void Init() override;       // 初期化
        void Load() override;       // リソース読み込み
        void LoadEnd() override;    // 読み込み完了処理
        void Update() override;     // 更新（入力受付・メニュー遷移）
        void Draw() override;       // 描画（背景・メニュー・プレビュー）
        void Release() override;    // 解放

    private:
        // ==========================================
        // 定数: ゲームルールのデフォルト値
        // ==========================================
        static constexpr int GRID_SIZE = 9;      // グリッドサイズ（9x9）

        static constexpr int DEF_P1_HP = 5;      // 1P初期体力
        static constexpr int DEF_P1_X = 3;       // 1P初期X座標
        static constexpr int DEF_P1_Y = 3;       // 1P初期Y座標

        static constexpr int DEF_P2_HP = 5;      // 2P初期体力
        static constexpr int DEF_P2_X = 7;       // 2P初期X座標
        static constexpr int DEF_P2_Y = 7;       // 2P初期Y座標

        // ==========================================
        // PlayerConfig: プレイヤー設定構造体
        // 1P・2Pの設定をまとめて管理
        // ==========================================
        struct PlayerConfig {
            int typeCursor;      // 操作タイプ（0=プレイヤー, 1=NPC）
            int customCursor;    // カスタム設定カーソル（0=体力, 1=X, 2=Y, 3=決定）
            int startNum;        // 初期体力
            int startX;          // 初期X座標（1～9）
            int startY;          // 初期Y座標（1～9）
        };

        // ==========================================
        // メニュー状態
        // ==========================================
        MenuState m_state;           // 現在のメニュー状態
        int m_frameCount;            // フレームカウンター

        // ==========================================
        // カーソル位置（各メニュー画面）
        // ==========================================
        int m_playerCursor;          // プレイヤー数カーソル（1=シングル, 2=2P）
        int m_modeCursor;            // モードカーソル（0=クラシック, 1=ゼロワン）
        int m_stocksCursor;          // 残機カーソル（0,1,2）
        int m_scoreCursor;           // スコアカーソル（0,1,2）
        int m_stageCursor;           // ステージカーソル（0,1,2）

        // ==========================================
        // プレイヤー設定
        // ==========================================
        PlayerConfig m_players[2];   // [0]=1P, [1]=2P

        // ==========================================
        // 終了フラグ
        // ==========================================
        bool m_shouldQuit;           // ESCキーでゲーム終了

        // ==========================================
        // フォントハンドル
        // ==========================================
        int m_fontTitle;             // タイトルロゴ用
        int m_fontMenu;              // メニュー項目用
        int m_fontSmall;             // 小さい文字用
        int m_fontNumber;            // 数値表示用

        // ==========================================
        // シェーダーハンドル（背景エフェクト）
        // ==========================================
        int m_psHandle;              // ピクセルシェーダー
        int m_cbHandle;              // 定数バッファ
        float m_shaderTime;          // シェーダー時間（アニメーション用）
    };

} // namespace App