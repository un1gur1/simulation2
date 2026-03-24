#include "InputManager.h"

namespace App {

    InputManager::InputManager()
        : lastLeftClickTime_(0)
        , isLeftDoubleClick_(false)
    {
    }

    InputManager::~InputManager() {}

    InputManager& InputManager::GetInstance() {
        static InputManager instance;
        return instance;
    }

    void InputManager::Init() {
        // デフォルトのアクション割り当て
        actionMap_[Action::Decide] = KEY_INPUT_RETURN;
        actionMap_[Action::Cancel] = KEY_INPUT_ESCAPE;
        actionMap_[Action::Up] = KEY_INPUT_UP;
        actionMap_[Action::Down] = KEY_INPUT_DOWN;
        actionMap_[Action::Left] = KEY_INPUT_LEFT;
        actionMap_[Action::Right] = KEY_INPUT_RIGHT;

        // 使用するキーをあらかじめ登録
        Add(KEY_INPUT_RETURN);
        Add(KEY_INPUT_ESCAPE);
        Add(KEY_INPUT_UP);
        Add(KEY_INPUT_DOWN);
        Add(KEY_INPUT_LEFT);
        Add(KEY_INPUT_RIGHT);
    }

    void InputManager::Add(int key) {
        if (keyInfos_.find(key) == keyInfos_.end()) {
            keyInfos_[key] = Info();
        }
    }

    void InputManager::Update() {
        // --- キー入力更新 ---
        char keyState[256];
        GetHitKeyStateAll(keyState);

        for (auto& pair : keyInfos_) {
            Info& info = pair.second;
            info.keyOld = info.keyNew;
            info.keyNew = (keyState[pair.first] == 1);
            info.keyTrgDown = (info.keyNew && !info.keyOld);
        }

        // --- マウス入力更新 ---
        int mx, my;
        GetMousePoint(&mx, &my);
        mousePos_ = Vector2((float)mx, (float)my);

        int mouseInput = GetMouseInput();
        Info& mLeft = mouseInfos_[MOUSE_INPUT_LEFT];
        mLeft.keyOld = mLeft.keyNew;
        mLeft.keyNew = (mouseInput & MOUSE_INPUT_LEFT);
        mLeft.keyTrgDown = (mLeft.keyNew && !mLeft.keyOld);

        // --- ダブルクリック判定 ---
        isLeftDoubleClick_ = false;
        if (mLeft.keyTrgDown) {
            int now = GetNowCount();
            if (now - lastLeftClickTime_ < DOUBLE_CLICK_TIME) {
                isLeftDoubleClick_ = true;
                lastLeftClickTime_ = 0; // 3連打防止
            }
            else {
                lastLeftClickTime_ = now;
            }
        }
    }

    bool InputManager::IsTrgDown(int key) const {
        auto it = keyInfos_.find(key);
        return (it != keyInfos_.end()) ? it->second.keyTrgDown : false;
    }

    bool InputManager::IsActionTrg(Action action) const {
        auto it = actionMap_.find(action);
        if (it != actionMap_.end()) {
            return IsTrgDown(it->second);
        }
        return false;
    }

    bool InputManager::IsMouseLeftTrg() const {
        auto it = mouseInfos_.find(MOUSE_INPUT_LEFT);
        return (it != mouseInfos_.end()) ? it->second.keyTrgDown : false;
    }

    bool InputManager::IsMouseLeftDoubleClick() const {
        return isLeftDoubleClick_;
    }

    // パッド情報の簡易実装
    VECTOR InputManager::GetDirectionXZ(int padNo) {
        DINPUT_JOYSTATE joy;
        GetJoypadDirectInputState(padNo, &joy);
        return VGet((float)joy.X / 1000.0f, 0, (float)joy.Y / -1000.0f);
    }

} // namespace App