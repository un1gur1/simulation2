#include "TitleScene.h"
#include <DxLib.h>
#include "SceneManager.h"

namespace App {

    TitleScene::TitleScene()
        : m_frameCount(0), m_state(MenuState::SELECT_PLAYERS)
        , m_playerCursor(1), m_modeCursor(0), m_scoreCursor(1) 
        , m_prevSpace(0), m_prevUp(0), m_prevDown(0), m_prevB(0)
    {
    }

    TitleScene::~TitleScene() {}

    void TitleScene::Init() {
        m_frameCount = 0;
        m_state = MenuState::SELECT_PLAYERS;
        m_playerCursor = 1;
        m_modeCursor = 0;
        m_scoreCursor = 1; 

        m_prevSpace = CheckHitKey(KEY_INPUT_SPACE);
        m_prevUp = CheckHitKey(KEY_INPUT_UP);
        m_prevDown = CheckHitKey(KEY_INPUT_DOWN);
        m_prevB = CheckHitKey(KEY_INPUT_B);
    }

    void TitleScene::Load() {}
    void TitleScene::LoadEnd() {}

    void TitleScene::Update() {
        ++m_frameCount;
        if (m_frameCount < 30) return;

        int currSpace = CheckHitKey(KEY_INPUT_SPACE);
        int currUp = CheckHitKey(KEY_INPUT_UP);
        int currDown = CheckHitKey(KEY_INPUT_DOWN);
        int currB = CheckHitKey(KEY_INPUT_B);

        bool spaceTrg = (currSpace == 1 && m_prevSpace == 0);
        bool upTrg = (currUp == 1 && m_prevUp == 0);
        bool downTrg = (currDown == 1 && m_prevDown == 0);
        bool bTrg = (currB == 1 && m_prevB == 0);

        m_prevSpace = currSpace;
        m_prevUp = currUp;
        m_prevDown = currDown;
        m_prevB = currB;

        if (m_state == MenuState::SELECT_PLAYERS) {
            if (upTrg) { m_playerCursor--; if (m_playerCursor < 1) m_playerCursor = 2; }
            if (downTrg) { m_playerCursor++; if (m_playerCursor > 2) m_playerCursor = 1; }
            if (spaceTrg) { m_state = MenuState::SELECT_MODE; m_frameCount = 0; }
        }
        else if (m_state == MenuState::SELECT_MODE) {
            if (upTrg) { m_modeCursor--; if (m_modeCursor < 0) m_modeCursor = 1; }
            if (downTrg) { m_modeCursor++; if (m_modeCursor > 1) m_modeCursor = 0; }
            if (bTrg) { m_state = MenuState::SELECT_PLAYERS; m_frameCount = 0; }
            if (spaceTrg) {
                if (m_modeCursor == 1) {
                    // ゼロワンを選んだらスコア選択へ
                    m_state = MenuState::SELECT_SCORE;
                    m_frameCount = 0;
                }
                else {
                    // クラシックはそのまま開始
                    SceneManager::GetInstance()->SetGameSettings(m_playerCursor, 0, 0);
                    SceneManager::GetInstance()->ChangeScene(SceneManager::SCENE_ID::GAME);
                }
            }
        }
        else if (m_state == MenuState::SELECT_SCORE) {
            if (upTrg) { m_scoreCursor--; if (m_scoreCursor < 0) m_scoreCursor = 2; }
            if (downTrg) { m_scoreCursor++; if (m_scoreCursor > 2) m_scoreCursor = 0; }
            if (bTrg) { m_state = MenuState::SELECT_MODE; m_frameCount = 0; }
            if (spaceTrg) {
                // ★選んだスコアをセットして開始
                int scores[3] = { 53, 103, 223 };
                SceneManager::GetInstance()->SetGameSettings(m_playerCursor, 1, scores[m_scoreCursor]);
                SceneManager::GetInstance()->ChangeScene(SceneManager::SCENE_ID::GAME);
            }
        }
    }

    void TitleScene::Draw() {
        SetFontSize(48);
        DrawString(100, 80, "超計算", GetColor(255, 255, 255));

        SetFontSize(32);
        if (m_state == MenuState::SELECT_PLAYERS) {
            DrawString(100, 200, "【 プレイ人数を選んでください 】", GetColor(255, 255, 100));
            const char* menu[2] = { "1人プレイ (対CPU)", "2人プレイ (ローカル)" };
            for (int i = 0; i < 2; ++i) {
                unsigned int color = (m_playerCursor == i + 1) ? GetColor(255, 100, 100) : GetColor(150, 150, 150);
                if (m_playerCursor == i + 1) DrawString(120, 280 + i * 50, "→", color);
                DrawString(160, 280 + i * 50, menu[i], color);
            }
        }
        else if (m_state == MenuState::SELECT_MODE) {
            DrawString(100, 200, "【 ゲームモードを選んでください 】", GetColor(100, 255, 100));
            const char* menu[2] = { "ノーマルバトル (相手の残機を0にする)", "カウントバトル (数値をピッタリ0にする)" };
            for (int i = 0; i < 2; ++i) {
                unsigned int color = (m_modeCursor == i) ? GetColor(100, 200, 255) : GetColor(150, 150, 150);
                if (m_modeCursor == i) DrawString(120, 280 + i * 50, "→", color);
                DrawString(160, 280 + i * 50, menu[i], color);
            }
        }
        else if (m_state == MenuState::SELECT_SCORE) {
            DrawString(100, 200, "【 初期スコアを選んでください 】", GetColor(255, 150, 255));
            const char* menu[3] = { "53", "103", "223" };
            for (int i = 0; i < 3; ++i) {
                unsigned int color = (m_scoreCursor == i) ? GetColor(255, 200, 255) : GetColor(150, 150, 150);
                if (m_scoreCursor == i) DrawString(120, 280 + i * 50, "→", color);
                DrawString(160, 280 + i * 50, menu[i], color);
            }
        }

        SetFontSize(24);
        if (m_state == MenuState::SELECT_PLAYERS) DrawString(100, 550, "↑↓キー: 選択  /  SPACEキー: 決定", GetColor(120, 120, 120));
        else DrawString(100, 550, "↑↓キー: 選択  /  SPACEキー: 決定  /  Bキー: 戻る", GetColor(120, 120, 120));
    }

    void TitleScene::Release() {}

} // namespace App