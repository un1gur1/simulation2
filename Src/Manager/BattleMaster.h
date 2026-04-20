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

    // 分数を完璧に計算・約分するオリジナル構造体
    struct Fraction {
        long long n; // 分子 (Numerator)
        long long d; // 分母 (Denominator)

        Fraction(long long num = 0, long long den = 1) : n(num), d(den) {
            if (d == 0) { d = 1; n = 0; } // 0割り防止
            if (d < 0) { n = -n; d = -d; }
            long long a = std::abs(n), b = d;
            while (b != 0) { long long t = b; b = a % b; a = t; }
            if (a != 0) { n /= a; d /= a; } // 最大公約数で約分
        }

        Fraction operator+(const Fraction& o) const { return Fraction(n * o.d + o.n * d, d * o.d); }
        Fraction operator-(const Fraction& o) const { return Fraction(n * o.d - o.n * d, d * o.d); }
        Fraction operator*(const Fraction& o) const { return Fraction(n * o.n, d * o.d); }
        Fraction operator/(const Fraction& o) const { return Fraction(n * o.d, d * o.n); }

        bool operator==(const Fraction& o) const { return n == o.n && d == o.d; }
        bool operator>(const Fraction& o) const { return n * o.d > o.n * d; }
        bool operator<(const Fraction& o) const { return n * o.d < o.n * d; }

        std::string ToString() const {
            if (d == 1) return std::to_string(n);
            long long whole = n / d;
            long long rem = std::abs(n % d);
            if (whole == 0) return (n < 0 ? "-" : "") + std::to_string(rem) + "/" + std::to_string(d);
            return std::to_string(whole) + (n < 0 ? " - " : " + ") + std::to_string(rem) + "/" + std::to_string(d);
        }
    };

    class BattleMaster {
    public:
        enum class Phase { P1_Move, P1_Action, P2_Move, P2_Action, Result, FINISH };
        enum class GameMode { VS_CPU, VS_PLAYER };
        enum class RuleMode { CLASSIC, ZERO_ONE };

    private:
        Phase    m_currentPhase;
        GameMode m_gameMode;
        RuleMode m_ruleMode;
        MapGrid  m_mapGrid;

        std::unique_ptr<Player> m_player;
        std::unique_ptr<Enemy>  m_enemy;

        Fraction m_p1ZeroOneScore;
        Fraction m_p2ZeroOneScore;
        int m_targetScore;

        bool       m_isPlayerSelected;
        IntVector2 m_hoverGrid;
        bool       m_enemyAIStarted;
        bool       m_playerAIStarted;

        bool m_is1P_NPC;
        bool m_is2P_NPC;

        // --- 演出・シェーダー用 ---
        int m_finishTimer = 0;
        int m_psHandle = -1;
        int m_cbHandle = -1;
        float m_shaderTime = 0.0f;
        float m_effectIntensity = 0.0f;

        // --- 戦績データ ---
        int m_totalMoves = 0;
        int m_totalOps = 0;
        int m_maxDamage = 0;
        int m_startTime = 0;
        std::vector<std::string> m_actionLog;
        void AddLog(const std::string& message);

        bool CanMove(int number, char op, IntVector2 start, IntVector2 target, int& outCost) const;
        void ApplyBattleResult(UnitBase& unit, const Fraction& resultFrac, int intRes, char op);

        bool Is1PTurn() const;
        UnitBase* GetActiveUnit() const;
        UnitBase* GetTargetUnit() const;

        // AI関連
        int  EvaluateBoard(const UnitBase& me, int myVirtualNumber, const UnitBase& enemy, IntVector2 targetPos, bool is1P) const;
        void ExecuteAI(UnitBase* me, UnitBase* opp, bool is1P);
        void PerformAIMove(UnitBase* me, IntVector2 bestTarget, int selectedCost, bool is1P);
        void ExecuteAIAction(UnitBase* me, UnitBase* opp, bool is1P);

    public:
        BattleMaster();
        ~BattleMaster();

        void Init();
        void Update();
        void Draw() const; // ★ 状態を変更しないためconst化

        bool IsGameOver() const;
        bool IsPlayerWin() const;

    private:
        void HandleMoveInput(UnitBase& activeUnit, Phase nextPhase);
        void HandleActionInput(UnitBase& actor, UnitBase& targetUnit);

        bool CheckButtonClick(int x, int y, int w, int h, const Vector2& mousePos) const;
        void ExecuteBattle(UnitBase& attacker, UnitBase& defender, UnitBase& target);

        void DrawMovableArea() const;       // ★ const化
        void DrawEnemyDangerArea() const;   // ★ const化
    };

} // namespace App