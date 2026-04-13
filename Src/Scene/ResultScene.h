#pragma once

#include "SceneBase.h"

namespace App {

    class ResultScene : public SceneBase {
    private:
        int m_frameCount;
        static bool s_isWin; 

    public:
        ResultScene();
        ~ResultScene() override;

        void Init() override;
        void Load() override;
        void LoadEnd() override;
        void Update() override;
        void Draw() override;
        void Release() override;

        static void SetIsWin(bool isWin); // š static ŠÖ”‚É•ÏX
    };

} // namespace App