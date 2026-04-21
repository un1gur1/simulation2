#include <DxLib.h>
#include "Application.h"

// ==========================================
// WinMain: Windowsアプリケーションのエントリーポイント
// プログラムの開始地点
// ==========================================
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,           // アプリケーションインスタンスハンドル
    _In_opt_ HINSTANCE hPrevInstance,   // 前のインスタンス（Win32では常にNULL）
    _In_ LPSTR lpCmdLine,                // コマンドライン引数
    _In_ int nCmdShow)                   // ウィンドウ表示方法
{
    // ==========================================
    // 1. 初期化フェーズ
    // ==========================================

    // Applicationインスタンスを生成
    App::Application::CreateInstance();

    // DXライブラリとゲームシステムを初期化
    App::Application::GetInstance()->Init();

    // 初期化が失敗した場合は即終了
    if (App::Application::GetInstance()->IsInitFail())
    {
        App::Application::DeleteInstance();
        return -1;  // 異常終了
    }

    // ==========================================
    // 2. 実行フェーズ
    // ==========================================

    // メインループを実行（Update→Drawの繰り返し）
    // ユーザーがゲームを終了するまで続く
    App::Application::GetInstance()->Run();

    // ==========================================
    // 3. 終了フェーズ
    // ==========================================

    // リソースを解放（DXライブラリ、シーンマネージャーなど）
    App::Application::GetInstance()->Delete();

    // 解放が失敗した場合はエラー終了
    if (App::Application::GetInstance()->IsReleaseFail())
    {
        App::Application::DeleteInstance();
        return -1;  // 異常終了
    }

    // Applicationインスタンスを削除
    App::Application::DeleteInstance();

    return 0;  // 正常終了
}