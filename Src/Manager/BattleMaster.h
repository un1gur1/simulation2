#pragma once

#include <memory>
#include <vector>
#include <queue>
#include <string>
#include "../Common/Vector2.h"
#include "../Object/Map/MapGrid.h"
#include "../Object/Unit/Player/Player.h"
#include "../Object/Unit/Enemy/Enemy.h"
#include "../Scene/PauseMenu.h"

namespace App {

    // ==========================================
    // Fraction: 分数計算用の構造体
    // 用途: 四則演算の結果を正確に保持（小数誤差なし）
    // 機能: 自動約分、演算子オーバーロード、文字列変換
    // ==========================================
    struct Fraction {
        long long n;  // 分子 (Numerator)
        long long d;  // 分母 (Denominator)

        // コンストラクタ（自動約分・正規化）
        Fraction(long long num = 0, long long den = 1) : n(num), d(den) {
            if (d == 0) { d = 1; n = 0; }  // 0割り防止
            if (d < 0) { n = -n; d = -d; }  // 分母を正に統一
            // ユークリッドの互除法で最大公約数を計算
            long long a = std::abs(n), b = d;
            while (b != 0) { long long t = b; b = a % b; a = t; }
            if (a != 0) { n /= a; d /= a; }  // 約分
        }

        // 四則演算（自動約分される）
        Fraction operator+(const Fraction& o) const { return Fraction(n * o.d + o.n * d, d * o.d); }
        Fraction operator-(const Fraction& o) const { return Fraction(n * o.d - o.n * d, d * o.d); }
        Fraction operator*(const Fraction& o) const { return Fraction(n * o.n, d * o.d); }
        Fraction operator/(const Fraction& o) const { return Fraction(n * o.d, d * o.n); }

        // 比較演算子
        bool operator==(const Fraction& o) const { return n == o.n && d == o.d; }
        bool operator>(const Fraction& o) const { return n * o.d > o.n * d; }
        bool operator<(const Fraction& o) const { return n * o.d < o.n * d; }

        // 文字列変換（帯分数形式: "1 + 2/3"）
        std::string ToString() const {
            if (d == 1) return std::to_string(n);
            long long whole = n / d;
            long long rem = std::abs(n % d);
            if (whole == 0) return (n < 0 ? "-" : "") + std::to_string(rem) + "/" + std::to_string(d);
            return std::to_string(whole) + (n < 0 ? " - " : " + ") + std::to_string(rem) + "/" + std::to_string(d);
        }
    };

    // ==========================================
    // BattleMaster: バトル全体を管理するクラス
    // 用途: ターン制バトルのフロー制御、AI思考、勝敗判定
    // ==========================================
    class BattleMaster {
    public:
        // ==========================================
        // 列挙型定義
        // ==========================================
        enum class Phase {
            P1_Move,    // 1P移動フェーズ
            P1_Action,  // 1P行動フェーズ
            P2_Move,    // 2P移動フェーズ
            P2_Action,  // 2P行動フェーズ
            Result,     // 結果表示
            FINISH      // 終了
        };

        enum class GameMode {
            VS_CPU,     // CPU対戦
            VS_PLAYER   // プレイヤー対戦
        };

        enum class RuleMode {
            CLASSIC,    // クラシックモード（残機制）
            ZERO_ONE    // ゼロワンモード（スコア制）
        };

        // ==========================================
        // コンストラクタ・デストラクタ
        // ==========================================
        BattleMaster();
        ~BattleMaster();

        // ==========================================
        // 公開メソッド
        // ==========================================
        void Init();                    // 初期化
        void Update();                  // 更新（毎フレーム）
        void Draw() const;              // 描画

        bool IsGameOver() const;        // ゲーム終了判定
        bool IsPlayerWin() const;       // プレイヤー勝利判定

    private:
        // ==========================================
        // 定数
        // ==========================================
        // （必要に応じて追加）

        // ==========================================
        // ゲーム状態の変数
        // ==========================================
        Phase    m_currentPhase;        // 現在のフェーズ
        GameMode m_gameMode;            // ゲームモード
        RuleMode m_ruleMode;            // ルールモード
        MapGrid  m_mapGrid;             // マップ情報

        std::unique_ptr<Player> m_player;  // 1Pユニット
        std::unique_ptr<Enemy>  m_enemy;   // 2Pユニット

        // ゼロワンモード用スコア
        Fraction m_p1ZeroOneScore;      // 1Pスコア（分数）
        Fraction m_p2ZeroOneScore;      // 2Pスコア（分数）
        int m_targetScore;              // 目標スコア

        // 入力・選択状態
        bool       m_isPlayerSelected;  // プレイヤーが選択中か
        IntVector2 m_hoverGrid;         // マウスホバー中のマス
        bool       m_enemyAIStarted;    // 2P AI開始フラグ
        bool       m_playerAIStarted;   // 1P AI開始フラグ

        bool m_is1P_NPC;                // 1PがCPUか
        bool m_is2P_NPC;                // 2PがCPUか

        int g_aiStayCount1P;            // 1P AI待機カウント
        int g_aiStayCount2P;            // 2P AI待機カウント

        // ==========================================
        // 演出用の変数
        // ==========================================
        int   m_finishTimer;            // 終了演出タイマー
        int   m_psHandle;               // ピクセルシェーダーハンドル
        int   m_cbHandle;               // 定数バッファハンドル
        float m_shaderTime;             // シェーダー時間
        float m_effectIntensity;        // エフェクト強度

        // ==========================================
        // 戦績データ
        // ==========================================
        int m_totalMoves;               // 総移動数
        int m_totalOps;                 // 総演算回数
        int m_maxDamage;                // 最大ダメージ
        int m_startTime;                // 開始時刻
        std::vector<std::string> m_actionLog;  // 行動ログ

        // ==========================================
        // 内部処理メソッド: ゲームロジック
        // ==========================================
        bool Is1PTurn() const;                      // 1Pのターンか判定
        UnitBase* GetActiveUnit() const;            // アクティブなユニット取得
        UnitBase* GetTargetUnit() const;            // ターゲットユニット取得

        bool CanMove(int number, char op, IntVector2 start, IntVector2 target, int& outCost) const;  // 移動可能判定
        void ApplyBattleResult(UnitBase& unit, const Fraction& resultFrac, int intRes, char op);     // 戦闘結果適用

        void AddLog(const std::string& message);    // ログ追加

        int m_maxTurns;
        bool m_isPaused;
        PauseMenu m_pauseMenu;
        // ==========================================
        // 内部処理メソッド: 入力処理
        // ==========================================
        void HandleMoveInput(UnitBase& activeUnit, Phase nextPhase);       // 移動入力処理
        void HandleActionInput(UnitBase& actor, UnitBase& targetUnit);     // 行動入力処理

        bool CheckButtonClick(int x, int y, int w, int h, const Vector2& mousePos) const;  // ボタンクリック判定

        // ==========================================
        // 内部処理メソッド: 戦闘処理
        // ==========================================
        void ExecuteBattle(UnitBase& attacker, UnitBase& defender, UnitBase& target);  // 戦闘実行

        // ==========================================
        // 内部処理メソッド: AI思考
        // ==========================================
        int  EvaluateBoard(const UnitBase& me, int myVirtualNumber, const UnitBase& enemy, IntVector2 targetPos, bool is1P) const;  // 盤面評価
        void ExecuteAI(UnitBase* me, UnitBase* opp, bool is1P);                        // AI実行
        void PerformAIMove(UnitBase* me, IntVector2 bestTarget, int selectedCost, bool is1P);  // AI移動実行
        void ExecuteAIAction(UnitBase* me, UnitBase* opp, bool is1P);                  // AI行動実行

        // ==========================================
        // 内部処理メソッド: 描画補助
        // ==========================================
        void DrawMovableArea() const;           // 移動可能範囲描画
        void DrawEnemyDangerArea() const;       // 敵の危険範囲描画
    };

} // namespace App