#include <DxLib.h>
#include "Application.h"

// WinMain関数
//---------------------------------
int WINAPI WinMain(
    _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    // インスタンスの生成（名前空間を考慮）
    App::Application::CreateInstance();
    App::Application::GetInstance()->Init();

    if (App::Application::GetInstance()->IsInitFail())
    {
        // 初期化失敗
        App::Application::DeleteInstance();
        return -1;
    }

    // 実行
    App::Application::GetInstance()->Run();

    // 解放
    App::Application::GetInstance()->Delete();

    if (App::Application::GetInstance()->IsReleaseFail())
    {
        // 解放失敗
        App::Application::DeleteInstance();
        return -1;
    }

    App::Application::DeleteInstance();

    return 0;
}