#include "TitleScene.h"
#include <DxLib.h>
#include "SceneManager.h"
#include<string>
namespace App {

    TitleScene::TitleScene()
        : m_frameCount(0), m_state(MenuState::SELECT_PLAYERS)
        , m_playerCursor(1), m_modeCursor(0), m_scoreCursor(1)
        , m_p1TypeCursor(0), m_p2TypeCursor(1)
        , m_p1CustomCursor(0), m_p2CustomCursor(0)
        , m_p1StartNum(5), m_p1StartX(1), m_p1StartY(1)
        , m_p2StartNum(7), m_p2StartX(7), m_p2StartY(7)
        , m_prevSpace(0), m_prevUp(0), m_prevDown(0), m_prevRight(0), m_prevLeft(0), m_prevB(0)
    {
    }

    TitleScene::~TitleScene() {}

    void TitleScene::Init() {
        m_frameCount = 0;
        m_state = MenuState::SELECT_PLAYERS;
        m_playerCursor = 1;
        m_modeCursor = 0;
        m_scoreCursor = 1;

        m_p1TypeCursor = 0; // デフォルト：1P=人間
        m_p2TypeCursor = 1; // デフォルト：2P=COM
        m_p1CustomCursor = 0;
        m_p2CustomCursor = 0;

        m_p1StartNum = 5; m_p1StartX = 1; m_p1StartY = 1;
        m_p2StartNum = 7; m_p2StartX = 7; m_p2StartY = 7;

        m_prevSpace = CheckHitKey(KEY_INPUT_SPACE);
        m_prevUp = CheckHitKey(KEY_INPUT_UP);
        m_prevDown = CheckHitKey(KEY_INPUT_DOWN);
        m_prevRight = CheckHitKey(KEY_INPUT_RIGHT);
        m_prevLeft = CheckHitKey(KEY_INPUT_LEFT);
        m_prevB = CheckHitKey(KEY_INPUT_B);
    }

    void TitleScene::Load() {}
    void TitleScene::LoadEnd() {}

    void TitleScene::Update() {
        ++m_frameCount;
        if (m_frameCount < 10) return;

        int currSpace = CheckHitKey(KEY_INPUT_SPACE) | CheckHitKey(KEY_INPUT_RETURN);
        int currUp = CheckHitKey(KEY_INPUT_UP);
        int currDown = CheckHitKey(KEY_INPUT_DOWN);
        int currRight = CheckHitKey(KEY_INPUT_RIGHT);
        int currLeft = CheckHitKey(KEY_INPUT_LEFT);
        int currB = CheckHitKey(KEY_INPUT_B) | CheckHitKey(KEY_INPUT_BACK);

        bool spaceTrg = (currSpace != 0 && m_prevSpace == 0);
        bool upTrg = (currUp != 0 && m_prevUp == 0);
        bool downTrg = (currDown != 0 && m_prevDown == 0);
        bool rightTrg = (currRight != 0 && m_prevRight == 0);
        bool leftTrg = (currLeft != 0 && m_prevLeft == 0);
        bool bTrg = (currB != 0 && m_prevB == 0);

        m_prevSpace = currSpace;
        m_prevUp = currUp;
        m_prevDown = currDown;
        m_prevRight = currRight;
        m_prevLeft = currLeft;
        m_prevB = currB;

        // 1. 人数選択
        if (m_state == MenuState::SELECT_PLAYERS) {
            if (upTrg || downTrg) { m_playerCursor = (m_playerCursor == 1) ? 2 : 1; }
            if (spaceTrg) {
                // 1人プレイなら2Pは強制COM、2人プレイなら強制人間としてカーソルを初期化しておく
                m_p2TypeCursor = (m_playerCursor == 1) ? 1 : 0;
                m_state = MenuState::SELECT_MODE;
                m_frameCount = 0;
            }
        }
        // 2. モード選択
        else if (m_state == MenuState::SELECT_MODE) {
            if (upTrg || downTrg) { m_modeCursor = (m_modeCursor == 0) ? 1 : 0; }
            if (bTrg) { m_state = MenuState::SELECT_PLAYERS; m_frameCount = 0; }
            if (spaceTrg) {
                if (m_modeCursor == 1) m_state = MenuState::SELECT_SCORE; // ゼロワンならスコアへ
                else m_state = MenuState::SELECT_P1_TYPE;                 // クラシックなら操作タイプへ
                m_frameCount = 0;
            }
        }
        // 3. 目標スコア選択
        else if (m_state == MenuState::SELECT_SCORE) {
            if (upTrg) { m_scoreCursor--; if (m_scoreCursor < 0) m_scoreCursor = 2; }
            if (downTrg) { m_scoreCursor++; if (m_scoreCursor > 2) m_scoreCursor = 0; }
            if (bTrg) { m_state = MenuState::SELECT_MODE; m_frameCount = 0; }
            if (spaceTrg) { m_state = MenuState::SELECT_P1_TYPE; m_frameCount = 0; } // 操作タイプへ
        }
        // 4. 1P 操作タイプ選択
        else if (m_state == MenuState::SELECT_P1_TYPE) {
            if (upTrg || downTrg) { m_p1TypeCursor = (m_p1TypeCursor == 0) ? 1 : 0; }
            if (bTrg) { m_state = (m_modeCursor == 1) ? MenuState::SELECT_SCORE : MenuState::SELECT_MODE; m_frameCount = 0; }
            if (spaceTrg) { m_state = MenuState::SELECT_P2_TYPE; m_frameCount = 0; }
        }
        // 5. 2P 操作タイプ選択
        else if (m_state == MenuState::SELECT_P2_TYPE) {
            if (upTrg || downTrg) { m_p2TypeCursor = (m_p2TypeCursor == 0) ? 1 : 0; }
            if (bTrg) { m_state = MenuState::SELECT_P1_TYPE; m_frameCount = 0; }
            if (spaceTrg) { m_state = MenuState::CUSTOM_P1_START; m_frameCount = 0; }
        }
        // 6. 1P カスタム設定
        else if (m_state == MenuState::CUSTOM_P1_START) {
            if (upTrg) { m_p1CustomCursor--; if (m_p1CustomCursor < 0) m_p1CustomCursor = 2; }
            if (downTrg) { m_p1CustomCursor++; if (m_p1CustomCursor > 2) m_p1CustomCursor = 0; }

            // 左右キーで数値を増減
            if (rightTrg) {
                if (m_p1CustomCursor == 0) { m_p1StartNum++; if (m_p1StartNum > 9) m_p1StartNum = 1; }
                if (m_p1CustomCursor == 1) { m_p1StartX++; if (m_p1StartX > 9) m_p1StartX = 1; }
                if (m_p1CustomCursor == 2) { m_p1StartY++; if (m_p1StartY > 9) m_p1StartY = 1; }
            }
            if (leftTrg) {
                if (m_p1CustomCursor == 0) { m_p1StartNum--; if (m_p1StartNum < 1) m_p1StartNum = 9; }
                if (m_p1CustomCursor == 1) { m_p1StartX--; if (m_p1StartX < 1) m_p1StartX = 9; }
                if (m_p1CustomCursor == 2) { m_p1StartY--; if (m_p1StartY < 1) m_p1StartY = 9; }
            }
            if (bTrg) { m_state = MenuState::SELECT_P2_TYPE; m_frameCount = 0; }
            if (spaceTrg) { m_state = MenuState::CUSTOM_P2_START; m_frameCount = 0; }
        }
        // 7. 2P カスタム設定＆ゲーム開始
        else if (m_state == MenuState::CUSTOM_P2_START) {
            if (upTrg) { m_p2CustomCursor--; if (m_p2CustomCursor < 0) m_p2CustomCursor = 2; }
            if (downTrg) { m_p2CustomCursor++; if (m_p2CustomCursor > 2) m_p2CustomCursor = 0; }

            if (rightTrg) {
                if (m_p2CustomCursor == 0) { m_p2StartNum++; if (m_p2StartNum > 9) m_p2StartNum = 1; }
                if (m_p2CustomCursor == 1) { m_p2StartX++; if (m_p2StartX > 9) m_p2StartX = 1; }
                if (m_p2CustomCursor == 2) { m_p2StartY++; if (m_p2StartY > 9) m_p2StartY = 1; }
            }
            if (leftTrg) {
                if (m_p2CustomCursor == 0) { m_p2StartNum--; if (m_p2StartNum < 1) m_p2StartNum = 9; }
                if (m_p2CustomCursor == 1) { m_p2StartX--; if (m_p2StartX < 1) m_p2StartX = 9; }
                if (m_p2CustomCursor == 2) { m_p2StartY--; if (m_p2StartY < 1) m_p2StartY = 9; }
            }
            if (bTrg) { m_state = MenuState::CUSTOM_P1_START; m_frameCount = 0; }
            if (spaceTrg) {
                // ★すべて確定！ SceneManagerに流し込んでゲーム開始
                int scores[3] = { 53, 103, 223 };
                int finalScore = (m_modeCursor == 1) ? scores[m_scoreCursor] : 0;

                auto* sm = SceneManager::GetInstance();
                sm->SetGameSettings(m_playerCursor, m_modeCursor, finalScore);
                sm->SetPlayer1Settings(m_p1TypeCursor == 1, m_p1StartNum, m_p1StartX - 1, 9 - m_p1StartY); // 画面座標系に変換
                sm->SetPlayer2Settings(m_p2TypeCursor == 1, m_p2StartNum, m_p2StartX - 1, 9 - m_p2StartY);

                sm->ChangeScene(SceneManager::SCENE_ID::GAME);
            }
        }
    }

    void TitleScene::Draw() {
        SetFontSize(48);
        DrawString(100, 80, "超計算 CUSTOM", GetColor(255, 255, 255));

        SetFontSize(32);

        // メニュー描画の共通ラムダ
        auto drawSimpleMenu = [&](int cursor, const char* title, const char* m0, const char* m1, const char* m2 = nullptr) {
            DrawString(100, 200, title, GetColor(100, 255, 100));
            unsigned int c0 = (cursor == 0) ? GetColor(255, 200, 100) : GetColor(150, 150, 150);
            unsigned int c1 = (cursor == 1) ? GetColor(255, 200, 100) : GetColor(150, 150, 150);
            if (cursor == 0) DrawString(120, 280, "→", c0); DrawString(160, 280, m0, c0);
            if (cursor == 1) DrawString(120, 330, "→", c1); DrawString(160, 330, m1, c1);
            if (m2) {
                unsigned int c2 = (cursor == 2) ? GetColor(255, 200, 100) : GetColor(150, 150, 150);
                if (cursor == 2) DrawString(120, 380, "→", c2); DrawString(160, 380, m2, c2);
            }
            };

        if (m_state == MenuState::SELECT_PLAYERS) {
            drawSimpleMenu(m_playerCursor - 1, "【 プレイ人数を選んでください 】", "1人プレイ (1P vs COM)", "2人プレイ (1P vs 2P)");
        }
        else if (m_state == MenuState::SELECT_MODE) {
            drawSimpleMenu(m_modeCursor, "【 ゲームモードを選んでください 】", "クラシック (相手の残機を0にする)", "ゼロワン (数値をピッタリ到達させる)");
        }
        else if (m_state == MenuState::SELECT_SCORE) {
            drawSimpleMenu(m_scoreCursor, "【 初期スコアを選んでください 】", "53", "103", "223");
        }
        else if (m_state == MenuState::SELECT_P1_TYPE) {
            drawSimpleMenu(m_p1TypeCursor, "【 1P の操作タイプを選んでください 】", "人間が操作する (PLAYER)", "AIに任せる (COM)");
        }
        else if (m_state == MenuState::SELECT_P2_TYPE) {
            drawSimpleMenu(m_p2TypeCursor, "【 2P の操作タイプを選んでください 】", "人間が操作する (PLAYER)", "AIに任せる (COM)");
        }
        else if (m_state == MenuState::CUSTOM_P1_START || m_state == MenuState::CUSTOM_P2_START) {
            bool is1P = (m_state == MenuState::CUSTOM_P1_START);
            std::string title = is1P ? "【 1P の初期設定 】" : "【 2P の初期設定 】";
            DrawString(100, 200, title.c_str(), is1P ? GetColor(255, 200, 100) : GetColor(100, 200, 255));

            int cursor = is1P ? m_p1CustomCursor : m_p2CustomCursor;
            int num = is1P ? m_p1StartNum : m_p2StartNum;
            int cx = is1P ? m_p1StartX : m_p2StartX;
            int cy = is1P ? m_p1StartY : m_p2StartY;

            const char* items[3] = { "初期の数字 : ", "初期のＸ座標 (1-9) : ", "初期のＹ座標 (1-9) : " };
            int vals[3] = { num, cx, cy };

            for (int i = 0; i < 3; ++i) {
                unsigned int color = (cursor == i) ? GetColor(255, 255, 255) : GetColor(150, 150, 150);
                if (cursor == i) DrawString(120, 280 + i * 50, "→", color);

                char buf[128];
                if (cursor == i) sprintf_s(buf, "%s <  %d  >", items[i], vals[i]); // 選択中は左右の矢印を表示
                else sprintf_s(buf, "%s    %d   ", items[i], vals[i]);

                DrawString(160, 280 + i * 50, buf, color);
            }
        }

        SetFontSize(24);
        if (m_state == MenuState::SELECT_PLAYERS) {
            DrawString(100, 550, "↑↓キー: 選択  /  SPACEキー: 決定", GetColor(120, 120, 120));
        }
        else if (m_state == MenuState::CUSTOM_P1_START || m_state == MenuState::CUSTOM_P2_START) {
            DrawString(100, 550, "↑↓キー: 項目選択  /  ←→キー: 数値変更  /  SPACEキー: 次へ  /  Bキー: 戻る", GetColor(120, 120, 120));
        }
        else {
            DrawString(100, 550, "↑↓キー: 選択  /  SPACEキー: 決定  /  Bキー: 戻る", GetColor(120, 120, 120));
        }
    }

    void TitleScene::Release() {}

} // namespace App