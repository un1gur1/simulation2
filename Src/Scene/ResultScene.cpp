#include "ResultScene.h"
#include <DxLib.h>
#include "SceneManager.h"
#include <cmath> // ぽわぽわ演出用

namespace App {

    bool ResultScene::s_isWin = false;

    ResultScene::ResultScene()
        : m_frameCount(0)
        
    {
    }

    ResultScene::~ResultScene() {
    }

    void ResultScene::Init() {
        m_frameCount = 0;
    }

    void ResultScene::Load() {
        // 必要ならリザルト用のBGMや画像をロード
    }

    void ResultScene::LoadEnd() {
    }

    void ResultScene::Update() {
        ++m_frameCount;

        // スペースキーでタイトル画面へ戻る
        if (CheckHitKey(KEY_INPUT_SPACE)) {
            // シーンマネージャーを使ってタイトルへ遷移
            SceneManager::GetInstance()->ChangeScene(SceneManager::SCENE_ID::TITLE);
        }
    }

    void ResultScene::Draw() {
        // 1. 背景を少し暗くする（演出）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
        DrawBox(0, 0, 1920, 1080, GetColor(10, 15, 25), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 2. 勝敗に応じたメインテキストの描画
        SetFontSize(80);
        if (s_isWin) {
            DrawFormatString(700, 400, GetColor(100, 255, 100), "MISSION CLEAR!");
            SetFontSize(32);
            DrawFormatString(800, 520, GetColor(200, 255, 200), "敵の撃破に成功しました");
        }
        else {
            DrawFormatString(700, 400, GetColor(255, 100, 100), "MISSION FAILED");
            SetFontSize(32);
            DrawFormatString(800, 520, GetColor(255, 200, 200), "プレイヤーの残機が尽きました...");
        }

        // 3. タイトルへ戻る案内（ぽわぽわ点滅演出付き）
        int pulseAlpha = 150 + (int)(std::sin(m_frameCount / 20.0f) * 100);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, pulseAlpha);
        SetFontSize(40);
        DrawFormatString(750, 800, GetColor(255, 255, 255), " SPACEキーでタイトルへ ");
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    void ResultScene::Release() {
        // 特別な解放処理が必要ならここに
    }

    void ResultScene::SetIsWin(bool isWin) {
        s_isWin = isWin;
    }

} // namespace App