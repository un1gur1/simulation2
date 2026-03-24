#include "GameScene.h"
#include <DxLib.h>
#include "../Input/InputManager.h" 

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

        // マネージャーの初期化（ユニットの配置とか）を呼ぶ
        m_battleMaster.Init();
    }

    void GameScene::Load() {
        // 必要ならリソースをロード
    }

    void GameScene::LoadEnd() {
    }

    void GameScene::Update() {
        // マウスのクリック判定などは消去し、すべて BattleMaster に任せます
        m_battleMaster.Update();

        ++m_frameCount;
    }
    void GameScene::Draw() {
        // 1. マネージャーに全部描いてもらう（マップ、ユニット、ハイライト全部！）
        m_battleMaster.Draw();

        // 2. シーン特有のデバッグ情報だけここで描く
    /*    if (m_showInfo) {
            DrawFormatString(10, 10, GetColor(255, 255, 255), "GameScene");
            DrawFormatString(10, 30, GetColor(255, 255, 0), "Frame: %u", (unsigned)m_frameCount);
        }*/
    }

    void GameScene::Release() {
        // 実体で持っているなら、特別な解放処理は不要だにょ（自動で消えるから）
    }

} // namespace App