#pragma once
#include <DxLib.h>
#include <unordered_map>
#include <vector>
#include "../Common/Vector2.h"

namespace App {

    // ==========================================
    // InputManager: 入力管理システム（シングルトン）
    // 用途: キーボード・マウスの入力状態を一元管理
    // 機能: トリガー判定、マウス座標取得、ダブルクリック検出
    // ==========================================
    class InputManager {
    public:
        // ==========================================
        // Action列挙型: ゲーム内の抽象的な操作
        // キー配置が変わっても、この名前で統一的に扱える
        // ==========================================
        enum class Action {
            Decide,   // 決定
            Cancel,   // キャンセル
            Menu,     // メニュー
            Up,       // 上移動
            Down,     // 下移動
            Left,     // 左移動
            Right     // 右移動
        };

        // ==========================================
        // シングルトンインスタンス取得
        // ゲーム内のどこからでもアクセス可能
        // ==========================================
        static InputManager& GetInstance();

        // ==========================================
        // 初期化・更新
        // ==========================================
        void Init();      // 初期化（ゲーム開始時に1回）
        void Update();    // 入力状態更新（毎フレーム）

        // ==========================================
        // キー判定
        // ==========================================
        void Add(int key);                       // 監視するキーを追加
        bool IsTrgDown(int key) const;           // キーが押された瞬間か判定
        bool IsActionTrg(Action action) const;   // アクションが実行されたか判定

        // ==========================================
        // マウス情報
        // ==========================================
        Vector2 GetMousePos() const { return mousePos_; }  // マウス座標取得
        bool IsMouseLeftTrg() const;                       // 左クリックされた瞬間か判定
        bool IsMouseLeftDoubleClick() const;               // ダブルクリックされたか判定

        // ==========================================
        // パッド情報（将来的な拡張用）
        // ==========================================
        VECTOR GetDirectionXZ(int padNo);  // パッドの方向入力取得

    private:
        // ==========================================
        // シングルトンパターン: コピー禁止
        // ==========================================
        InputManager();
        ~InputManager();
        InputManager(const InputManager&) = delete;
        InputManager& operator=(const InputManager&) = delete;

        // ==========================================
        // Info構造体: キー/ボタンの状態管理
        // ==========================================
        struct Info {
            bool keyOld = false;      // 前フレームの状態
            bool keyNew = false;      // 現フレームの状態
            bool keyTrgDown = false;  // トリガー判定（押された瞬間）
        };

        // ==========================================
        // 定数
        // ==========================================
        static constexpr int DOUBLE_CLICK_TIME = 300;  // ダブルクリック判定時間（ミリ秒）

        // ==========================================
        // メンバ変数: 入力状態の保存
        // ==========================================
        std::unordered_map<int, Info> keyInfos_;      // キーボード状態
        std::unordered_map<int, Info> mouseInfos_;    // マウスボタン状態
        std::unordered_map<Action, int> actionMap_;   // アクション→キーコードのマッピング

        Vector2 mousePos_;           // マウスカーソル座標
        int lastLeftClickTime_;      // 最後に左クリックされた時刻
        bool isLeftDoubleClick_;     // ダブルクリック検出フラグ
    };

} // namespace App