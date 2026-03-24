#pragma once

#include "SceneBase.h"         // 親クラスのヘッダー
#include "../Manager/BattleMaster.h" // バトルマスターを連れてくるにょ
#include "../Common/Vector2.h"    // 座標計算用

namespace App {

    class GameScene : public SceneBase {
    private:
        // --- 定数 ---
        static constexpr int kTileSize = 64; // タイルサイズ（必要ならここで定義）

        // --- メンバ変数 ---

        // 【重要】ポインタを使わず、実体として持つにょ。
        // これで delete 忘れの心配なし！
        BattleMaster m_battleMaster;

        int  m_frameCount; // フレーム数カウント
        bool m_showInfo;   // デバッグ情報の表示フラグ
        int  m_prevMouse;  // 前フレームのマウス入力状態（エッジ検出用）

    public:
        GameScene();
        ~GameScene() override;

        // シーンの基本サイクル
        void Init() override;
        void Load() override;
        void LoadEnd() override;
        void Update() override;
        void Draw() override;
        void Release() override;

    private:
        // もしシーン特有の補助関数が必要ならここに書くにょ
    };

} // namespace App