#pragma once
#include "SceneBase.h"

namespace App {

    class TitleScene : public SceneBase {
    public:
        enum class MenuState {
            PRESS_START,
            SELECT_PLAYERS,
            SELECT_MODE,
            SELECT_SCORE,
            SELECT_P1_TYPE,
            SELECT_P2_TYPE,
            CUSTOM_P1_START,
            CUSTOM_P2_START
        };

    private:
        // ==========================================
        // ★ ゲームルールの定数
        // ==========================================
        static constexpr int GRID_SIZE = 9;

        static constexpr int DEF_P1_HP = 5;
        static constexpr int DEF_P1_X = 1;
        static constexpr int DEF_P1_Y = 1;

        static constexpr int DEF_P2_HP = 7;
        static constexpr int DEF_P2_X = 7;
        static constexpr int DEF_P2_Y = 7;

        // ==========================================
        // ★ プレイヤー設定構造体
        // ==========================================
        struct PlayerConfig {
            int typeCursor;   // 0:PLAYER, 1:NPC
            int customCursor; // 0:数値, 1:X座標, 2:Y座標, 3:決定
            int startNum;
            int startX;
            int startY;
        };

        int m_frameCount;
        MenuState m_state;
        int m_playerCursor;
        int m_modeCursor;
        int m_scoreCursor;

        // 1Pと2Pの設定を配列で一括管理 (インデックス0=1P, 1=2P)
        PlayerConfig m_players[2];

        bool m_shouldQuit;

        // フォントハンドル
        int m_fontTitle;
        int m_fontMenu;
        int m_fontSmall;
        int m_fontNumber;

        // ピクセルシェーダーハンドル
        int m_psHandle;
        int m_cbHandle;
        float m_shaderTime;
        



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