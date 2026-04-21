#include "PauseMenu.h"
#include "../Input/InputManager.h" 
#include <cmath>

namespace App {

    // ==========================================
    // コンストラクタ: ポーズメニューの初期値設定
    // ==========================================
    PauseMenu::PauseMenu()
        : m_cursor(0)          // カーソル位置（0=再開, 1=タイトル, 2=終了）
        , m_angle(0.0f)        // アイコン回転角度
    {
    }

    // ==========================================
    // 初期化: メニューを開いた時の状態リセット
    // ==========================================
    void PauseMenu::Init() {
        m_cursor = 0;      // 常に「再開」が選択された状態で開く
        m_angle = 0.0f;
    }

    // ==========================================
    // 更新処理: 入力受付と選択結果の返却
    // 戻り値: NONE=選択中 / RESUME=再開 / TITLE=タイトルへ / EXIT=終了
    // ==========================================
    PauseMenu::Result PauseMenu::Update() {
        m_angle += 0.05f;  // 演算子アイコンの回転アニメーション

        auto& input = InputManager::GetInstance();

        // ==========================================
        // 1. マウス操作: ホバー＆クリック
        // ==========================================
        int mx, my;
        GetMousePoint(&mx, &my);

        // マウス移動検出（無駄なカーソル変更を防ぐ）
        static int prevMx = 0, prevMy = 0;
        bool mouseMoved = (mx != prevMx || my != prevMy);
        prevMx = mx; prevMy = my;

        // 矩形内判定用のラムダ関数
        auto HoverBox = [&](int x, int y, int w, int h) {
            return (mx >= x && mx <= x + w && my >= y && my <= y + h);
            };

        int sw = 1920;  // 画面幅

        // マウスホバーでカーソル移動（マウスが動いた時のみ）
        if (mouseMoved) {
            for (int i = 0; i < 3; ++i) {
                int ty = 520 + i * 100;  // 各メニュー項目のY座標
                if (HoverBox(sw / 2 - 450, ty - 10, 900, 80)) {
                    m_cursor = i;
                    break;
                }
            }
        }

        // マウスクリックで決定
        if (input.IsMouseLeftTrg()) {
            for (int i = 0; i < 3; ++i) {
                int ty = 520 + i * 100;
                if (HoverBox(sw / 2 - 450, ty - 10, 900, 80)) {
                    m_cursor = i;
                    goto DECIDE;  // 決定処理へジャンプ
                }
            }
        }

        // ==========================================
        // 2. キーボード操作: 十字キー＆WASD対応
        // ==========================================

        // 上移動: ↑キー または Wキー
        if (input.IsTrgDown(KEY_INPUT_UP) || input.IsTrgDown(KEY_INPUT_W)) {
            m_cursor = (m_cursor - 1 + 3) % 3;  // 0→2へループ
        }

        // 下移動: ↓キー または Sキー
        if (input.IsTrgDown(KEY_INPUT_DOWN) || input.IsTrgDown(KEY_INPUT_S)) {
            m_cursor = (m_cursor + 1) % 3;  // 2→0へループ
        }

        // 決定: Spaceキー または Enterキー
        if (input.IsTrgDown(KEY_INPUT_SPACE) || input.IsTrgDown(KEY_INPUT_RETURN)) {
            goto DECIDE;
        }

        return Result::NONE;  // まだ選択していない

        // ==========================================
        // 決定処理: カーソル位置に応じた結果を返す
        // ==========================================
    DECIDE:
        if (m_cursor == 0) return Result::RESUME;  // ゲームに戻る
        if (m_cursor == 1) return Result::TITLE;   // タイトル画面へ
        if (m_cursor == 2) return Result::EXIT;    // ゲーム終了

        return Result::NONE;
    }

    // ==========================================
    // 描画処理: ポーズメニュー画面の表示
    // ==========================================
    void PauseMenu::Draw() {
        int sw = 1920, sh = 1080;  // 画面サイズ

        // ==========================================
        // 1. 背景オーバーレイ: 半透明の暗幕
        // ==========================================
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
        DrawBox(0, 0, sw, sh, GetColor(2, 5, 25), TRUE);

        // サイバー風グリッド線（80ピクセル間隔）
        for (int i = 0; i < sw; i += 80)
            DrawLine(i, 0, i, sh, GetColor(0, 80, 180), 1);
        for (int j = 0; j < sh; j += 80)
            DrawLine(0, j, sw, j, GetColor(0, 80, 180), 1);

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // ==========================================
        // 2. ヘッダー: 演算子アイコンとタイトル
        // ==========================================
        DrawOperatorIcon(sw / 2, 220, 60, GetColor(0, 255, 255));

        SetFontSize(80);
        DrawString(sw / 2 - 280, 300, "SYSTEM PAUSED", GetColor(200, 240, 255));

        // ==========================================
        // 3. メニュー項目: 3つの選択肢
        // ==========================================
        SetFontSize(40);
        const char* items[] = {
            "[A-01] 演算再開 : RESUME PROCESS",      // ゲームに戻る
            "[A-02] タイトルへ : EXIT TO TITLE",     // タイトル画面へ
            "[A-03] システム終了 : SHUTDOWN"         // ゲーム終了
        };

        for (int i = 0; i < 3; ++i) {
            int ty = 520 + i * 100;  // Y座標（100ピクセル間隔）

            // 選択中かどうかで色を変える
            unsigned int col = (m_cursor == i)
                ? GetColor(255, 160, 0)      // 選択中: オレンジ
                : GetColor(70, 130, 180);    // 非選択: 青

            // 選択中の項目は背景を光らせる
            if (m_cursor == i) {
                // 半透明の背景矩形
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 60);
                DrawBox(sw / 2 - 450, ty - 10, sw / 2 + 450, ty + 70, col, TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

                // 枠線
                DrawBox(sw / 2 - 450, ty - 10, sw / 2 + 450, ty + 70, col, FALSE);

                // 選択マーカー「>>」
                DrawString(sw / 2 - 520, ty + 5, ">>", col);
            }

            // メニューテキスト
            DrawString(sw / 2 - 400, ty + 5, items[i], col);
        }
    }

    // ==========================================
    // 演算子アイコン描画: 回転する「＊」マーク
    // 引数: x, y - 中心座標 / size - 半径 / color - 色
    // ==========================================
    void PauseMenu::DrawOperatorIcon(int x, int y, int size, unsigned int color) {
        // 8方向に線を伸ばして「*」を表現
        for (int i = 0; i < 8; ++i) {
            // 各線の角度計算（360度を8等分）
            float th = m_angle + (DX_PI_F * 2.0f / 8.0f) * i;

            // 線の終点座標
            int x2 = x + (int)(cosf(th) * size);
            int y2 = y + (int)(sinf(th) * size);

            // 中心から外へ伸びる線
            DrawLine(x, y, x2, y2, color, 5);

            // 線の先端に小さな四角を配置（装飾）
            DrawBox(x2 - 5, y2 - 5, x2 + 5, y2 + 5, color, TRUE);
        }
    }

} // namespace App