#include "Application.h"
#include <DxLib.h>
#include "Scene/SceneManager.h" 
namespace App {

    // 静的インスタンス
    Application* Application::s_instance = nullptr;

    void Application::CreateInstance() {
        if (s_instance == nullptr) {
            s_instance = new Application();
        }
    }

    Application* Application::GetInstance() {
        return s_instance;
    }

    void Application::DeleteInstance() {
        if (s_instance != nullptr) {
            delete s_instance;
            s_instance = nullptr;
        }
    }

    Application::Application()
        : m_shouldQuit(false)
        , m_showInfo(true)
        , m_frameCount(0)
        , m_windowWidth(1280) // 画面サイズ（必要に応じて変更してください）
        , m_windowHeight(720)
        , m_initFail(false)
        , m_releaseFail(false)
    {
    }

    Application::~Application() {
    }

    bool Application::Init() {
        m_initFail = false;
        if (!Initialize()) {
            m_initFail = true;
            return false;
        }

        // --- シーン管理の初期化と最初のシーン設定 ---
        // ※SceneManager側の実装（CreateInstance等）に合わせて適宜調整してください
        if (SceneManager::GetInstance() == nullptr) {
            SceneManager::CreateInstance();
        }
        SceneManager::GetInstance()->Init();

        SceneManager::GetInstance()->ChangeScene(SceneManager::SCENE_ID::TITLE);

        return true;
    }

    bool Application::IsInitFail() const {
        return m_initFail;
    }

    void Application::Run() {
        // メインループ
        while (!m_shouldQuit) {
            int msg = ProcessMessage();
            if (msg == -1 || msg == 1) {
                m_shouldQuit = true;
                break;
            }

            Update();
            Draw();
            ScreenFlip();
            Sleep(1);
        }
    }

    void Application::Delete() {
        m_releaseFail = false;
        Shutdown();
    }

    bool Application::IsReleaseFail() const {
        return m_releaseFail;
    }

    // --- 内部実装 ---

    bool Application::Initialize() {
        // 1. 解像度をフルHDに設定
        m_windowWidth = 1920;
        m_windowHeight = 1080;
        SetGraphMode(m_windowWidth, m_windowHeight, 32);

        // 2. フルスクリーンに設定（FALSEでフルスクリーン）
        // 開発中は TRUE (ウィンドウ) の方がデバッグしやすいですが、本番は FALSE にします
        ChangeWindowMode(FALSE);

        SetMainWindowText("simulation");
        //SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8);

        if (DxLib_Init() == -1) return false;

        SetMouseDispFlag(TRUE);

        SetDrawScreen(DX_SCREEN_BACK);
        SetBackgroundColor(20, 20, 25); // 少し紺色っぽい暗い背景がおしゃれ
        return true;
    }
    void Application::Shutdown() {
        // DxLibを終了する前に、SceneManagerを安全に破棄してメモリを解放
        SceneManager::DeleteInstance();

        DxLib_End();
    }

    void Application::Update() {
        // ESCキーによるアプリケーション終了処理
        if (CheckHitKey(KEY_INPUT_ESCAPE) == 1) {
            m_shouldQuit = true;
            return;
        }

        // ゲーム全体の更新処理を SceneManager に委譲
        SceneManager::GetInstance()->Update();

        ++m_frameCount;
    }

    void Application::Draw() {
        // 画面のクリア
        ClearDrawScreen();

        // ゲーム全体の描画処理を SceneManager に委譲
        SceneManager::GetInstance()->Draw();
    }

} // namespace App