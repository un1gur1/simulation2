#pragma once
#include "SceneBase.h"
#include "SceneManager.h"

namespace App {

    // ==========================================
    // ResultScene: リザルト画面のシーン
    // 用途: バトル終了後の結果表示（勝敗・戦績）
    // 継承: SceneBaseのライフサイクルを実装
    // ==========================================
    class ResultScene : public SceneBase {
    public:
        // ==========================================
        // コンストラクタ・デストラクタ
        // ==========================================
        ResultScene();
        virtual ~ResultScene();

        // ==========================================
        // シーンライフサイクル
        // SceneManager経由で呼ばれる
        // ==========================================
        virtual void Init() override;       // 初期化（結果データを受け取る）
        virtual void Load() override;       // リソース読み込み
        virtual void LoadEnd() override;    // 読み込み完了処理
        virtual void Update() override;     // 更新（入力受付）
        virtual void Draw() override;       // 描画（結果表示）
        virtual void Release() override;    // 解放

    private:
        // ==========================================
        // 演出用
        // ==========================================
        int m_frameCount;       // フレームカウンター（アニメーション用）
        int m_psHandle;         // ピクセルシェーダーハンドル（背景エフェクト）
        int m_cbHandle;         // 定数バッファハンドル（シェーダーパラメータ）

        // ==========================================
        // バトル結果データ
        // SceneManagerから受け取る
        // ==========================================
        bool m_isWin;           // 勝利したか（true=勝利, false=敗北）
        BattleStats m_stats;    // 戦績データ（総ターン数・最大ダメージなど）
    };

} // namespace App