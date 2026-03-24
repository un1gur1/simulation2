#pragma once

namespace App {

    // シーン基底（純粋仮想クラス）
    // ヘッダに実装を置き、派生で必ずオーバーライドさせる設計
    class SceneBase {
    public:
        SceneBase() = default;

        // 抽象デストラクタだが定義を与えてリンク時の未定義参照を防ぐ
        virtual ~SceneBase() = 0;

        // ライフサイクル（派生で実装）
        virtual void Init() = 0;     // 初期化（リソースなどの準備前）
        virtual void Load() = 0;     // リソース読み込み
        virtual void LoadEnd() = 0;  // 読み込み完了後の初期化
        virtual void Update() = 0;   // 毎フレーム更新
        virtual void Draw() = 0;     // 毎フレーム描画
        virtual void Release() = 0;  // 解放処理
    };

    inline SceneBase::~SceneBase() {}
} // namespace App