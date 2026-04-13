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
        // ★修正：1Pと2Pのターンでフェーズを分ける
        enum class Phase {
            P1_Move,
            P1_Action,
            P2_Move,
            P2_Action,
            Result
        };

        // ★追加：対人戦とNPC戦のモード選択
        enum class GameMode {
            VS_CPU,
            VS_PLAYER
        };

    private:
        Phase    m_currentPhase;
        GameMode m_gameMode; // モード保持用
        MapGrid  m_mapGrid;

        std::unique_ptr<Player> m_player; // 1P
        std::unique_ptr<Enemy>  m_enemy;  // 2P (CPU または 人間)

        bool       m_isPlayerSelected;
        IntVector2 m_hoverGrid;
        bool       m_enemyAIStarted;

        std::vector<std::string> m_actionLog;
        void AddLog(const std::string& message);

        bool CanMove(int number, char op, IntVector2 start, IntVector2 target, int& outCost) const;
        void ApplyBattleResult(UnitBase& unit, int resultNum);

    public:
        BattleMaster();
        ~BattleMaster() = default;

        // ★修正：初期化時にモードを受け取る（デフォルトはVS_CPU）
        void Init(GameMode mode = GameMode::VS_CPU);
        void Update();
        void Draw();

        bool IsGameOver() const;
        bool IsPlayerWin() const;

    private:
        // ★追加：入力処理の共通化
        void HandleMoveInput(UnitBase& activeUnit, Phase nextPhase);
        void HandleActionInput(UnitBase& actor, UnitBase& targetUnit);

        bool CheckButtonClick(int x, int y, int w, int h, Vector2 mousePos);
        void ExecuteBattle(UnitBase& attacker, UnitBase& defender, UnitBase& target);
        void ExecuteEnemyAI();
        void DrawMovableArea();
        void DrawEnemyDangerArea();
    };

} // namespace App