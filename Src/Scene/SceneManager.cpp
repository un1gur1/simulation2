#include "SceneManager.h"
#include "GameScene.h"
#include "TitleScene.h"
#include "ResultScene.h"
#include <utility>

namespace App {
    SceneManager* SceneManager::instance_ = nullptr;

    SceneManager::SceneManager()
        : scene_(nullptr), load_(nullptr), fader_(nullptr)
        , sceneId_(SCENE_ID::NONE), nextSceneId_(SCENE_ID::NONE)
        , isChanging_(false), isGameEnd_(false)
        , playerCount_(1), gameMode_(0), zeroOneScore_(501)
        // ★追加：変数の初期化（デフォルトは1P人間・2P敵のいつもの設定）
        , is1P_NPC_(false), is2P_NPC_(true)
        , p1StartNum_(5), p2StartNum_(7)
        , p1StartX_(1), p1StartY_(1)
        , p2StartX_(7), p2StartY_(7)
    {
    }

    SceneManager::~SceneManager() {
        if (scene_) { scene_->Release(); delete scene_; scene_ = nullptr; }
        if (load_) { delete load_; load_ = nullptr; }
        if (fader_) { delete fader_; fader_ = nullptr; }
    }

    void SceneManager::Init() {
        isGameEnd_ = false;
        isChanging_ = false;
        sceneId_ = SCENE_ID::NONE;
        nextSceneId_ = SCENE_ID::NONE;

        playerCount_ = 1;
        gameMode_ = 0;
        zeroOneScore_ = 501;

        // ★追加：タイトルに戻った際に初期値にリセット
        is1P_NPC_ = false;
        is2P_NPC_ = true;
        p1StartNum_ = 5;
        p2StartNum_ = 7;
        p1StartX_ = 1;
        p1StartY_ = 1;
        p2StartX_ = 7;
        p2StartY_ = 7;

        ChangeScene(SCENE_ID::TITLE);
    }

    void SceneManager::Init3D() {}

    void SceneManager::Update() {
        if (isChanging_) PerformSceneChange();
        if (scene_) scene_->Update();
    }

    void SceneManager::Draw() {
        if (scene_) scene_->Draw();
    }

    void SceneManager::Delete() {
        if (scene_) { scene_->Release(); delete scene_; scene_ = nullptr; }
        if (load_) { delete load_; load_ = nullptr; }
        if (fader_) { delete fader_; fader_ = nullptr; }
        sceneId_ = SCENE_ID::NONE;
        nextSceneId_ = SCENE_ID::NONE;
        isChanging_ = false;
    }

    void SceneManager::ChangeScene(SCENE_ID nextId) {
        nextSceneId_ = nextId;
        isChanging_ = true;
    }

    void SceneManager::PerformSceneChange() {
        if (scene_) { scene_->Release(); delete scene_; scene_ = nullptr; }

        switch (nextSceneId_) {
        case SCENE_ID::NONE:   scene_ = nullptr; break;
        case SCENE_ID::TITLE:  scene_ = new TitleScene(); break;
        case SCENE_ID::GAME:   scene_ = new GameScene(); break;
        case SCENE_ID::RESULT: scene_ = new ResultScene(); break;
        default:               scene_ = nullptr; break;
        }

        sceneId_ = nextSceneId_;
        nextSceneId_ = SCENE_ID::NONE;
        isChanging_ = false;

        if (scene_) {
            scene_->Init();
            scene_->Load();
            scene_->LoadEnd();
        }
    }
} // namespace App