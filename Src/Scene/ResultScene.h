#pragma once
#include "SceneBase.h"
#include "SceneManager.h" // ★ BattleStats をここで読み込む

namespace App {

    class ResultScene : public SceneBase {
    public:
        ResultScene();
        virtual ~ResultScene();

        virtual void Init() override;
        virtual void Load() override;
        virtual void LoadEnd() override;
        virtual void Update() override;
        virtual void Draw() override;
        virtual void Release() override;

    private:
        int m_frameCount;
        int m_psHandle;
        int m_cbHandle;

        // ★ SceneManager から受け取った結果を保存する変数（staticは不要！）
        bool m_isWin;
        BattleStats m_stats;
    };

} // namespace App