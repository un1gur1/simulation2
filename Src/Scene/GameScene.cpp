#include "GameScene.h"
#include <DxLib.h>
#include "../Input/InputManager.h" 
#include "SceneManager.h"   // シーンマネージャー

// ★ ResultScene.h はもう直接触らないのでインクルード不要！

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
        m_battleMaster.Init();
    }

    void GameScene::Load() {}
    void GameScene::LoadEnd() {}

    void GameScene::Update() {
        // ★ バトルの進行、決着演出、戦績集計、リザルトへの遷移は
        // すべて BattleMaster がやってくれるので、ここは Update を呼ぶだけでOK！
        m_battleMaster.Update();

        ++m_frameCount;
    }

    void GameScene::Draw() {
        m_battleMaster.Draw();
    }

    void GameScene::Release() {
    }

} // namespace App