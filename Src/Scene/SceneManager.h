#pragma once
#include "SceneBase.h"

namespace App {

    class Loading;
    class Fader;
    class GameScene;
    class TitleScene;
    class ResultScene;
    class PauseMenu;

    // ★追加：戦績データをまとめる構造体
    struct BattleStats {
        int totalTurns;     // 経過ターン数
        int totalMoves;     // 総移動マス数
        int totalOpsUsed;   // 使用した演算子数
        int playTimeFrames; // プレイ時間（フレーム数）
        int maxDamage;      // 最大ダメージ（スコア）
    };

    class SceneManager
    {
    public:
        enum class SCENE_ID { NONE, TITLE, GAME, RESULT };

    public:
        static void CreateInstance() { if (instance_ == nullptr) { instance_ = new SceneManager(); } }
        static SceneManager* GetInstance() { return instance_; }
        static void DeleteInstance() { if (instance_ != nullptr) { delete instance_; instance_ = nullptr; } }

        void Init();
        void Init3D();
        void Update();
        void Draw();
        void Delete();
        void ChangeScene(SCENE_ID nextId);
        SCENE_ID GetSceneID() const { return sceneId_; }

        void GameEnd() { isGameEnd_ = true; }
        bool GetGameEnd() const { return isGameEnd_; }

        // --- 既存のゲーム設定 ---
        void SetGameSettings(int players, int mode, int zeroOneScore = 501) {
            playerCount_ = players;
            gameMode_ = mode;
            zeroOneScore_ = zeroOneScore;
        }
        int GetPlayerCount() const { return playerCount_; }
        int GetGameMode() const { return gameMode_; }
        int GetZeroOneScore() const { return zeroOneScore_; }

        void SetPlayer1Settings(bool isNPC, int startNum, int startX, int startY) {
            is1P_NPC_ = isNPC; p1StartNum_ = startNum; p1StartX_ = startX; p1StartY_ = startY;
        }
        void SetPlayer2Settings(bool isNPC, int startNum, int startX, int startY) {
            is2P_NPC_ = isNPC; p2StartNum_ = startNum; p2StartX_ = startX; p2StartY_ = startY;
        }

        bool Is1PNPC() const { return is1P_NPC_; }
        bool Is2PNPC() const { return is2P_NPC_; }
        int Get1PStartNum() const { return p1StartNum_; }
        int Get2PStartNum() const { return p2StartNum_; }
        int Get1PStartX() const { return p1StartX_; }
        int Get1PStartY() const { return p1StartY_; }
        int Get2PStartX() const { return p2StartX_; }
        int Get2PStartY() const { return p2StartY_; }

        // ==========================================
        // ★追加：戦績データのセッターとゲッター
        // ==========================================
        void SetBattleResult(bool isWin, const BattleStats& stats) {
            m_lastIsWin = isWin;
            m_lastStats = stats;
        }
        bool GetLastIsWin() const { return m_lastIsWin; }
        const BattleStats& GetLastStats() const { return m_lastStats; }

    private:
        void PerformSceneChange();
        static SceneManager* instance_;

        SceneBase* scene_;
        Loading* load_;
        Fader* fader_;
        PauseMenu* pauseMenu_;

        SCENE_ID sceneId_;
        SCENE_ID nextSceneId_;

        bool isChanging_;
        bool isGameEnd_;
        bool isPaused_;
        int playerCount_;
        int gameMode_;
        int zeroOneScore_;

        bool is1P_NPC_;
        bool is2P_NPC_;
        int  p1StartNum_;
        int  p2StartNum_;
        int  p1StartX_;
        int  p1StartY_;
        int  p2StartX_;
        int  p2StartY_;

        // ★追加：戦績保持用のメンバ変数
        bool m_lastIsWin = false;
        BattleStats m_lastStats = { 0, 0, 0, 0, 0 };

    private:
        SceneManager();
        ~SceneManager();
        SceneManager(const SceneManager&) = delete;
        SceneManager& operator=(const SceneManager&) = delete;
        SceneManager(SceneManager&&) = delete;
        SceneManager& operator=(SceneManager&&) = delete;
    };

} // namespace App