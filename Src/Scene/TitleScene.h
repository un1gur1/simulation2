#pragma once
#include "SceneBase.h"

namespace App {

    class TitleScene : public SceneBase {
    public:
        enum class MenuState {
            SELECT_PLAYERS,
            SELECT_MODE,
            SELECT_SCORE,
            SELECT_P1_TYPE,   // ★追加：1PがプレイヤーかCOMか
            SELECT_P2_TYPE,   // ★追加：2PがプレイヤーかCOMか
            CUSTOM_P1_START,  // ★追加：1Pの初期数字と位置
            CUSTOM_P2_START   // ★追加：2Pの初期数字と位置
        };

    private:
        int m_frameCount;
        MenuState m_state;
        int m_playerCursor;
        int m_modeCursor;
        int m_scoreCursor;

        // ★追加：カスタム設定用の変数
        int m_p1TypeCursor;   // 0:PLAYER, 1:COM
        int m_p2TypeCursor;   // 0:PLAYER, 1:COM

        int m_p1CustomCursor; // 0:数値, 1:X座標, 2:Y座標
        int m_p2CustomCursor;
        int m_p1StartNum, m_p1StartX, m_p1StartY;
        int m_p2StartNum, m_p2StartX, m_p2StartY;

        int m_prevSpace;
        int m_prevUp;
        int m_prevDown;
        int m_prevRight;
        int m_prevLeft;
        int m_prevB;

    public:
        TitleScene();
        ~TitleScene() override;

        void Init() override;
        void Load() override;
        void LoadEnd() override;
        void Update() override;
        void Draw() override;
        void Release() override;
    };

} // namespace App