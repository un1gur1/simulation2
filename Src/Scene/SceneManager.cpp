#include "SceneManager.h"
#include "GameScene.h"
#include "TitleScene.h"
#include "ResultScene.h"
#include "PauseMenu.h"
#include "../Input/InputManager.h"
#include <utility>

namespace App {
    // ==========================================
    // シングルトンインスタンス
    // ==========================================
    SceneManager* SceneManager::instance_ = nullptr;

    // ==========================================
    // コンストラクタ: 各種変数の初期化
    // ==========================================
    SceneManager::SceneManager()
        : scene_(nullptr)              // 現在のシーン
        , load_(nullptr)               // ローディング画面（将来的な拡張用）
        , fader_(nullptr)              // フェード演出（将来的な拡張用）
        , sceneId_(SCENE_ID::NONE)     // 現在のシーンID
        , nextSceneId_(SCENE_ID::NONE) // 次のシーンID
        , isChanging_(false)           // シーン切り替え中フラグ
        , isGameEnd_(false)            // ゲーム終了フラグ
        , playerCount_(1)              // プレイヤー人数（1=1P vs CPU, 2=2P対戦）
        , gameMode_(0)                 // ゲームモード（0=クラシック, 1=ゼロワン）
        , zeroOneScore_(501)           // ゼロワンモードの目標スコア
        , is1P_NPC_(false)             // 1PがCPUかどうか
        , is2P_NPC_(true)              // 2PがCPUかどうか
        , p1StartNum_(5)               // 1Pの初期数値
        , p2StartNum_(7)               // 2Pの初期数値
        , p1StartX_(1), p1StartY_(1)   // 1Pの初期座標
        , p2StartX_(7), p2StartY_(7)   // 2Pの初期座標
        , m_lastIsWin(false)           // 前回の戦闘結果（勝敗）
        , m_lastStats({ 0, 0, 0, 0, 0 }) // 前回の戦闘統計
        , isPaused_(false)             // ポーズ中フラグ
        , pauseMenu_(new PauseMenu())  // ポーズメニュー
    {
    }

    // ==========================================
    // デストラクタ: リソース解放
    // ==========================================
    SceneManager::~SceneManager() {
        if (scene_) {
            scene_->Release();
            delete scene_;
            scene_ = nullptr;
        }
        if (load_) {
            delete load_;
            load_ = nullptr;
        }
        if (fader_) {
            delete fader_;
            fader_ = nullptr;
        }
        if (pauseMenu_) {
            delete pauseMenu_;
            pauseMenu_ = nullptr;
        }
    }

    // ==========================================
    // 初期化: ゲーム開始時の設定
    // ==========================================
    void SceneManager::Init() {
        isGameEnd_ = false;
        isChanging_ = false;
        isPaused_ = false;
        sceneId_ = SCENE_ID::NONE;
        nextSceneId_ = SCENE_ID::NONE;

        // デフォルトのゲーム設定
        playerCount_ = 1;          // 1人プレイ
        gameMode_ = 0;             // クラシックモード
        zeroOneScore_ = 501;       // ゼロワンの目標スコア

        // プレイヤー設定
        is1P_NPC_ = false;         // 1Pは人間
        is2P_NPC_ = true;          // 2PはCPU
        p1StartNum_ = 5;           // 初期値5（2マス移動）
        p2StartNum_ = 7;           // 初期値7（全方向移動）
        p1StartX_ = 1;             // 左上スタート
        p1StartY_ = 1;
        p2StartX_ = 7;             // 右下スタート
        p2StartY_ = 7;

        // タイトル画面へ
        ChangeScene(SCENE_ID::TITLE);
    }

    // ==========================================
    // 3D初期化: 現在は非使用（将来的な拡張用）
    // ==========================================
    void SceneManager::Init3D() {}

    // ==========================================
    // 更新処理: シーン切り替えとポーズ管理
    // ==========================================
    void SceneManager::Update() {
        // シーン切り替え処理
        if (isChanging_) PerformSceneChange();

        // ==========================================
        // ESCキーによるポーズトグル
        // ==========================================
      // ==========================================
        // ESCキー ＆ マウスによるポーズトグル
        // ==========================================
        static bool prevEsc = false;
        bool escHit = (CheckHitKey(KEY_INPUT_ESCAPE) == 1);

        // GAMEシーン中のみ、右上(1740, 10)～(1900, 60)のクリックを検知
        bool mouseHit = false;
        if (sceneId_ == SCENE_ID::GAME) {
            auto& input = InputManager::GetInstance();
            if (input.IsMouseLeftTrg()) {
                Vector2 m = input.GetMousePos();
                if (m.x >= 1740 && m.x <= 1900 && m.y >= 10 && m.y <= 60) mouseHit = true;
            }
        }

        if (escHit || mouseHit) {
            if (!prevEsc || mouseHit) {
                isPaused_ = !isPaused_;
                if (isPaused_ && pauseMenu_) pauseMenu_->Init();
            }
            prevEsc = escHit; // マウスクリック時はキー押しっぱなし防止のフラグを更新しない
        }
        else {
            prevEsc = false;
        }

        // ==========================================
        // 状態に応じた更新処理
        // ==========================================
        if (isPaused_) {
            // ポーズ中: メニュー操作のみ受け付ける
            if (pauseMenu_) {
                auto result = pauseMenu_->Update();

                if (result == PauseMenu::Result::RESUME) {
                    // 再開: ポーズ解除
                    isPaused_ = false;
                }
                else if (result == PauseMenu::Result::TITLE) {
                    // タイトルへ戻る
                    isPaused_ = false;
                    ChangeScene(SCENE_ID::TITLE);
                }
                else if (result == PauseMenu::Result::EXIT) {
                    // ゲーム終了
                    isGameEnd_ = true;
                }
            }
        }
        else {
            // 通常時: 現在のシーンを更新
            if (scene_) scene_->Update();
        }
    }

    // ==========================================
    // 描画処理: シーンとポーズメニューの表示
    // ==========================================
    void SceneManager::Draw() {
        // 1. 背面のゲーム画面を描画
        if (scene_) scene_->Draw();

        // 2. ポーズ中ならメニューを重ねて描画
        if (isPaused_ && pauseMenu_) {
            pauseMenu_->Draw();
        }
    }

    // ==========================================
    // 削除処理: 全リソースの解放
    // ==========================================
    void SceneManager::Delete() {
        if (scene_) {
            scene_->Release();
            delete scene_;
            scene_ = nullptr;
        }
        if (load_) {
            delete load_;
            load_ = nullptr;
        }
        if (fader_) {
            delete fader_;
            fader_ = nullptr;
        }

        sceneId_ = SCENE_ID::NONE;
        nextSceneId_ = SCENE_ID::NONE;
        isChanging_ = false;
    }

    // ==========================================
    // シーン切り替え要求: 次フレームで実行
    // ==========================================
    void SceneManager::ChangeScene(SCENE_ID nextId) {
        nextSceneId_ = nextId;
        isChanging_ = true;
    }

    // ==========================================
    // シーン切り替え実行: 実際のシーンオブジェクト生成
    // ==========================================
    void SceneManager::PerformSceneChange() {
        // 現在のシーンを破棄
        if (scene_) {
            scene_->Release();
            delete scene_;
            scene_ = nullptr;
        }

        // 新しいシーンを生成
        switch (nextSceneId_) {
        case SCENE_ID::NONE:
            scene_ = nullptr;
            break;
        case SCENE_ID::TITLE:
            scene_ = new TitleScene();
            break;
        case SCENE_ID::GAME:
            scene_ = new GameScene();
            break;
        case SCENE_ID::RESULT:
            scene_ = new ResultScene();
            break;
        default:
            scene_ = nullptr;
            break;
        }

        // シーンIDを更新
        sceneId_ = nextSceneId_;
        nextSceneId_ = SCENE_ID::NONE;
        isChanging_ = false;

        // シーン切り替え時はポーズを強制解除
        isPaused_ = false;

        // 新しいシーンの初期化
        if (scene_) {
            scene_->Init();        // 基本設定
            scene_->Load();        // リソース読み込み
            scene_->LoadEnd();     // 読み込み完了処理
        }
    }

} // namespace App