#pragma once

#include "SceneBase.h"

namespace App {

    class TitleScene : public SceneBase {
    private:
        int m_frameCount;

    public:
        TitleScene();
        ~TitleScene() override;

        void Init() override;
        void Load() override;
        void LoadEnd() override;
        void Update() override;
        void Draw() override;
        void Release() override;

    private:
        // •K—v‚È‚ç•â•ŠÖ”‚ğ‚±‚±‚É
    };

} // namespace App
