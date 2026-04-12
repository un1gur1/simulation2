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

    class BattleMaster {
    public:
        enum class Phase {
            PlayerTurn,
            ActionSelect,
            EnemyTurn,
            Result
        };

    private:
        Phase    m_currentPhase;
        MapGrid  m_mapGrid;

        std::unique_ptr<Player> m_player;
        std::unique_ptr<Enemy>  m_enemy;

        bool       m_isPlayerSelected;
        IntVector2 m_hoverGrid;
        bool       m_enemyAIStarted;

        std::vector<std::string> m_actionLog;
        void AddLog(const std::string& message);

        bool CanMove(int number, char op, IntVector2 start, IntVector2 target, int& outCost) const;

        // バトル結果の適用ロジック
        void ApplyBattleResult(UnitBase& unit, int resultNum);

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
        void ExecuteEnemyAI();
        void DrawMovableArea();
        void DrawEnemyDangerArea();
    };

} // namespace App