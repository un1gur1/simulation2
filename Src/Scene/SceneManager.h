#pragma once

#include "SceneBase.h"

namespace App {

    class Loading;
    class Fader;
    class GameScene;
    class TitleScene;
    class ResultScene;

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

        // ★修正：スコア(501など)を受け取れるように引数を追加
        void SetGameSettings(int players, int mode, int zeroOneScore = 501) {
            playerCount_ = players;
            gameMode_ = mode;
            zeroOneScore_ = zeroOneScore; // ★追加
        }
        int GetPlayerCount() const { return playerCount_; }
        int GetGameMode() const { return gameMode_; }
        int GetZeroOneScore() const { return zeroOneScore_; } // ★追加

    private:
        void PerformSceneChange();
        static SceneManager* instance_;

        SceneBase* scene_;
        Loading* load_;
        Fader* fader_;

        SCENE_ID sceneId_;
        SCENE_ID nextSceneId_;

        bool isChanging_;
        bool isGameEnd_;

        int playerCount_;
        int gameMode_;
        int zeroOneScore_; // ★追加：ゼロワンの初期スコア

    private:
        SceneManager();
        ~SceneManager();
        SceneManager(const SceneManager&) = delete;
        SceneManager& operator=(const SceneManager&) = delete;
        SceneManager(SceneManager&&) = delete;
        SceneManager& operator=(SceneManager&&) = delete;
    };

} // namespace App