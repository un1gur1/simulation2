#pragma once
#include "SceneBase.h"

namespace App {

    class TitleScene : public SceneBase {
    public:
        enum class MenuState {
            SELECT_PLAYERS,
            SELECT_MODE,
            SELECT_SCORE // ★追加：スコア選択画面
        };

    private:
        int m_frameCount;
        MenuState m_state;
        int m_playerCursor;
        int m_modeCursor;
        int m_scoreCursor;  // ★追加：(0: 301, 1: 501, 2: 701)

        int m_prevSpace;
        int m_prevUp;
        int m_prevDown;
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