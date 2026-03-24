#pragma once

#include <memory>
#include <vector>
#include <queue>
#include <string>
#include "../Common/Vector2.h"
#include "../Object/Map/MapGrid.h"
#include "../Object/Unit/Player/Player.h"
#include "../Object/Unit/Enemy/Enemy.h"

namespace App {

    /**
     * @brief ゲームのメインロジック、フェーズ、UIレイアウトを統括するクラス
     */
    class BattleMaster {
    public:
        enum class Phase {
            PlayerTurn,      // 移動先を選ぶフェーズ
            ActionSelect,    // 移動後に「攻撃」か「待機」を選ぶフェーズ
            EnemyTurn,       // 敵のフェーズ
            Result
        };

    private:
        // --- システム・進行管理 ---
        Phase    m_currentPhase;
        MapGrid  m_mapGrid;

        // --- ユニット管理 ---
        std::unique_ptr<Player> m_player;
        std::unique_ptr<Enemy>  m_enemy;

        // --- スマート移動・操作用 ---
        bool       m_isPlayerSelected; // プレイヤーをクリックして選択中か
        IntVector2 m_hoverGrid;        // 現在マウスが乗っているマス
        bool       m_enemyAIStarted;   // 敵のAIが思考・移動を開始したか

        std::vector<std::string> m_actionLog;
        void AddLog(const std::string& message);

    public:
        BattleMaster();
        ~BattleMaster() = default;

        void Init();
        void Update();
        void Draw();

        bool IsGameOver() const;

    private:
        bool CheckButtonClick(int x, int y, int w, int h, Vector2 mousePos);
        void ExecuteBattle(UnitBase& attacker, UnitBase& defender);
        void CheckTurnEnd();

        void ExecuteEnemyAI();    // 敵の思考と自動移動
        void DrawMovableArea();   // 移動可能エリアのハイライト描画
        void DrawEnemyDangerArea();
    };

} // namespace App