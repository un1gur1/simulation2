#include "GameScene.h"
#include <DxLib.h>
#include "../Input/InputManager.h" 
#include "SceneManager.h"   // ★追加：シーンマネージャー
#include "ResultScene.h"    // ★追加：リザルトシーンにデータを渡すため

namespace App {

    GameScene::GameScene()
        : m_frameCount(0)
        , m_showInfo(true)
        , m_prevMouse(0)
    {
    }

    GameScene::~GameScene() {
    }

    void GameScene::Init() {
        m_frameCount = 0;
        m_showInfo = true;
        m_battleMaster.Init(BattleMaster::GameMode::VS_PLAYER);
    }

    void GameScene::Load() {}
    void GameScene::LoadEnd() {}

    void GameScene::Update() {
        m_battleMaster.Update();

        // ★追加：ゲームオーバー判定とシーン遷移
        if (m_battleMaster.IsGameOver()) {
            // どっちが勝ったかを取得
            bool isWin = m_battleMaster.IsPlayerWin();

            // 次のシーン（ResultScene）に勝敗データをセット
            ResultScene::SetIsWin(isWin);

            // リザルト画面へ飛ぶ！
            // ※ SCENE_ID に RESULT が無い場合は SceneManager 側で追加してください
            SceneManager::GetInstance()->ChangeScene(SceneManager::SCENE_ID::RESULT);
        }

        ++m_frameCount;
    }

    void GameScene::Draw() {
        m_battleMaster.Draw();
    }

    void GameScene::Release() {
    }

} // namespace App