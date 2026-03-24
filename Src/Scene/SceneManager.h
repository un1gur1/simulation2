#pragma once

#include "SceneBase.h"

namespace App {

    class Loading;
    class Fader;
    class GameScene; 
	class TitleScene;

    class SceneManager
    {
    public:
        // シーン管理用
        enum class SCENE_ID
        {
            NONE,
            TITLE,
            GAME,
            RESULT_WIN,
            RESULT_LOSE,
        };

    public:
        // シングルトン（生成・取得・削除）
        static void CreateInstance() { if (instance_ == nullptr) { instance_ = new SceneManager(); } }
        static SceneManager* GetInstance() { return instance_; }
        static void DeleteInstance() { if (instance_ != nullptr) { delete instance_; instance_ = nullptr; } }

        // ライフサイクル
        void Init();        // 初期化
        void Init3D();      // 3D の初期化（必要なら実装）
        void Update();      // 更新
        void Draw();        // 描画
        void Delete();      // リソースの破棄

        // 状態遷移
        void ChangeScene(SCENE_ID nextId);

        // シーンIDの取得
        SCENE_ID GetSceneID() const { return sceneId_; }

        // ゲーム終了
        void GameEnd() { isGameEnd_ = true; }
        bool GetGameEnd() const { return isGameEnd_; }

    private:
        // 実際の切り替えロジック（内部）
        void PerformSceneChange();

        // シングルトン本体
        static SceneManager* instance_;

        // 各種メンバ（設計に合わせて末尾アンダースコア）
        SceneBase* scene_;
        Loading* load_;
        Fader* fader_;

        SCENE_ID sceneId_;
        SCENE_ID nextSceneId_;

        bool isChanging_;
        bool isGameEnd_;

        

    private:
        // 外部での生成を禁止
        SceneManager();
        ~SceneManager();

        // コピー・ムーブ禁止
        SceneManager(const SceneManager&) = delete;
        SceneManager& operator=(const SceneManager&) = delete;
        SceneManager(SceneManager&&) = delete;
        SceneManager& operator=(SceneManager&&) = delete;
    };

} // namespace App