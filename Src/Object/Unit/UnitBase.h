#pragma once
#include <string>
#include <queue>
#include <vector>
#include <algorithm>
#include "../../Common/Vector2.h"

namespace App {

    // ==========================================
    // UnitState: ユニットの状態
    // ==========================================
    enum class UnitState {
        Idle,     // 待機中
        Moving,   // 移動中
        Dead      // 死亡（使用予定）
    };

    // ==========================================
    // UnitBase: ユニットの基底クラス
    // 用途: Player・Enemyの共通処理を実装
    // 機能: 移動・演算子管理・陣地管理・状態表示
    // ==========================================
    class UnitBase {
    public:
        // ==========================================
        // コンストラクタ・デストラクタ
        // ==========================================
        UnitBase(const std::string& name, IntVector2 startGrid, Vector2 startScreen,
            int number, int stocks, int maxStocks = 5);
        virtual ~UnitBase() = default;

        // ==========================================
        // 更新・描画
        // ==========================================
        virtual void Update();                      // 状態更新（毎フレーム）
        virtual void Draw();                        // 描画

        // ==========================================
        // 移動制御
        // ==========================================
        void StartMove(IntVector2 finalGrid, std::queue<Vector2> path);  // 移動開始
        bool IsMoving() const { return m_state == UnitState::Moving; }   // 移動中か判定

        // ==========================================
        // 数値（体力）管理
        // ==========================================
        void SetNumber(int n);                      // 数値を設定
        void AddNumber(int v) { SetNumber(m_number + v); }  // 数値を加算
        int GetNumber() const { return m_number; }  // 現在の数値取得

        // ==========================================
        // 残機管理
        // ==========================================
        void SetStocks(int s);                      // 残機を設定
        void AddStocks(int v) { SetStocks(m_currentStocks + v); }  // 残機を加算
        int GetStocks() const { return m_currentStocks; }           // 現在の残機数
        int GetMaxStocks() const { return m_maxStocks; }            // 最大残機数

        // ==========================================
        // 演算子管理
        // ==========================================
        void SetOp(char op) { m_currentOp = op; }   // 演算子を設定
        void CycleOp();                             // 演算子を切り替え（+→-→*→/→+...）
        char GetOp() const { return m_currentOp; }  // 現在の演算子取得

        // ==========================================
        // 陣地（ワープ駅）管理
        // 用途: 割り算で作った陣地にワープできる
        // ==========================================
        void AddWarpNode(IntVector2 node) { m_warpNodes.push_back(node); }  // 陣地を追加
        bool HasWarpNode(IntVector2 node) const {                           // 陣地を持っているか
            return std::find(m_warpNodes.begin(), m_warpNodes.end(), node) != m_warpNodes.end();
        }
        const std::vector<IntVector2>& GetWarpNodes() const { return m_warpNodes; }  // 全陣地取得

        // ==========================================
        // 位置取得
        // ==========================================
        IntVector2 GetGridPos() const { return m_gridPos; }  // グリッド座標取得

    protected:
        // ==========================================
        // 基本情報
        // ==========================================
        std::string m_name;         // ユニット名
        unsigned int m_color;       // 表示色

        // ==========================================
        // 位置・状態
        // ==========================================
        IntVector2  m_gridPos;      // グリッド座標（マス目）
        Vector2     m_screenPos;    // 画面座標（ピクセル）
        UnitState   m_state;        // 現在の状態

        // ==========================================
        // ゲームパラメータ
        // ==========================================
        int  m_number;              // 現在の数値（体力）
        int  m_currentStocks;       // 現在の残機数
        int  m_maxStocks;           // 最大残機数
        char m_currentOp;           // 現在装備している演算子（+, -, *, /）

        // ==========================================
        // 移動アニメーション
        // ==========================================
        std::queue<Vector2> m_pathQueue;    // 移動経路のキュー
        Vector2 m_moveStartPos;             // 移動開始座標
        Vector2 m_moveTargetPos;            // 移動目標座標
        float   m_moveLerp;                 // 補間パラメータ（0.0～1.0）

        // ==========================================
        // 陣地システム
        // ==========================================
        std::vector<IntVector2> m_warpNodes;  // 割り算で作った陣地のリスト

        // ==========================================
        // 内部処理: 描画
        // ==========================================
        virtual void DrawUnitGraphic() = 0;  // ユニットの見た目（純粋仮想関数）
        void DrawStatusOnMap() const;        // ステータス表示（数値・演算子）
    };

} // namespace App