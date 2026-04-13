#include "SceneManager.h"

#include "GameScene.h"
#include "TitleScene.h"
#include "ResultScene.h"

#include <utility>

namespace App {

    // 静的インスタンス定義
    SceneManager* SceneManager::instance_ = nullptr;

    SceneManager::SceneManager()
        : scene_(nullptr),
        load_(nullptr),
        fader_(nullptr),
        sceneId_(SCENE_ID::NONE),
        nextSceneId_(SCENE_ID::NONE),
        isChanging_(false),
        isGameEnd_(false)
    {
    }

    SceneManager::~SceneManager()
    {
        // 安全に解放
        if (scene_) {
            scene_->Release();
            delete scene_;
            scene_ = nullptr;
        }
        if (load_) {
            delete load_;
            load_ = nullptr;
        }
        if (fader_) {
            delete fader_;
            fader_ = nullptr;
        }
    }

    void SceneManager::Init()
    {
        // 初期化処理（必要に応じて拡張）
        isGameEnd_ = false;
        isChanging_ = false;
        sceneId_ = SCENE_ID::NONE;
        nextSceneId_ = SCENE_ID::NONE;

		// 最初のシーンをタイトルに設定
		ChangeScene(SCENE_ID::TITLE);



    }

    void SceneManager::Init3D()
    {
        // 3D 初期化が必要ならここに記述
    }

    void SceneManager::Update()
    {
        // シーン遷移予約があれば実行
        if (isChanging_) {
            PerformSceneChange();
        }

        // 現在シーンの更新
        if (scene_) {
            scene_->Update();
        }
    }

    void SceneManager::Draw()
    {
        // 現在シーンの描画
        if (scene_) {
            scene_->Draw();
        }
    }

    void SceneManager::Delete()
    {
        // 現在シーンの解放
        if (scene_) {
            scene_->Release();
            delete scene_;
            scene_ = nullptr;
        }

        // 補助オブジェクトの解放
        if (load_) {
            delete load_;
            load_ = nullptr;
        }
        if (fader_) {
            delete fader_;
            fader_ = nullptr;
        }

        sceneId_ = SCENE_ID::NONE;
        nextSceneId_ = SCENE_ID::NONE;
        isChanging_ = false;
    }

    void SceneManager::ChangeScene(SCENE_ID nextId)
    {
        // 重複遷移の予約は上書きする設計
        nextSceneId_ = nextId;
        isChanging_ = true;
    }

    void SceneManager::PerformSceneChange()
    {
        // 現在のシーンを安全に解放
        if (scene_) {
            scene_->Release();
            delete scene_;
            scene_ = nullptr;
        }

        // 新しいシーンを生成（必要なシーンはここで追加）
        switch (nextSceneId_) {
        case SCENE_ID::NONE:
            scene_ = nullptr;
            break;
        case SCENE_ID::TITLE:
            // TitleScene 実装があればここで生成
            scene_ = new TitleScene();
            break;
        case SCENE_ID::GAME:
            // GameScene が実装済みであれば生成
            scene_ = new GameScene();
            break;
        case SCENE_ID::RESULT:
            scene_ = new ResultScene();
            break;
        default:
            scene_ = nullptr;
            break;
        }

        // シーンID とフラグ更新
        sceneId_ = nextSceneId_;
        nextSceneId_ = SCENE_ID::NONE;
        isChanging_ = false;

        // 新しいシーンのライフサイクルを呼ぶ
        if (scene_) {
            scene_->Init();
            scene_->Load();
            scene_->LoadEnd();
        }
    }

} // namespace App