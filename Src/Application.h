#pragma once

#include <DxLib.h>

namespace App {

    class Application {
    public:
        // シングルトン操作
        static void CreateInstance();
        static Application* GetInstance();
        static void DeleteInstance();

        // ライフサイクル
        bool Init();                // 初期化
        bool IsInitFail() const;    // 初期化失敗フラグ
        void Run();                 // 実行（メインループ）
        void Delete();              // 解放
        bool IsReleaseFail() const; // 解放失敗フラグ

    private:
        Application();
        ~Application();

        // コピー禁止
        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        // 既存ロジック
        bool Initialize(); // 内部初期化
        void Shutdown();   // 内部解放
        void Update();
        void Draw();

    private:
        // ★追加：シングルトン用の静的インスタンス変数の宣言
        static Application* s_instance;

        bool m_shouldQuit;
        bool m_showInfo;
        unsigned int m_frameCount;
        int m_windowWidth;
        int m_windowHeight;

        bool m_initFail;
        bool m_releaseFail;
    };

} // namespace App