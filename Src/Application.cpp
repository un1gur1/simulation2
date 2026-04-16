#include "Application.h"
#include <DxLib.h>
#include "Scene/SceneManager.h"
#include "Input/InputManager.h" // InputManagerのパスは環境に合わせてください

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
        , m_windowWidth(1920) // 初期値をフルHDに
        , m_windowHeight(1080)
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

        // ★追加：InputManagerの初期化
        InputManager::GetInstance().Init();

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

            if (SceneManager::GetInstance()->GetGameEnd()) {
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

    bool Application::Initialize() {
        m_windowWidth = 1920;
        m_windowHeight = 1080;

        ChangeWindowMode(TRUE);

        // ★ ボーダーレスフルスクリーン設定（カーソル消失防止）
        SetFullScreenResolutionMode(DX_FSRESOLUTIONMODE_DESKTOP);

        SetGraphMode(m_windowWidth, m_windowHeight, 32);

        SetWindowSizeChangeEnableFlag(TRUE, TRUE);
        SetWindowStyleMode(7);
        SetWindowSizeExtendRate(1.0);
        SetWindowSize(1280, 720);
        SetMainWindowText("超計算マスBATTLE");

        if (DxLib_Init() == -1) return false;

        SetMouseDispFlag(TRUE);
        SetDrawScreen(DX_SCREEN_BACK);

        // コンセプトカラー：深い紺色（背景色と同化させて黒帯を目立たせない）
        SetBackgroundColor(5, 10, 25);
        return true;
    }

    void Application::Shutdown() {
        SceneManager::DeleteInstance();
        DxLib_End();
    }

    void Application::Update() {
        // ★ 全シーン共通：入力を毎フレーム更新
        InputManager::GetInstance().Update();

        // ※ ESCキーでの即終了は削除（SceneManagerのポーズ処理に委譲するため）

        // ====================================================
        // ★ F11キーで フルスクリーン ⇔ ウィンドウ の切り替え
        // ====================================================
        static int prevF11 = 0;
        int currentF11 = CheckHitKey(KEY_INPUT_F11);

        if (currentF11 == 1 && prevF11 == 0) {
            if (GetWindowModeFlag() == TRUE) {
                ChangeWindowMode(FALSE);
            }
            else {
                ChangeWindowMode(TRUE);
            }
            SetMouseDispFlag(TRUE); // 切り替え時の念押し表示
        }
        prevF11 = currentF11;
        // ====================================================

        SceneManager::GetInstance()->Update();
        ++m_frameCount;
    }

    void Application::Draw() {
        ClearDrawScreen();

        // 描画は常に SceneManager（およびその中のポーズ画面）が行う
        SceneManager::GetInstance()->Draw();
    }

} // namespace App