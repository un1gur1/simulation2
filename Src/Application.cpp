#include "Application.h"
#include <DxLib.h>
#include "Scene/SceneManager.h"
#include "Input/InputManager.h"
#include "Manager/ProceduralAudio.h"
namespace App {

    // ==========================================
    // シングルトンインスタンス
    // ゲーム全体で1つだけ存在するApplicationオブジェクト
    // ==========================================
    Application* Application::s_instance = nullptr;

    // ==========================================
    // インスタンス生成
    // ゲーム起動時に1回だけ呼ばれる
    // ==========================================
    void Application::CreateInstance() {
        if (s_instance == nullptr) {
            s_instance = new Application();
        }
    }

    // ==========================================
    // インスタンス取得
    // ゲーム内のどこからでもアクセス可能
    // ==========================================
    Application* Application::GetInstance() {
        return s_instance;
    }

    // ==========================================
    // インスタンス削除
    // ゲーム終了時に1回だけ呼ばれる
    // ==========================================
    void Application::DeleteInstance() {
        if (s_instance != nullptr) {
            delete s_instance;
            s_instance = nullptr;
        }
    }

    // ==========================================
    // コンストラクタ: 初期値設定
    // ==========================================
    Application::Application()
        : m_shouldQuit(false)           // 終了フラグ
        , m_showInfo(true)              // 情報表示フラグ（将来的な拡張用）
        , m_frameCount(0)               // フレームカウンター
        , m_windowWidth(1920)           // ウィンドウ幅（フルHD）
        , m_windowHeight(1080)          // ウィンドウ高さ（フルHD）
        , m_initFail(false)             // 初期化失敗フラグ
        , m_releaseFail(false)          // 解放失敗フラグ
    {
    }

    // ==========================================
    // デストラクタ
    // ==========================================
    Application::~Application() {
    }

    // ==========================================
    // 初期化: DXライブラリとゲームシステムの準備
    // 戻り値: 成功時true、失敗時false
    // ==========================================
    bool Application::Init() {
        m_initFail = false;

        // DXライブラリの初期化
        if (!Initialize()) {
            m_initFail = true;
            return false;
        }



        // 入力管理システムの初期化
        InputManager::GetInstance().Init();

        // シーン管理システムの初期化
        if (SceneManager::GetInstance() == nullptr) {
            SceneManager::CreateInstance();
        }
        SceneManager::GetInstance()->Init();

        // タイトル画面から開始
        SceneManager::GetInstance()->ChangeScene(SceneManager::SCENE_ID::TITLE);

        return true;
    }

    // ==========================================
    // 初期化失敗フラグの取得
    // ==========================================
    bool Application::IsInitFail() const {
        return m_initFail;
    }

    // ==========================================
    // メインループ: ゲームの心臓部
    // Update → Draw → 画面更新を繰り返す
    // ==========================================
    void Application::Run() {
        while (!m_shouldQuit) {
            // Windowsメッセージ処理
            int msg = ProcessMessage();
            if (msg == -1 || msg == 1) {
                // ウィンドウが閉じられた
                m_shouldQuit = true;
                break;
            }

            // ゲーム終了フラグチェック
            if (SceneManager::GetInstance()->GetGameEnd()) {
                m_shouldQuit = true;
                break;
            }

            Update();       // ゲームロジック更新
            Draw();         // 描画
            ScreenFlip();   // 裏画面を表画面に反映
            Sleep(1);       // CPU負荷軽減
        }
    }

    // ==========================================
    // 解放処理: リソースのクリーンアップ
    // ゲーム終了時に1回だけ呼ばれる
    // ==========================================
    void Application::Delete() {
        m_releaseFail = false;
        Shutdown();
    }

    // ==========================================
    // 解放失敗フラグの取得
    // ==========================================
    bool Application::IsReleaseFail() const {
        return m_releaseFail;
    }

    // ==========================================
    // DXライブラリの初期化: ウィンドウとグラフィックスの設定
    // 戻り値: 成功時true、失敗時false
    // ==========================================
    bool Application::Initialize() {
        m_windowWidth = 1920;
        m_windowHeight = 1080;

        // ウィンドウモードで起動
        ChangeWindowMode(TRUE);

        // ボーダーレスフルスクリーン設定（マウスカーソル消失防止）
        SetFullScreenResolutionMode(DX_FSRESOLUTIONMODE_DESKTOP);

        // グラフィックスモード設定（1920x1080, 32bit カラー）
        SetGraphMode(m_windowWidth, m_windowHeight, 32);

        // ウィンドウサイズ変更を許可
        SetWindowSizeChangeEnableFlag(TRUE, TRUE);

        // ウィンドウスタイル（枠付き）
        SetWindowStyleMode(7);

        // ウィンドウの拡大率
        SetWindowSizeExtendRate(1.0);

        // 初期ウィンドウサイズ（HD解像度）
        SetWindowSize(1280, 720);

        // ウィンドウタイトル
        SetMainWindowText("超計算マスBATTLE");

        // 画面モード変更時にグラフィックスをリセットしない
        SetChangeScreenModeGraphicsSystemResetFlag(FALSE);

        // DXライブラリの初期化
        if (DxLib_Init() == -1) return false;
        // ProceduralAudioの初期化
        ProceduralAudio::GetInstance().Init();
        // マウスカーソルを表示
        SetMouseDispFlag(TRUE);

        // 描画先を裏画面に設定（ダブルバッファリング）
        SetDrawScreen(DX_SCREEN_BACK);

        // 背景色設定: 深い紺色（ゲームのテーマカラー）
        SetBackgroundColor(5, 10, 25);

        return true;
    }

    // ==========================================
    // 終了処理: DXライブラリとシステムのシャットダウン
    // ==========================================
    void Application::Shutdown() {
        SceneManager::DeleteInstance();
        DxLib_End();
    }

    // ==========================================
    // 更新処理: 毎フレーム呼ばれる
    // 入力・画面切り替え・シーン更新を実行
    // ==========================================
    void Application::Update() {
        // 入力状態の更新（キーボード・マウス）
        InputManager::GetInstance().Update();

        // F11キーでフルスクリーン⇔ウィンドウ切り替え
        static int prevF11 = 0;
        int currentF11 = CheckHitKey(KEY_INPUT_F11);

        if (currentF11 == 1 && prevF11 == 0) {
            // キーが押された瞬間だけトグル
            if (GetWindowModeFlag() == TRUE) {
                ChangeWindowMode(FALSE);  // ウィンドウ → フルスクリーン
            }
            else {
                ChangeWindowMode(TRUE);   // フルスクリーン → ウィンドウ
            }
            SetMouseDispFlag(TRUE);  // 切り替え時にカーソルを確実に表示
        }
        prevF11 = currentF11;

        // シーン管理システムの更新
        SceneManager::GetInstance()->Update();

        ++m_frameCount;  // フレームカウント（デバッグ用）
    }

    // ==========================================
    // 描画処理: 毎フレーム呼ばれる
    // シーンマネージャーに描画を委譲
    // ==========================================
    void Application::Draw() {
        ClearDrawScreen();  // 画面クリア

        // シーン管理システムが現在のシーンを描画
        // （タイトル・ゲーム・リザルト・ポーズメニューなど）
        SceneManager::GetInstance()->Draw();
    }

} // namespace App