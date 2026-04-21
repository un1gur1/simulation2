#pragma once
#include "SceneBase.h"

namespace App {

    // 前方宣言
    class Loading;
    class Fader;
    class GameScene;
    class TitleScene;
    class ResultScene;
    class PauseMenu;

    // ==========================================
    // BattleStats: バトルの戦績データ
    // 用途: GameScene→ResultSceneへの結果受け渡し
    // ==========================================
    struct BattleStats {
        int totalTurns;      // 経過ターン数
        int totalMoves;      // 総移動マス数
        int totalOpsUsed;    // 使用した演算子数
        int playTimeFrames;  // プレイ時間（ミリ秒）
        int maxDamage;       // 最大ダメージ（スコア）
        int m_maxStocks;     // 最大残機数
    };

    // ==========================================
    // SceneManager: シーン管理システム（シングルトン）
    // 用途: シーン切り替え・ゲーム設定保持・ポーズ管理
    // ==========================================
    class SceneManager {
    public:
        // ==========================================
        // SCENE_ID: シーンの種類
        // ==========================================
        enum class SCENE_ID {
            NONE,      // シーンなし
            TITLE,     // タイトル画面
            GAME,      // ゲーム本編
            RESULT     // リザルト画面
        };

        // ==========================================
        // シングルトンパターン
        // ==========================================
        static void CreateInstance() { if (instance_ == nullptr) { instance_ = new SceneManager(); } }
        static SceneManager* GetInstance() { return instance_; }
        static void DeleteInstance() { if (instance_ != nullptr) { delete instance_; instance_ = nullptr; } }

        // ==========================================
        // 初期化・更新・描画
        // ==========================================
        void Init();           // 初期化
        void Init3D();         // 3D初期化（将来的な拡張用）
        void Update();         // 更新（毎フレーム）
        void Draw();           // 描画（毎フレーム）
        void Delete();         // 削除

        // ==========================================
        // シーン管理
        // ==========================================
        void ChangeScene(SCENE_ID nextId);          // シーン切り替え
        SCENE_ID GetSceneID() const { return sceneId_; }  // 現在のシーンID取得

        // ==========================================
        // ゲーム終了管理
        // ==========================================
        void GameEnd() { isGameEnd_ = true; }
        bool GetGameEnd() const { return isGameEnd_; }

        // ==========================================
        // ゲーム設定（タイトル画面で設定）
        // ==========================================
        void SetGameSettings(int players, int mode, int zeroOneScore = 501) {
            playerCount_ = players;
            gameMode_ = mode;
            zeroOneScore_ = zeroOneScore;
        }
        int GetPlayerCount() const { return playerCount_; }     // プレイヤー人数
        int GetGameMode() const { return gameMode_; }           // ゲームモード
        int GetZeroOneScore() const { return zeroOneScore_; }   // ゼロワン目標スコア
        int GetMaxStocks() const { return zeroOneScore_; }      // 最大残機数

        // ==========================================
        // プレイヤー設定
        // ==========================================
        void SetPlayer1Settings(bool isNPC, int startNum, int startX, int startY) {
            is1P_NPC_ = isNPC;
            p1StartNum_ = startNum;
            p1StartX_ = startX;
            p1StartY_ = startY;
        }
        void SetPlayer2Settings(bool isNPC, int startNum, int startX, int startY) {
            is2P_NPC_ = isNPC;
            p2StartNum_ = startNum;
            p2StartX_ = startX;
            p2StartY_ = startY;
        }

        bool Is1PNPC() const { return is1P_NPC_; }          // 1PがCPUか
        bool Is2PNPC() const { return is2P_NPC_; }          // 2PがCPUか
        int Get1PStartNum() const { return p1StartNum_; }   // 1P初期数値
        int Get2PStartNum() const { return p2StartNum_; }   // 2P初期数値
        int Get1PStartX() const { return p1StartX_; }       // 1P初期X座標
        int Get1PStartY() const { return p1StartY_; }       // 1P初期Y座標
        int Get2PStartX() const { return p2StartX_; }       // 2P初期X座標
        int Get2PStartY() const { return p2StartY_; }       // 2P初期Y座標

        // ==========================================
        // ステージ設定
        // ==========================================
        void SetStageIndex(int idx) { m_stageIndex = idx; }
        int GetStageIndex() const { return m_stageIndex; }  // ステージ番号（0,1,2）

        // ==========================================
        // バトル結果（GameScene→ResultScene受け渡し用）
        // ==========================================
        void SetBattleResult(bool isWin, const BattleStats& stats) {
            m_lastIsWin = isWin;
            m_lastStats = stats;
        }
        bool GetLastIsWin() const { return m_lastIsWin; }              // 勝利したか
        const BattleStats& GetLastStats() const { return m_lastStats; } // 戦績データ

    private:
        // ==========================================
        // シングルトン: コピー・ムーブ禁止
        // ==========================================
        SceneManager();
        ~SceneManager();
        SceneManager(const SceneManager&) = delete;
        SceneManager& operator=(const SceneManager&) = delete;
        SceneManager(SceneManager&&) = delete;
        SceneManager& operator=(SceneManager&&) = delete;

        // ==========================================
        // 内部処理
        // ==========================================
        void PerformSceneChange();  // シーン切り替え実行

        // ==========================================
        // シングルトンインスタンス
        // ==========================================
        static SceneManager* instance_;

        // ==========================================
        // シーン管理
        // ==========================================
        SceneBase* scene_;          // 現在のシーン
        Loading* load_;             // ローディング画面（将来的な拡張用）
        Fader* fader_;              // フェード演出（将来的な拡張用）
        PauseMenu* pauseMenu_;      // ポーズメニュー

        SCENE_ID sceneId_;          // 現在のシーンID
        SCENE_ID nextSceneId_;      // 次のシーンID

        // ==========================================
        // 状態フラグ
        // ==========================================
        bool isChanging_;           // シーン切り替え中か
        bool isGameEnd_;            // ゲーム終了フラグ
        bool isPaused_;             // ポーズ中か

        // ==========================================
        // ゲーム設定（タイトル画面で設定）
        // ==========================================
        int playerCount_;           // プレイヤー人数（1=シングル, 2=2P）
        int gameMode_;              // ゲームモード（0=クラシック, 1=ゼロワン）
        int zeroOneScore_;          // ゼロワンモードの目標スコア

        // ==========================================
        // プレイヤー設定
        // ==========================================
        bool is1P_NPC_;             // 1PがCPUか
        bool is2P_NPC_;             // 2PがCPUか
        int  p1StartNum_;           // 1P初期数値
        int  p2StartNum_;           // 2P初期数値
        int  p1StartX_;             // 1P初期X座標
        int  p1StartY_;             // 1P初期Y座標
        int  p2StartX_;             // 2P初期X座標
        int  p2StartY_;             // 2P初期Y座標

        // ==========================================
        // ステージ設定
        // ==========================================
        int m_stageIndex;           // ステージ番号（0=バランス, 1=マイナス, 2=カオス）

        // ==========================================
        // バトル結果（GameScene→ResultScene受け渡し）
        // ==========================================
        bool m_lastIsWin;           // 前回の勝敗
        BattleStats m_lastStats;    // 前回の戦績
    };

} // namespace App