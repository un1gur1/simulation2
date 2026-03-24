#pragma once
#include <DxLib.h>
#include <unordered_map>
#include <vector>
#include "../Common/Vector2.h"

namespace App {

    class InputManager {
    public:
        enum class Action { Decide, Cancel, Menu, Up, Down, Left, Right };

        static InputManager& GetInstance();

        void Init();
        void Update();

        // 判定用
        void Add(int key);
        bool IsTrgDown(int key) const;
        bool IsActionTrg(Action action) const;

        // マウス情報
        Vector2 GetMousePos() const { return mousePos_; }
        bool IsMouseLeftTrg() const;
        bool IsMouseLeftDoubleClick() const;

        // パッド情報
        VECTOR GetDirectionXZ(int padNo);

    private:
        InputManager();
        ~InputManager();
        InputManager(const InputManager&) = delete;
        InputManager& operator=(const InputManager&) = delete;

        struct Info {
            bool keyOld = false;
            bool keyNew = false;
            bool keyTrgDown = false;
        };

        std::unordered_map<int, Info> keyInfos_;
        std::unordered_map<int, Info> mouseInfos_;
        std::unordered_map<Action, int> actionMap_;

        Vector2 mousePos_;
        int lastLeftClickTime_;
        bool isLeftDoubleClick_;

        static constexpr int DOUBLE_CLICK_TIME = 300;
    };

} // namespace App