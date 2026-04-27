#include "TitleScene.h"
#include <DxLib.h>
#include "SceneManager.h"
#include "../Input/InputManager.h" 
#include"../Manager/ProceduralAudio.h"
#include <string>
#include <cmath>
#include "../../CyberGrid.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ==========================================
// UI定数定義: レイアウト・色・アニメーション設定
// 無名名前空間で外部から隠蔽
// ==========================================
namespace {
    // 黄金比・白銀比（将来的なレイアウト調整用）
    constexpr double GOLDEN = 1.618;
    constexpr double SILVER = 1.414;

    // システム設定
    constexpr int WAIT_START_FRAMES = 10;              // 起動直後の待機フレーム数
    constexpr int TARGET_SCORES[3] = { 53, 103, 223 }; // ゼロワンモードの目標スコア選択肢

    // レイアウト: 基準座標
    constexpr int MENU_CENTER_OFFSET_Y = -130;  // メニュー全体の縦オフセット（画面中央から）
    constexpr int MENU_CUSTOM_OFFSET_X = -300;  // カスタム画面の横オフセット（ミニマップ配置用）

    // レイアウト: メニュー項目のサイズと間隔
    constexpr int MENU_ITEM_BASE_Y = 100;       // 最初の項目のY座標（メニュー開始位置から）
    constexpr int MENU_ITEM_STEP_Y = 85;        // 項目間の縦間隔
    constexpr int MENU_BOX_HALF_W = 380;        // 選択ボックスの半分の幅
    constexpr int MENU_BOX_H = 55;              // 選択ボックスの高さ
    constexpr int MENU_BOX_OFFSET_Y = 10;       // 選択ボックスのYオフセット

    // レイアウト: カスタム設定の増減ボタン
    constexpr int CUSTOM_BTN_BASE_X = 140;      // ボタンの基準X座標
    constexpr int CUSTOM_BTN_OFFSET_X = 60;     // ボタンの横間隔
    constexpr int CUSTOM_BTN_SIZE = 50;         // ボタンのサイズ

    // レイアウト: ミニマップ（配置プレビュー）
    constexpr int MAP_CELL_SIZE = 45;           // マス目1個のサイズ
    constexpr int MAP_BASE_OFFSET_X = 180;      // マップのXオフセット
    constexpr int MAP_BASE_OFFSET_Y = 40;       // マップのYオフセット

    // アニメーション設定
    constexpr double BLINK_SPEED = 3.0;         // 点滅速度（Hz）
    constexpr int BLINK_BASE_ALPHA = 150;       // 基本透明度
    constexpr int BLINK_AMP_ALPHA = 105;        // 透明度の振幅

    // カラーパレット: サイバー風配色
    inline unsigned int COL_BG() { return GetColor(5, 10, 25); }          // 背景（暗い青）
    inline unsigned int COL_GRID() { return GetColor(0, 150, 255); }      // グリッド線（青）
    inline unsigned int COL_P1() { return GetColor(255, 140, 0); }        // 1Pカラー（オレンジ）
    inline unsigned int COL_P2() { return GetColor(0, 150, 255); }        // 2Pカラー（青）
    inline unsigned int COL_TEXT_ON() { return GetColor(255, 180, 0); }   // 選択中テキスト（オレンジ）
    inline unsigned int COL_TEXT_OFF() { return GetColor(50, 100, 150); } // 非選択テキスト（暗い青）
    inline unsigned int COL_WHITE() { return GetColor(255, 255, 255); }   // 白
    inline unsigned int COL_BLACK() { return GetColor(0, 0, 0); }         // 黒
    inline unsigned int COL_TITLE_MAIN() { return GetColor(220, 245, 255); } // タイトルメイン（明るい青）
    inline unsigned int COL_TITLE_SUB() { return GetColor(0, 120, 255); }    // タイトルサブ（青）
    inline unsigned int COL_DANGER() { return GetColor(255, 100, 100); }  // 警告・終了（赤）
}

namespace App {

    // ==========================================
    // コンストラクタ: 初期値設定
    // ==========================================
    TitleScene::TitleScene()
        : m_frameCount(0)                      // フレームカウンター
        , m_state(MenuState::PRESS_START)     // 初期状態: スタート画面
        , m_playerCursor(1)                    // プレイヤー数カーソル（1=シングル, 2=2P）
        , m_modeCursor(0)                      // モードカーソル（0=クラシック, 1=ゼロワン）
        , m_scoreCursor(1)                     // スコアカーソル（0,1,2）
        , m_shouldQuit(false)                  // 終了フラグ
        , m_fontTitle(-1)                      // タイトルフォントハンドル
        , m_fontMenu(-1)                       // メニューフォントハンドル
        , m_fontSmall(-1)                      // 小さいフォントハンドル
        , m_fontNumber(-1)                     // 数値フォントハンドル
        , m_psHandle(-1)                       // ピクセルシェーダーハンドル
        , m_cbHandle(-1)                       // 定数バッファハンドル
        , m_shaderTime(0.0f)                   // シェーダー時間（アニメーション用）
        , m_players()                          // プレイヤー設定配列
        , m_stocksCursor(-1)                   // 残機カーソル
    {
    }

    // ==========================================
    // デストラクタ
    // ==========================================
    TitleScene::~TitleScene() {}

    // ==========================================
    // 初期化: フォント・シェーダーの読み込み
    // シーン開始時に1回だけ呼ばれる
    // ==========================================
    void TitleScene::Init() {
        m_frameCount = 0;
        m_state = MenuState::PRESS_START;
        m_playerCursor = 1;
        m_modeCursor = 0;
        m_scoreCursor = 1;
        m_stocksCursor = 0;
        m_shouldQuit = false;
        m_stageCursor = 0;

        // プレイヤー設定の初期化
        m_players[0] = { 0, 0, DEF_P1_HP, DEF_P1_X, DEF_P1_Y };
        m_players[1] = { 1, 0, DEF_P2_HP, DEF_P2_X, DEF_P2_Y };

        // ProceduralAudioの初期化
        ProceduralAudio::GetInstance().PlayBGM(true);

        // フォント作成
        m_fontTitle = CreateFontToHandle("BIZ UD明朝 Medium", 100, 3, DX_FONTTYPE_ANTIALIASING);
        m_fontMenu = CreateFontToHandle("BIZ UDゴシック", 40, 2, DX_FONTTYPE_NORMAL);
        m_fontSmall = CreateFontToHandle("遊ゴシック", 24, 2, DX_FONTTYPE_NORMAL);
        m_fontNumber = CreateFontToHandle("HGP創英角ﾎﾟｯﾌﾟ体", 48, 2, DX_FONTTYPE_ANTIALIASING);

        // 背景シェーダー読み込み）
        m_psHandle = LoadPixelShaderFromMem(g_ps_CyberGrid, sizeof(g_ps_CyberGrid));
        m_cbHandle = CreateShaderConstantBuffer(sizeof(float) * 4);
        m_shaderTime = 0.0f;
    }

    // ==========================================
    // リソース読み込み: 
    // ==========================================
    void TitleScene::Load() {}
    void TitleScene::LoadEnd() {}

    // ==========================================
    // 更新処理: 入力受付とメニュー遷移制御
    // 多階層メニューのステートマシン
    // ==========================================
    void TitleScene::Update() {
        auto& input = InputManager::GetInstance();

        ProceduralAudio::GetInstance().Update();

        ++m_frameCount;
        if (m_frameCount < WAIT_START_FRAMES) return;  // 初期待機

        m_shaderTime += 0.0016f;  // シェーダーアニメーション時間を進める

        // ==========================================
        // 入力取得: キーボード・マウス統合
        // ==========================================
        bool spaceTrg = input.IsTrgDown(KEY_INPUT_SPACE) || input.IsTrgDown(KEY_INPUT_RETURN);
        bool upTrg = input.IsTrgDown(KEY_INPUT_UP) || input.IsTrgDown(KEY_INPUT_W);
        bool downTrg = input.IsTrgDown(KEY_INPUT_DOWN) || input.IsTrgDown(KEY_INPUT_S);
        bool rightTrg = input.IsTrgDown(KEY_INPUT_RIGHT) || input.IsTrgDown(KEY_INPUT_D);
        bool leftTrg = input.IsTrgDown(KEY_INPUT_LEFT) || input.IsTrgDown(KEY_INPUT_A);
        bool bTrg = input.IsTrgDown(KEY_INPUT_B) || input.IsTrgDown(KEY_INPUT_BACK);

        // 数値直接入力（1~9キー、テンキー対応）
        int hitNum = -1;
        for (int i = 0; i < GRID_SIZE; ++i) {
            if (input.IsTrgDown(KEY_INPUT_1 + i) || input.IsTrgDown(KEY_INPUT_NUMPAD1 + i)) {
                hitNum = i + 1;
            }
        }

        // マウス入力
        Vector2 m = input.GetMousePos();
        bool mClick = input.IsMouseLeftTrg();

        // マウス移動検出（ホバー判定用）
        static Vector2 prevM = m;
        bool mouseMoved = (m.x != prevM.x || m.y != prevM.y);
        prevM = m;

        // 矩形内判定用ラムダ関数
        auto HoverBox = [&](int x, int y, int w, int h) {
            return (m.x >= x && m.x <= x + w && m.y >= y && m.y <= y + h);
            };

        // 画面サイズとレイアウト計算
        int sw, sh;
        GetDrawScreenSize(&sw, &sh);
        int CX = sw / 2;  // 画面中央X
        int CY = sh / 2;  // 画面中央Y
        int menuStartY = CY + MENU_CENTER_OFFSET_Y;

        // ==========================================
        // 右上のEXITボタンのクリック判定
        // ==========================================
        int exitBtnW = 160;
        int exitBtnH = 50;
        int exitBtnX = sw - exitBtnW - 20;
        int exitBtnY = 20;

        if (HoverBox(exitBtnX, exitBtnY, exitBtnW, exitBtnH)) {
            if (mouseMoved) ProceduralAudio::GetInstance().PlayPowerSE(2); // ホバー音
            if (mClick) {
                SceneManager::GetInstance()->TogglePause();
                return;
            }
        }

        // カスタム設定画面かどうか
        bool isCustomState = (m_state == MenuState::CUSTOM_P1_START || m_state == MenuState::CUSTOM_P2_START);
        int menuCX = isCustomState ? CX + MENU_CUSTOM_OFFSET_X : CX;

        // 戻るボタンの座標（画面下部中央）
        int backBtnX = CX - 150;
        int backBtnY = sh - 150;
        int backBtnW = 300;
        int backBtnH = 60;

        // 戻るボタンのクリック判定を変数化しておく
        bool isBackBtnClicked = mClick && HoverBox(backBtnX, backBtnY, backBtnW, backBtnH);

        // ==========================================
        // ステートマシン: メニュー階層の遷移制御
        // ==========================================

        // --- [1] スタート画面 ---
        if (m_state == MenuState::PRESS_START) {
            if (spaceTrg || mClick) {
                ProceduralAudio::GetInstance().PlayPowerSE(9); //  決定音
                m_state = MenuState::SELECT_PLAYERS;
                m_frameCount = 0;
            }
        }
        // --- [2] プレイヤー数選択（シングル / 2P対戦） ---
        else if (m_state == MenuState::SELECT_PLAYERS) {
            for (int i = 0; i < 2; ++i) {
                if (HoverBox(menuCX - MENU_BOX_HALF_W, menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y - MENU_BOX_OFFSET_Y, MENU_BOX_HALF_W * 2, MENU_BOX_H)) {
                    if (mouseMoved && m_playerCursor != i + 1) {
                        m_playerCursor = i + 1;
                        ProceduralAudio::GetInstance().PlayPowerSE(2); // ウスホバー音
                    }
                    if (mClick) { m_playerCursor = i + 1; spaceTrg = true; }
                }
            }
            if (upTrg || downTrg) {
                m_playerCursor = (m_playerCursor == 1) ? 2 : 1;
                ProceduralAudio::GetInstance().PlayPowerSE(2); // キーボード移動音
            }

            if (bTrg || isBackBtnClicked) {
                ProceduralAudio::GetInstance().PlayErrorSE(); // キャンセル音
                m_state = MenuState::PRESS_START;
                m_frameCount = 0;
            }
            else if (spaceTrg) {
                ProceduralAudio::GetInstance().PlayPowerSE(9); // 決定音
                m_players[1].typeCursor = (m_playerCursor == 1) ? 1 : 0;
                m_state = MenuState::SELECT_MODE;
                m_frameCount = 0;
            }
        }
        // --- [3] ゲームモード選択（ノーマル / カウント） ---
        else if (m_state == MenuState::SELECT_MODE) {
            for (int i = 0; i < 2; ++i) {
                if (HoverBox(menuCX - MENU_BOX_HALF_W, menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y - MENU_BOX_OFFSET_Y, MENU_BOX_HALF_W * 2, MENU_BOX_H)) {
                    if (mouseMoved && m_modeCursor != i) {
                        m_modeCursor = i;
                        ProceduralAudio::GetInstance().PlayPowerSE(2);
                    }
                    if (mClick) { m_modeCursor = i; spaceTrg = true; }
                }
            }
            if (upTrg || downTrg) {
                m_modeCursor = (m_modeCursor == 0) ? 1 : 0;
                ProceduralAudio::GetInstance().PlayPowerSE(2);
            }

            if (bTrg || isBackBtnClicked) {
                ProceduralAudio::GetInstance().PlayErrorSE();
                m_state = MenuState::SELECT_PLAYERS;
                m_frameCount = 0;
            }
            else if (spaceTrg) {
                ProceduralAudio::GetInstance().PlayPowerSE(9);
                if (m_modeCursor == 0) m_state = MenuState::SELECT_CLASSIC_STOCKS;
                else m_state = MenuState::SELECT_SCORE;
                m_frameCount = 0;
            }
        }
        // --- [4a] 残機設定（クラシックモード専用）---
        else if (m_state == MenuState::SELECT_CLASSIC_STOCKS) {
            for (int i = 0; i < 3; ++i) {
                if (HoverBox(menuCX - MENU_BOX_HALF_W, menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y - MENU_BOX_OFFSET_Y, MENU_BOX_HALF_W * 2, MENU_BOX_H)) {
                    if (mouseMoved && m_stocksCursor != i) {
                        m_stocksCursor = i;
                        ProceduralAudio::GetInstance().PlayPowerSE(2);
                    }
                    if (mClick) { m_stocksCursor = i; spaceTrg = true; }
                }
            }
            if (upTrg) { m_stocksCursor--; if (m_stocksCursor < 0) m_stocksCursor = 2; ProceduralAudio::GetInstance().PlayPowerSE(2); }
            if (downTrg) { m_stocksCursor++; if (m_stocksCursor > 2) m_stocksCursor = 0; ProceduralAudio::GetInstance().PlayPowerSE(2); }

            if (bTrg || isBackBtnClicked) {
                ProceduralAudio::GetInstance().PlayErrorSE();
                m_state = MenuState::SELECT_MODE;
                m_frameCount = 0;
            }
            else if (spaceTrg) {
                ProceduralAudio::GetInstance().PlayPowerSE(9);
                m_state = MenuState::SELECT_P1_TYPE;
                m_frameCount = 0;
            }
        }
        // --- [4b] 目標スコア設定（ゼロワンモード専用）---
        else if (m_state == MenuState::SELECT_SCORE) {
            for (int i = 0; i < 3; ++i) {
                if (HoverBox(menuCX - MENU_BOX_HALF_W, menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y - MENU_BOX_OFFSET_Y, MENU_BOX_HALF_W * 2, MENU_BOX_H)) {
                    if (mouseMoved && m_scoreCursor != i) {
                        m_scoreCursor = i;
                        ProceduralAudio::GetInstance().PlayPowerSE(2);
                    }
                    if (mClick) { m_scoreCursor = i; spaceTrg = true; }
                }
            }
            if (upTrg) { m_scoreCursor--; if (m_scoreCursor < 0) m_scoreCursor = 2; ProceduralAudio::GetInstance().PlayPowerSE(2); }
            if (downTrg) { m_scoreCursor++; if (m_scoreCursor > 2) m_scoreCursor = 0; ProceduralAudio::GetInstance().PlayPowerSE(2); }

            if (bTrg || isBackBtnClicked) {
                ProceduralAudio::GetInstance().PlayErrorSE();
                m_state = MenuState::SELECT_MODE;
                m_frameCount = 0;
            }
            else if (spaceTrg) {
                ProceduralAudio::GetInstance().PlayPowerSE(9);
                m_state = MenuState::SELECT_P1_TYPE;
                m_frameCount = 0;
            }
        }
        // --- [5] 1P操作設定（プレイヤー / NPC）---
        else if (m_state == MenuState::SELECT_P1_TYPE) {
            for (int i = 0; i < 2; ++i) {
                if (HoverBox(menuCX - MENU_BOX_HALF_W, menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y - MENU_BOX_OFFSET_Y, MENU_BOX_HALF_W * 2, MENU_BOX_H)) {
                    if (mouseMoved && m_players[0].typeCursor != i) {
                        m_players[0].typeCursor = i;
                        ProceduralAudio::GetInstance().PlayPowerSE(2);
                    }
                    if (mClick) { m_players[0].typeCursor = i; spaceTrg = true; }
                }
            }
            if (upTrg || downTrg) {
                m_players[0].typeCursor = (m_players[0].typeCursor == 0) ? 1 : 0;
                ProceduralAudio::GetInstance().PlayPowerSE(2);
            }

            if (bTrg || isBackBtnClicked) {
                ProceduralAudio::GetInstance().PlayErrorSE();
                if (m_modeCursor == 0) m_state = MenuState::SELECT_CLASSIC_STOCKS;
                else m_state = MenuState::SELECT_SCORE;
                m_frameCount = 0;
            }
            else if (spaceTrg) {
                ProceduralAudio::GetInstance().PlayPowerSE(9);
                m_state = MenuState::SELECT_P2_TYPE;
                m_frameCount = 0;
            }
        }
        // --- [6] 2P操作設定（プレイヤー / NPC）---
        else if (m_state == MenuState::SELECT_P2_TYPE) {
            for (int i = 0; i < 2; ++i) {
                if (HoverBox(menuCX - MENU_BOX_HALF_W, menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y - MENU_BOX_OFFSET_Y, MENU_BOX_HALF_W * 2, MENU_BOX_H)) {
                    if (mouseMoved && m_players[1].typeCursor != i) {
                        m_players[1].typeCursor = i;
                        ProceduralAudio::GetInstance().PlayPowerSE(2);
                    }
                    if (mClick) { m_players[1].typeCursor = i; spaceTrg = true; }
                }
            }
            if (upTrg || downTrg) {
                m_players[1].typeCursor = (m_players[1].typeCursor == 0) ? 1 : 0;
                ProceduralAudio::GetInstance().PlayPowerSE(2);
            }

            if (bTrg || isBackBtnClicked) {
                ProceduralAudio::GetInstance().PlayErrorSE();
                m_state = MenuState::SELECT_P1_TYPE;
                m_frameCount = 0;
            }
            else if (spaceTrg) {
                ProceduralAudio::GetInstance().PlayPowerSE(9);
                m_state = MenuState::SELECT_STAGE;
                m_frameCount = 0;
            }
        }
        // --- [7] ステージ選択（バランス / マイナス / カオス）---
        else if (m_state == MenuState::SELECT_STAGE) {
            for (int i = 0; i < 3; ++i) {
                if (HoverBox(menuCX - MENU_BOX_HALF_W, menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y - MENU_BOX_OFFSET_Y, MENU_BOX_HALF_W * 2, MENU_BOX_H)) {
                    if (mouseMoved && m_stageCursor != i) {
                        m_stageCursor = i;
                        ProceduralAudio::GetInstance().PlayPowerSE(2);
                    }
                    if (mClick) { m_stageCursor = i; spaceTrg = true; }
                }
            }
            if (upTrg) { m_stageCursor--; if (m_stageCursor < 0) m_stageCursor = 2; ProceduralAudio::GetInstance().PlayPowerSE(2); }
            if (downTrg) { m_stageCursor++; if (m_stageCursor > 2) m_stageCursor = 0; ProceduralAudio::GetInstance().PlayPowerSE(2); }

            if (bTrg || isBackBtnClicked) {
                ProceduralAudio::GetInstance().PlayErrorSE();
                m_state = MenuState::SELECT_P2_TYPE;
                m_frameCount = 0;
            }
            else if (spaceTrg) {
                ProceduralAudio::GetInstance().PlayPowerSE(9);
                m_state = MenuState::CUSTOM_P1_START;
                m_frameCount = 0;
            }
        }
        // --- [8] カスタム設定（初期体力・座標）---
        else if (isCustomState) {
            bool is1P = (m_state == MenuState::CUSTOM_P1_START);
            int pIdx = is1P ? 0 : 1;
            auto& p = m_players[pIdx];

            // 4項目のカーソル移動・クリック処理
            for (int i = 0; i < 4; ++i) {
                int iy = menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y;
                int valX = menuCX + CUSTOM_BTN_BASE_X;

                // 項目全体のホバー・クリック
                if (HoverBox(menuCX - MENU_BOX_HALF_W, iy - MENU_BOX_OFFSET_Y, MENU_BOX_HALF_W * 2, MENU_BOX_H)) {
                    if (mouseMoved && p.customCursor != i) {
                        p.customCursor = i;
                        ProceduralAudio::GetInstance().PlayPowerSE(2);
                    }
                    if (mClick) {
                        p.customCursor = i;
                        if (i == 3) spaceTrg = true;  // 最後の項目は決定ボタン
                    }
                }

                // 増減ボタンのクリック処理（最初の3項目のみ）
                if (i < 3 && mClick) {
                    if (HoverBox(valX - CUSTOM_BTN_OFFSET_X, iy - 5, CUSTOM_BTN_SIZE, CUSTOM_BTN_SIZE)) {
                        p.customCursor = i;
                        leftTrg = true;
                    }
                    else if (HoverBox(valX + CUSTOM_BTN_OFFSET_X, iy - 5, CUSTOM_BTN_SIZE, CUSTOM_BTN_SIZE)) {
                        p.customCursor = i;
                        rightTrg = true;
                    }
                }
            }

            // 数値直接入力（1~9キー）
            if (hitNum != -1 && p.customCursor < 3) {
                ProceduralAudio::GetInstance().PlayPowerSE(hitNum); // 入力した数字の音程がそのまま鳴る！
                if (p.customCursor == 0) p.startNum = hitNum;
                else if (p.customCursor == 1) p.startX = hitNum;
                else if (p.customCursor == 2) p.startY = hitNum;
                p.customCursor++;  // 次の項目へ自動移動
            }

            // カーソル移動（上下キー）
            if (upTrg) { p.customCursor--; if (p.customCursor < 0) p.customCursor = 0; ProceduralAudio::GetInstance().PlayPowerSE(2); }
            if (downTrg) { p.customCursor++; if (p.customCursor > 3) p.customCursor = 3; ProceduralAudio::GetInstance().PlayPowerSE(2); }

            // 数値の増減（左右キー）
            if (p.customCursor < 3) {
                if (rightTrg) {
                    ProceduralAudio::GetInstance().PlayPowerSE(5); // 値変更音
                    if (p.customCursor == 0) { p.startNum++; if (p.startNum > GRID_SIZE) p.startNum = 1; }
                    if (p.customCursor == 1) { p.startX++; if (p.startX > GRID_SIZE) p.startX = 1; }
                    if (p.customCursor == 2) { p.startY++; if (p.startY > GRID_SIZE) p.startY = 1; }
                }
                if (leftTrg) {
                    ProceduralAudio::GetInstance().PlayPowerSE(5); // 値変更音
                    if (p.customCursor == 0) { p.startNum--; if (p.startNum < 1) p.startNum = GRID_SIZE; }
                    if (p.customCursor == 1) { p.startX--; if (p.startX < 1) p.startX = GRID_SIZE; }
                    if (p.customCursor == 2) { p.startY--; if (p.startY < 1) p.startY = GRID_SIZE; }
                }
            }

            // 戻る処理
            if (bTrg || isBackBtnClicked) {
                ProceduralAudio::GetInstance().PlayErrorSE(); // キャンセル音
                if (p.customCursor > 0) p.customCursor--;  // カーソルを1つ戻す
                else if (is1P) {
                    m_state = MenuState::SELECT_STAGE;
                    m_frameCount = 0;
                }
                else {
                    m_state = MenuState::CUSTOM_P1_START;
                    m_players[0].customCursor = 3;
                    m_frameCount = 0;
                }
            }

            // 決定処理
            if (spaceTrg) {
                ProceduralAudio::GetInstance().PlayPowerSE(9); // 決定音
                if (p.customCursor < 3) {
                    p.customCursor++;  // 次の項目へ
                }
                else {
                    if (is1P) {
                        // 1P設定完了 → 2P設定へ
                        m_state = MenuState::CUSTOM_P2_START;
                        m_players[1].customCursor = 0;
                        m_frameCount = 0;
                    }
                    else {
                        // 2P設定完了 → ゲーム開始
                        auto* sm = SceneManager::GetInstance();

                        // ゲーム設定を登録
                        if (m_modeCursor == 0) {
                            // クラシックモード: 残機数を設定
                            int maxStocks = (m_stocksCursor == 0) ? 1 : (m_stocksCursor == 1) ? 3 : 5;
                            sm->SetGameSettings(m_playerCursor, m_modeCursor, maxStocks);
                        }
                        else {
                            // ゼロワンモード: 目標スコアを設定
                            int finalScore = TARGET_SCORES[m_scoreCursor];
                            sm->SetGameSettings(m_playerCursor, m_modeCursor, finalScore);
                        }

                        // プレイヤー設定を登録（座標をグリッド座標に変換）
                        sm->SetPlayer1Settings(
                            m_players[0].typeCursor == 1,      // NPCかどうか
                            m_players[0].startNum,             // 初期体力
                            m_players[0].startX - 1,           // X座標（0始まりに変換）
                            GRID_SIZE - m_players[0].startY    // Y座標（上下反転）
                        );
                        sm->SetPlayer2Settings(
                            m_players[1].typeCursor == 1,
                            m_players[1].startNum,
                            m_players[1].startX - 1,
                            GRID_SIZE - m_players[1].startY
                        );

                        // ステージ設定
                        sm->SetStageIndex(m_stageCursor);

                        // ゲーム画面へ遷移
                        sm->ChangeScene(SceneManager::SCENE_ID::GAME);
                    }
                }
            }
        }
    }
    // ==========================================
    // 描画処理: タイトル画面のUI表示
    // 背景・タイトルロゴ・メニュー・ガイドを描画
    // ==========================================
    void TitleScene::Draw() {
        int sw, sh;
        GetDrawScreenSize(&sw, &sh);
        int CX = sw / 2;  // 画面中央X
        int CY = sh / 2;  // 画面中央Y

        auto& input = InputManager::GetInstance();
        Vector2 m = input.GetMousePos();

        // 矩形内判定用ラムダ関数（マウスホバー判定）
        auto HoverBox = [&](int x, int y, int w, int h) {
            return (m.x >= x && m.x <= x + w && m.y >= y && m.y <= y + h);
            };

        // ==========================================
        // 1. 背景描画
        // ==========================================
        DrawBox(0, 0, sw, sh, COL_BG(), TRUE);

        // HLSLシェーダーによるサイバーグリッドエフェクト
        if (m_psHandle != -1 && m_cbHandle != -1) {
            // 時間と解像度をシェーダーに渡してスクリーン空間エフェクトを生成
            float* cb = (float*)GetBufferShaderConstantBuffer(m_cbHandle);
            cb[0] = m_shaderTime;  // 時間（アニメーション）
            cb[1] = (float)sw;     // 画面幅
            cb[2] = (float)sh;     // 画面高さ
            cb[3] = 0.0f;          // 予備
            UpdateShaderConstantBuffer(m_cbHandle);
            SetShaderConstantBuffer(m_cbHandle, DX_SHADERTYPE_PIXEL, 0);

            SetUsePixelShader(m_psHandle);

            // 画面全体を覆う2つの三角形を描画
            VERTEX2DSHADER v[6];
            for (int i = 0; i < 6; ++i) {
                v[i].pos = VGet(0, 0, 0);
                v[i].rhw = 1.0f;
                v[i].dif = GetColorU8(255, 255, 255, 255);
                v[i].spc = GetColorU8(0, 0, 0, 0);
                v[i].u = 0.0f; v[i].v = 0.0f;
            }
            // 頂点座標とUV座標を設定
            v[0].pos.x = 0;   v[0].pos.y = 0;   v[0].u = 0.0f; v[0].v = 0.0f;
            v[1].pos.x = sw;  v[1].pos.y = 0;   v[1].u = 1.0f; v[1].v = 0.0f;
            v[2].pos.x = 0;   v[2].pos.y = sh;  v[2].u = 0.0f; v[2].v = 1.0f;
            v[3].pos.x = sw;  v[3].pos.y = 0;   v[3].u = 1.0f; v[3].v = 0.0f;
            v[4].pos.x = sw;  v[4].pos.y = sh;  v[4].u = 1.0f; v[4].v = 1.0f;
            v[5].pos.x = 0;   v[5].pos.y = sh;  v[5].u = 0.0f; v[5].v = 1.0f;

            DrawPrimitive2DToShader(v, 6, DX_PRIMTYPE_TRIANGLELIST);
            SetUsePixelShader(-1);
        }

        // グリッド線描画（半透明）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 30);
        for (int i = 0; i < sw; i += MAP_CELL_SIZE)
            DrawLine(i, 0, i, sh, COL_GRID(), 1);
        for (int j = 0; j < sh; j += MAP_CELL_SIZE)
            DrawLine(0, j, sw, j, COL_GRID(), 1);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 上下の装飾線
        DrawBox(0, 40, sw, 45, COL_P1(), TRUE);
        DrawBox(0, sh - 45, sw, sh - 40, COL_P1(), TRUE);

        // ==========================================
        // 2. タイトルロゴ（豪華エフェクト付き）
        // ==========================================
        const char* titleText = "超計算マスBATTLE";
        int titleW = GetDrawStringWidthToHandle(titleText, (int)strlen(titleText), m_fontTitle);

        // アニメーション用パラメータ
        double t = GetNowCount() / 1000.0;
        float floatY = (float)sin(t * 2.0) * 8.0f;  // 上下浮遊
        int titleX = CX - titleW / 2;
        int titleY = 100 + (int)floatY;

        // オーラエフェクト（加算合成で発光）
        SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
        for (int i = 0; i < 4; ++i) {
            int offset = i * 2;
            DrawStringToHandle(titleX - offset, titleY - offset, titleText, COL_TITLE_SUB(), m_fontTitle);
            DrawStringToHandle(titleX + offset, titleY + offset, titleText, COL_TITLE_SUB(), m_fontTitle);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // グリッチエフェクト（RGBずらし）
        float glitchStrength = (float)sin(t * 10.0) * 3.0f;
        SetDrawBlendMode(DX_BLENDMODE_ADD, 150);
        DrawStringToHandle(titleX + (int)glitchStrength, titleY, titleText, GetColor(255, 0, 100), m_fontTitle);
        DrawStringToHandle(titleX - (int)glitchStrength, titleY, titleText, GetColor(0, 100, 255), m_fontTitle);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // メインテキスト
        DrawStringToHandle(titleX, titleY, titleText, COL_TITLE_MAIN(), m_fontTitle);

        // スキャンラインエフェクト（光が横切る演出）
        float scanPos = (float)fmod(t * 1.5, 2.0) - 1.0f;
        int shineX = titleX + (int)(scanPos * titleW * 1.5f);

        SetDrawArea(titleX, titleY, titleX + titleW, titleY + 110);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 180);
        for (int i = 0; i < 20; ++i) {
            DrawLine(shineX + i, titleY, shineX + i - 30, titleY + 100, GetColor(255, 255, 255));
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        SetDrawArea(0, 0, sw, sh);

        // タイトル下の装飾線
        DrawLine(CX - 500, 230, CX + 500, 230, COL_TITLE_SUB(), 5);
        SetDrawBlendMode(DX_BLENDMODE_ADD, 200);
        DrawLine(CX - 500, 230, CX + 500, 230, COL_TITLE_MAIN(), 2);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // ==========================================
        // 3. 描画ヘルパー関数の定義
        // ==========================================
        double time = GetNowCount() / 1000.0;
        int blinkAlpha = (int)(BLINK_BASE_ALPHA + BLINK_AMP_ALPHA * sin(time * M_PI * BLINK_SPEED));

        // ブラケット装飾（四隅のL字）
        auto drawBracket = [&](int x, int y, int w, int h, unsigned int col) {
            int d = 15;  // ブラケットのサイズ
            // 左上
            DrawLine(x, y, x + d, y, col, 2);
            DrawLine(x, y, x, y + d, col, 2);
            // 右上
            DrawLine(x + w - d, y, x + w, y, col, 2);
            DrawLine(x + w, y, x + w, y + d, col, 2);
            // 左下
            DrawLine(x, y + h - d, x, y + h, col, 2);
            DrawLine(x, y + h, x + d, y + h, col, 2);
            // 右下
            DrawLine(x + w - d, y + h, x + w, y + h, col, 2);
            DrawLine(x + w, y + h - d, x + w, y + h, col, 2);
            };

        // レイアウト計算
        bool isCustomState = (m_state == MenuState::CUSTOM_P1_START || m_state == MenuState::CUSTOM_P2_START);
        int menuCX = isCustomState ? CX + MENU_CUSTOM_OFFSET_X : CX;
        int menuStartY = CY + MENU_CENTER_OFFSET_Y;

        // シンプルメニュー描画（2~3択のメニュー用）
        auto drawSimpleMenu = [&](int cursor, const char* menuTitle, const char* m0, const char* m1, const char* m2 = nullptr) {
            // タイトル表示
            int mtW = GetDrawStringWidthToHandle(menuTitle, (int)strlen(menuTitle), m_fontMenu);
            DrawStringToHandle(menuCX - mtW / 2, menuStartY, menuTitle, COL_GRID(), m_fontMenu);

            // 各項目の表示
            const char* items[3] = { m0, m1, m2 };
            for (int i = 0; i < (m2 ? 3 : 2); ++i) {
                int iy = menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y;
                unsigned int baseCol = (cursor == i) ? COL_TEXT_ON() : COL_TEXT_OFF();
                int tw = GetDrawStringWidthToHandle(items[i], (int)strlen(items[i]), m_fontMenu);

                // 選択中の項目は背景とブラケットを描画
                if (cursor == i) {
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, blinkAlpha / 3);
                    DrawBox(menuCX - MENU_BOX_HALF_W, iy - MENU_BOX_OFFSET_Y,
                        menuCX + MENU_BOX_HALF_W, iy + MENU_BOX_H - MENU_BOX_OFFSET_Y, baseCol, TRUE);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                    drawBracket(menuCX - MENU_BOX_HALF_W, iy - 15, MENU_BOX_HALF_W * 2, 70, baseCol);
                    DrawStringToHandle(menuCX - 380, iy, " >>", baseCol, m_fontMenu);
                    DrawStringToHandle(menuCX + 335, iy, "<< ", baseCol, m_fontMenu);
                }

                // 項目テキスト（中央揃え）
                DrawStringToHandle(menuCX - tw / 2, iy, items[i], baseCol, m_fontMenu);
            }
            };

        // ==========================================
        // 4. ステートに応じたメニュー描画
        // ==========================================

        // [1] スタート画面: 点滅するプロンプト
        if (m_state == MenuState::PRESS_START) {
            const char* pushText = "スペースかクリックでスタート";
            int ptW = GetDrawStringWidthToHandle(pushText, (int)strlen(pushText), m_fontMenu);

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, blinkAlpha);
            DrawStringToHandle(CX - ptW / 2, CY + 100, pushText, COL_GRID(), m_fontMenu);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
        // [2] プレイヤー数選択
        else if (m_state == MenuState::SELECT_PLAYERS) {
            drawSimpleMenu(m_playerCursor - 1, "【 バトル方式 】", "シングルバトル", "オフラインバトル");
        }
        // [3] ゲームモード選択
        else if (m_state == MenuState::SELECT_MODE) {
            drawSimpleMenu(m_modeCursor, "【 プレイモード 】", "ノーマルバトル", "カウントバトル");

            // モード説明文の描画
            const char* modeDesc = nullptr;
            if (m_modeCursor == 0) {
                modeDesc = "相手のバッテリーを削り切れ！四則演算を用いた王道ダメージバトル";
            }
            else {
                modeDesc = "目標スコアへピタリと着地しろ！極限の頭脳戦モード";
            }

            if (modeDesc != nullptr) {
                int descW = GetDrawStringWidthToHandle(modeDesc, (int)strlen(modeDesc), m_fontSmall);
                int descX = menuCX - descW / 2;
                int descY = menuStartY + MENU_ITEM_BASE_Y + 2 * MENU_ITEM_STEP_Y + 40;

                // 説明文の背景
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
                DrawBox(descX - 20, descY - 10, descX + descW + 20, descY + 40,
                    COL_BG(), TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

                // 説明文の枠線
                DrawBox(descX - 20, descY - 10, descX + descW + 20, descY + 40,
                    m_modeCursor == 0 ? COL_P1() : COL_P2(), FALSE);

                // 説明文テキスト
                DrawStringToHandle(descX, descY, modeDesc, COL_TEXT_ON(), m_fontSmall);
            }
        }
        // [4a] 残機設定
        else if (m_state == MenuState::SELECT_CLASSIC_STOCKS) {
            drawSimpleMenu(m_stocksCursor, "【 残機設定 】", "残機: 1", "残機: 3", "残機: 5");
        }
        // [4b] 目標スコア設定
        else if (m_state == MenuState::SELECT_SCORE) {
            drawSimpleMenu(m_scoreCursor, "【 目標スコア設定 】", "目標スコア: 53", "目標スコア: 103", "目標スコア: 223");
        }
        // [5] 1P操作設定
        else if (m_state == MenuState::SELECT_P1_TYPE) {
            drawSimpleMenu(m_players[0].typeCursor, "【 1P 操作設定 】", "プレイヤー", "NPC");
        }
        // [6] 2P操作設定
        else if (m_state == MenuState::SELECT_P2_TYPE) {
            drawSimpleMenu(m_players[1].typeCursor, "【 2P 操作設定 】", "プレイヤー", "NPC");
        }
        // [7] ステージ選択
        else if (m_state == MenuState::SELECT_STAGE) {
            drawSimpleMenu(m_stageCursor, "【 ステージ選択 】", "バランステージ", "マイナステージ", "カオステージ");

            // ==========================================
            // おすすめ情報の表示
            // ==========================================
            const char* recommendText = nullptr;
            unsigned int recommendCol = COL_TEXT_OFF();

            // ノーマルバトルの場合
            if (m_modeCursor == 0) {
                if (m_stageCursor == 1) {
                    // マイナステージが選択されている場合
                    recommendText = "おすすめ！";
                    recommendCol = GetColor(255, 215, 0); // ゴールド
                }
                else {
                    // それ以外のステージの場合はヒント表示
                    recommendText = "（マイナステージがおすすめ）";
                    recommendCol = GetColor(150, 150, 180);
                }
            }
            // カウントバトルの場合
            else {
                recommendText = "すべておすすめ！";
                recommendCol = GetColor(100, 255, 150); // 明るい緑
            }

            // おすすめテキストを描画（メニューの右側）
            if (recommendText != nullptr) {
                int recW = GetDrawStringWidthToHandle(recommendText, (int)strlen(recommendText), m_fontSmall);
                int recX = menuCX + 420;
                int recY = menuStartY + 50;

                // 選択中のステージに合わせておすすめマークを移動（ノーマルバトル時のみ）
                if (m_modeCursor == 0 && m_stageCursor == 1) {
                    recY = menuStartY + MENU_ITEM_BASE_Y + m_stageCursor * MENU_ITEM_STEP_Y + 15;
                }

                DrawStringToHandle(recX, recY, recommendText, recommendCol, m_fontSmall);
            }
        }
        // [8] カスタム設定画面
        else if (isCustomState) {
            bool is1P = (m_state == MenuState::CUSTOM_P1_START);
            int pIdx = is1P ? 0 : 1;
            auto& p = m_players[pIdx];

            // タイトル表示
            std::string menuTitle = is1P ? "【 1P 初期設定 】" : "【 2P 初期設定 】";
            int mtW = GetDrawStringWidthToHandle(menuTitle.c_str(), (int)menuTitle.length(), m_fontMenu);
            DrawStringToHandle(menuCX - mtW / 2, menuStartY, menuTitle.c_str(),
                is1P ? COL_P1() : COL_P2(), m_fontMenu);

            const char* items[3] = { "初期体力", "初期Ｘ座標 (1-9)", "初期Ｙ座標 (1-9)" };
            int vals[3] = { p.startNum, p.startX, p.startY };

            // 4項目（体力・X・Y・決定ボタン）の描画
            for (int i = 0; i < 4; ++i) {
                int iy = menuStartY + MENU_ITEM_BASE_Y + i * MENU_ITEM_STEP_Y;
                unsigned int color = (p.customCursor == i) ? COL_TEXT_ON() : COL_TEXT_OFF();

                // 選択中の項目は背景とブラケットを描画
                if (p.customCursor == i) {
                    SetDrawBlendMode(DX_BLENDMODE_ALPHA, blinkAlpha / 3);
                    DrawBox(menuCX - MENU_BOX_HALF_W, iy - MENU_BOX_OFFSET_Y,
                        menuCX + MENU_BOX_HALF_W, iy + MENU_BOX_H - MENU_BOX_OFFSET_Y, color, TRUE);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                    drawBracket(menuCX - MENU_BOX_HALF_W, iy - 15, MENU_BOX_HALF_W * 2, 70, color);
                }

                if (i < 3) {
                    // 最初の3項目: 数値設定項目
                    char nameBuf[128];
                    sprintf_s(nameBuf, "%-20s", items[i]);
                    DrawStringToHandle(menuCX - 320, iy, nameBuf, color, m_fontMenu);

                    int valX = menuCX + CUSTOM_BTN_BASE_X;

                    // 左ボタン（減）
                    bool hoverL = HoverBox(valX - CUSTOM_BTN_OFFSET_X, iy - 5, CUSTOM_BTN_SIZE, CUSTOM_BTN_SIZE);
                    unsigned int colL = hoverL ? COL_WHITE() : color;
                    DrawBox(valX - CUSTOM_BTN_OFFSET_X, iy - 5, valX - 10, iy + 45, colL, FALSE);
                    DrawStringToHandle(valX - 45, iy, "<", colL, m_fontMenu);

                    // 現在値表示
                    DrawFormatStringToHandle(valX + 15, iy - 4, color, m_fontNumber, "%d", vals[i]);

                    // 右ボタン（増）
                    bool hoverR = HoverBox(valX + CUSTOM_BTN_OFFSET_X, iy - 5, CUSTOM_BTN_SIZE, CUSTOM_BTN_SIZE);
                    unsigned int colR = hoverR ? COL_WHITE() : color;
                    DrawBox(valX + CUSTOM_BTN_OFFSET_X, iy - 5, valX + 110, iy + 45, colR, FALSE);
                    DrawStringToHandle(valX + 75, iy, ">", colR, m_fontMenu);
                }
                else {
                    // 4項目目: 決定ボタン
                    const char* decideText = is1P ? "設定完了 (NEXT)" : "バトル開始 (GAME START)";
                    int tw = GetDrawStringWidthToHandle(decideText, (int)strlen(decideText), m_fontMenu);
                    DrawStringToHandle(menuCX - tw / 2, iy, decideText, color, m_fontMenu);

                    if (p.customCursor == i) {
                        DrawStringToHandle(menuCX - tw / 2 - 60, iy, " >>", color, m_fontMenu);
                        DrawStringToHandle(menuCX + tw / 2 + 20, iy, "<< ", color, m_fontMenu);
                    }
                }
            }

            // ==========================================
            // ミニマップ描画（配置プレビュー）
            // ==========================================
            int mapBaseX = CX + MAP_BASE_OFFSET_X;
            int mapBaseY = menuStartY + MAP_BASE_OFFSET_Y;

            DrawStringToHandle(mapBaseX + 80, mapBaseY - 50, "【 配置プレビュー 】",
                COL_TITLE_MAIN(), m_fontSmall);

            // マップ枠
            DrawBox(mapBaseX - 5, mapBaseY - 5,
                mapBaseX + MAP_CELL_SIZE * GRID_SIZE + 5,
                mapBaseY + MAP_CELL_SIZE * GRID_SIZE + 5, COL_GRID(), FALSE);

            // 軸ラベル
            DrawStringToHandle(mapBaseX - 40, mapBaseY - 40, "Y", COL_GRID(), m_fontSmall);
            DrawStringToHandle(mapBaseX + MAP_CELL_SIZE * GRID_SIZE + 15,
                mapBaseY + MAP_CELL_SIZE * GRID_SIZE - 10, "X", COL_GRID(), m_fontSmall);

            // 9x9グリッドの描画
            for (int y = 0; y < GRID_SIZE; ++y) {
                for (int x = 0; x < GRID_SIZE; ++x) {
                    int drawX = mapBaseX + x * MAP_CELL_SIZE;
                    int drawY = mapBaseY + y * MAP_CELL_SIZE;
                    int uiX = x + 1;           // UI座標（1始まり）
                    int uiY = GRID_SIZE - y;   // Y軸反転

                    // マス目の背景と枠線
                    DrawBox(drawX, drawY, drawX + MAP_CELL_SIZE, drawY + MAP_CELL_SIZE, COL_BG(), TRUE);
                    DrawBox(drawX, drawY, drawX + MAP_CELL_SIZE, drawY + MAP_CELL_SIZE, COL_TITLE_SUB(), FALSE);

                    // 1Pの初期位置
                    if (uiX == m_players[0].startX && uiY == m_players[0].startY) {
                        int alpha = (is1P) ? blinkAlpha : 150;  // 設定中のプレイヤーは点滅
                        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
                        DrawBox(drawX + 2, drawY + 2, drawX + MAP_CELL_SIZE - 2,
                            drawY + MAP_CELL_SIZE - 2, COL_P1(), TRUE);
                        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                        DrawFormatStringToHandle(drawX + 16, drawY + 10, COL_BLACK(),
                            m_fontSmall, "%d", m_players[0].startNum);
                    }

                    // 2Pの初期位置
                    if (uiX == m_players[1].startX && uiY == m_players[1].startY) {
                        int alpha = (!is1P) ? blinkAlpha : 150;
                        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
                        DrawBox(drawX + 2, drawY + 2, drawX + MAP_CELL_SIZE - 2,
                            drawY + MAP_CELL_SIZE - 2, COL_P2(), TRUE);
                        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                        DrawFormatStringToHandle(drawX + 16, drawY + 10, COL_BLACK(),
                            m_fontSmall, "%d", m_players[1].startNum);
                    }
                }
            }
        }

        // ==========================================
        // 5. 戻るボタン（スタート画面以外で表示）
        // ==========================================
        if (m_state != MenuState::PRESS_START) {
            int backBtnX = CX - 150;
            int backBtnY = sh - 150;
            int backBtnW = 300;
            int backBtnH = 60;
            bool isHover = HoverBox(backBtnX, backBtnY, backBtnW, backBtnH);
            unsigned int btnCol = isHover ? COL_TEXT_ON() : COL_TEXT_OFF();

            // ボタン背景（半透明）
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, isHover ? 200 : 120);
            DrawBox(backBtnX, backBtnY, backBtnX + backBtnW, backBtnY + backBtnH, COL_BG(), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            // ボタン枠線
            DrawBox(backBtnX, backBtnY, backBtnX + backBtnW, backBtnY + backBtnH, btnCol, FALSE);

            // ボタンテキスト
            const char* backStr = "戻る (B)";
            int bw = GetDrawStringWidthToHandle(backStr, (int)strlen(backStr), m_fontMenu);
            DrawStringToHandle(backBtnX + (backBtnW - bw) / 2, backBtnY + 10, backStr, btnCol, m_fontMenu);


        }

        // ==========================================
        // 6. 画面下部のオペレーションガイド
        // ==========================================
        DrawBox(0, sh - 70, sw, sh - 15, GetColor(10, 20, 40), TRUE);

        // ステートに応じたガイドテキスト
        std::string guideText;
        if (m_state == MenuState::PRESS_START) {
            guideText = "";
        }
        else if (m_state == MenuState::SELECT_PLAYERS) {
            guideText = "[↑][↓]/CLICK: 項目選択   |   [SPACE]: 決定";
        }
        else if (isCustomState) {
            guideText = "[↑][↓]/CLICK: 項目選択   |   [<][>]/CLICK: 数値変更   |   [1-9]: 直接入力";
        }
        else {
            guideText = "[↑][↓]/CLICK: 項目選択   |   [SPACE]: 決定";
        }

        if (!guideText.empty()) {
            int gw = GetDrawStringWidthToHandle(guideText.c_str(), (int)guideText.length(), m_fontSmall);
            int guideX = isCustomState ? (CX - gw / 2 - 100) : (CX - gw / 2);
            DrawStringToHandle(guideX, sh - 55, guideText.c_str(), COL_TEXT_OFF(), m_fontSmall);
        }

        // ==========================================
        // 7. 右上のEXIT（終了）ボタン
        // ==========================================
        int exitBtnW = 160;
        int exitBtnH = 50;
        int exitBtnX = sw - exitBtnW - 20;
        int exitBtnY = 20;
        bool isExitHover = HoverBox(exitBtnX, exitBtnY, exitBtnW, exitBtnH);
        unsigned int exitCol = isExitHover ? COL_DANGER() : COL_TEXT_OFF();

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, isExitHover ? 200 : 120);
        DrawBox(exitBtnX, exitBtnY, exitBtnX + exitBtnW, exitBtnY + exitBtnH, COL_BG(), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawBox(exitBtnX, exitBtnY, exitBtnX + exitBtnW, exitBtnY + exitBtnH, exitCol, FALSE);

        const char* exitStr = "ポーズ (ESC)";
        int twExit = GetDrawStringWidthToHandle(exitStr, (int)strlen(exitStr), m_fontSmall);
        DrawStringToHandle(exitBtnX + (exitBtnW - twExit) / 2, exitBtnY + 12, exitStr, exitCol, m_fontSmall);

        // ==========================================
        // 8. 全画面切り替えの案内（画面右下）
        // ==========================================
        const char* fullscreenGuide = "[F11] 全画面表示 / ウィンドウ切替";
        int fsGuideW = GetDrawStringWidthToHandle(fullscreenGuide, (int)strlen(fullscreenGuide), m_fontSmall);

        // ガイドテキストの右端に少し余白（20px）を持たせて描画
        DrawStringToHandle(sw - fsGuideW - 20, sh - 55, fullscreenGuide, GetColor(150, 150, 180), m_fontSmall);
    }

    // ==========================================
    // 解放処理: フォント・シェーダーのクリーンアップ
    // シーン終了時に1回だけ呼ばれる
    // ==========================================
    void TitleScene::Release() {
        if (m_fontTitle != -1)  DeleteFontToHandle(m_fontTitle);
        if (m_fontMenu != -1)   DeleteFontToHandle(m_fontMenu);
        if (m_fontSmall != -1)  DeleteFontToHandle(m_fontSmall);
        if (m_fontNumber != -1) DeleteFontToHandle(m_fontNumber);

        if (m_psHandle != -1)   DeleteShader(m_psHandle);
        if (m_cbHandle != -1)   DeleteShaderConstantBuffer(m_cbHandle);
    }

} // namespace App