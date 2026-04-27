#pragma once

#include "SceneBase.h"
#include "../Manager/BattleMaster.h"
#include "../Common/Vector2.h"

namespace App {

    // ==========================================
    // GameScene: ゲーム本編のシーン
    // 用途: ターン制バトルの実行・管理
    // 継承: SceneBaseのライフサイクルを実装
    // ==========================================
    class GameScene : public SceneBase {
    public:
        // ==========================================
        // コンストラクタ・デストラクタ
        // ==========================================
        GameScene();
        ~GameScene() override;

        // ==========================================
        // シーンライフサイクル
        // SceneManager経由で呼ばれる
        // ==========================================
        void Init() override;       // 初期化（シーン開始時に1回）
        void Load() override;       // リソース読み込み
        void LoadEnd() override;    // 読み込み完了処理
        void Update() override;     // 更新（毎フレーム）
        void Draw() override;       // 描画（毎フレーム）
        void Release() override;    // 解放（シーン終了時に1回）

    private:
        // ==========================================
        // 定数
        // ==========================================
        static constexpr int kTileSize = 64;  // タイルサイズ（ピクセル）

        // ==========================================
        // バトルシステム
        // ==========================================
        BattleMaster m_battleMaster;  // バトル全体の管理（実体として保持）

        // ==========================================
        // デバッグ・補助情報
        // ==========================================
        int  m_frameCount;   // フレームカウンター
        bool m_showInfo;     // デバッグ情報の表示フラグ
        int  m_prevMouse;    // 前フレームのマウス状態（エッジ検出用）

     
    };

} // namespace App