#include "TitleScene.h"
#include <DxLib.h>
#include "SceneManager.h"

namespace App {

    TitleScene::TitleScene()
        : m_frameCount(0)
    {
    }

    TitleScene::~TitleScene() {
    }

    void TitleScene::Init() {
        m_frameCount = 0;
    }

    void TitleScene::Load() {
        // 必要ならリソースをロード
    }

    void TitleScene::LoadEnd() {
    }

    void TitleScene::Update() {
        ++m_frameCount;

        if (m_frameCount < 30) return;
        // スペースキーでゲーム開始（シーン遷移処理は管理側で実装）
        if (CheckHitKey(KEY_INPUT_SPACE)) {
            // ここでシーン遷移要求を出す

			SceneManager::GetInstance()->ChangeScene(SceneManager::SCENE_ID::GAME);
        }
    }

    void TitleScene::Draw() {
        DrawFormatString(200, 200, GetColor(255, 255, 255), "タイトルシーン");
        DrawFormatString(200, 240, GetColor(255, 255, 0), "スペースキーでゲーム開始");
    }

    void TitleScene::Release() {
        // 特別な解放処理が必要ならここに
    }

} // namespace App