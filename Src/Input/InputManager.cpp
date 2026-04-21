#include "InputManager.h"

namespace App {

    // ==========================================
    // コンストラクタ: ダブルクリック判定用の変数を初期化
    // ==========================================
    InputManager::InputManager()
        : lastLeftClickTime_(0)        // 最後にクリックした時刻
        , isLeftDoubleClick_(false)    // ダブルクリックフラグ
    {
    }

    // ==========================================
    // デストラクタ
    // ==========================================
    InputManager::~InputManager() {}

    // ==========================================
    // シングルトンインスタンス取得
    // 用途: ゲーム全体で1つだけの入力管理オブジェクトを共有
    // ==========================================
    InputManager& InputManager::GetInstance() {
        static InputManager instance;
        return instance;
    }

    // ==========================================
    // 初期化: キー割り当てと監視対象キーの登録
    // ==========================================
    void InputManager::Init() {
        // アクション名とキーコードの対応付け
        // これにより「決定」などの抽象的な操作名でキー入力を扱える
        actionMap_[Action::Decide] = KEY_INPUT_RETURN;   // 決定キー: Enter
        actionMap_[Action::Cancel] = KEY_INPUT_ESCAPE;   // キャンセルキー: ESC
        actionMap_[Action::Up] = KEY_INPUT_UP;           // 上移動: ↑
        actionMap_[Action::Down] = KEY_INPUT_DOWN;       // 下移動: ↓
        actionMap_[Action::Left] = KEY_INPUT_LEFT;       // 左移動: ←
        actionMap_[Action::Right] = KEY_INPUT_RIGHT;     // 右移動: →

        // 監視対象のキーを登録（登録しないキーは検出されない）
        Add(KEY_INPUT_RETURN);
        Add(KEY_INPUT_ESCAPE);
        Add(KEY_INPUT_UP);
        Add(KEY_INPUT_DOWN);
        Add(KEY_INPUT_LEFT);
        Add(KEY_INPUT_RIGHT);

        Add(KEY_INPUT_W);       // W: 上移動（WASD操作用）
        Add(KEY_INPUT_S);       // S: 下移動（WASD操作用）
        Add(KEY_INPUT_SPACE);   // スペース: ジャンプ・決定など
    }

    // ==========================================
    // キー登録: 監視対象にキーを追加
    // 引数: key - DXライブラリのキーコード（KEY_INPUT_*）
    // ==========================================
    void InputManager::Add(int key) {
        // 既に登録済みでなければ新規登録
        if (keyInfos_.find(key) == keyInfos_.end()) {
            keyInfos_[key] = Info();  // 入力状態管理用の構造体を作成
        }
    }

    // ==========================================
    // 毎フレーム更新: キー・マウス入力の状態を検出
    // ==========================================
    void InputManager::Update() {
        // --- キーボード入力の更新 ---
        char keyState[256];  // 全キーの押下状態を格納
        GetHitKeyStateAll(keyState);  // DXライブラリから取得

        // 登録済みキーの状態を更新
        for (auto& pair : keyInfos_) {
            Info& info = pair.second;

            // 前フレームの状態を保存
            info.keyOld = info.keyNew;

            // 現在フレームの状態を取得
            info.keyNew = (keyState[pair.first] == 1);

            // トリガー判定: 「前フレーム=離れてた」かつ「今フレーム=押された」
            // 用途: ボタン押した瞬間だけ反応させたい処理（メニュー選択など）
            info.keyTrgDown = (info.keyNew && !info.keyOld);
        }

        // --- マウス入力の更新 ---

        // マウスカーソルの座標取得
        int mx, my;
        GetMousePoint(&mx, &my);
        mousePos_ = Vector2((float)mx, (float)my);

        // マウスボタンの状態取得
        int mouseInput = GetMouseInput();
        Info& mLeft = mouseInfos_[MOUSE_INPUT_LEFT];

        mLeft.keyOld = mLeft.keyNew;
        mLeft.keyNew = (mouseInput & MOUSE_INPUT_LEFT);  // ビット演算で左ボタン判定
        mLeft.keyTrgDown = (mLeft.keyNew && !mLeft.keyOld);

        // --- ダブルクリック判定 ---
        // 仕組み: 2回のクリックの間隔が一定時間以内ならダブルクリック
        isLeftDoubleClick_ = false;

        if (mLeft.keyTrgDown) {  // 左ボタンが押された瞬間
            int now = GetNowCount();  // 現在時刻（ミリ秒）

            // 前回クリックから規定時間以内ならダブルクリック判定
            if (now - lastLeftClickTime_ < DOUBLE_CLICK_TIME) {
                isLeftDoubleClick_ = true;
                lastLeftClickTime_ = 0;  // 3連打を防ぐため時刻をリセット
            }
            else {
                lastLeftClickTime_ = now;  // 今回のクリック時刻を記録
            }
        }
    }

    // ==========================================
    // キートリガー判定: 指定キーが「押された瞬間」か
    // 引数: key - 判定するキーコード
    // 戻り値: true=押された瞬間 / false=それ以外
    // ==========================================
    bool InputManager::IsTrgDown(int key) const {
        auto it = keyInfos_.find(key);
        return (it != keyInfos_.end()) ? it->second.keyTrgDown : false;
    }

    // ==========================================
    // アクショントリガー判定: 抽象的な操作名で判定
    // 引数: action - Action列挙型（Decide, Cancel など）
    // 戻り値: true=対応するキーが押された瞬間
    // 用途: キー設定変更に対応しやすい設計
    // ==========================================
    bool InputManager::IsActionTrg(Action action) const {
        auto it = actionMap_.find(action);
        if (it != actionMap_.end()) {
            return IsTrgDown(it->second);  // 割り当てられたキーで判定
        }
        return false;
    }

    // ==========================================
    // マウス左ボタントリガー判定
    // 戻り値: true=左ボタンが押された瞬間
    // 用途: UI要素のクリック検出など
    // ==========================================
    bool InputManager::IsMouseLeftTrg() const {
        auto it = mouseInfos_.find(MOUSE_INPUT_LEFT);
        return (it != mouseInfos_.end()) ? it->second.keyTrgDown : false;
    }

    // ==========================================
    // ダブルクリック判定
    // 戻り値: true=今フレームでダブルクリックが成立した
    // 用途: ファイル開く、全選択など
    // ==========================================
    bool InputManager::IsMouseLeftDoubleClick() const {
        return isLeftDoubleClick_;
    }

    // ==========================================
    // ゲームパッドの方向入力取得（簡易実装）
    // 引数: padNo - パッド番号（0=1P, 1=2P...）
    // 戻り値: XZ平面での方向ベクトル（X=左右, Z=前後）
    // 用途: 3Dゲームでのキャラクター移動など
    // ==========================================
    VECTOR InputManager::GetDirectionXZ(int padNo) {
        DINPUT_JOYSTATE joy;
        GetJoypadDirectInputState(padNo, &joy);

        // スティック入力を正規化（-1000～1000 → -1.0～1.0）
        // Y軸は反転（スティック上=負、ゲーム内前進=正）
        return VGet((float)joy.X / 1000.0f, 0, (float)joy.Y / -1000.0f);
    }

} // namespace App