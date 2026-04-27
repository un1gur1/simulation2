#include "UnitBase.h"
#include <DxLib.h>
#include <algorithm>

namespace App {

    // ==========================================
    // コンストラクタ: ユニットの基本情報を初期化
    // ==========================================
    UnitBase::UnitBase(const std::string& name, IntVector2 startGrid, Vector2 startScreen, int number, int stocks, int maxStocks)
        : m_name(name)                   // ユニット名（Player / Enemy）
        , m_gridPos(startGrid)           // グリッド上の座標
        , m_screenPos(startScreen)       // 画面上の座標
        , m_number(number)               // 現在の数値（1~9）
        , m_currentStocks(stocks)        // 現在の残機数
        , m_maxStocks(maxStocks)         // 最大残機数
        , m_state(UnitState::Idle)       // 初期状態: 待機
        , m_currentOp('\0')              // 初期演算子: なし（マップから取得する）
        , m_moveLerp(0.0f)               // 移動補間値
    {
        SetNumber(m_number);  // 数値設定（残機変動処理を含む）
    }

    // ==========================================
    // 毎フレーム更新: 移動アニメーションの処理
    // ==========================================
    void UnitBase::Update() {
        // 移動中のみ処理
        if (m_state == UnitState::Moving) {
            m_moveLerp += 0.1f;  // 補間値を進める（0.0 → 1.0）

            // 1マス分の移動完了
            if (m_moveLerp >= 1.0f) {
                m_screenPos = m_moveTargetPos;  // 目標座標に到達

                // 次の経由地点があるか確認
                if (!m_pathQueue.empty()) {
                    // 次のマスへ移動開始
                    m_moveStartPos = m_screenPos;
                    m_moveTargetPos = m_pathQueue.front();
                    m_pathQueue.pop();
                    m_moveLerp = 0.0f;
                }
                else {
                    // 全ての経由地点を通過完了
                    m_state = UnitState::Idle;
                }
            }
            else {
                // 線形補間（Lerp）で滑らかに移動
                // 例: lerp=0.5なら、開始地点と目標地点の中間に表示
                m_screenPos = m_moveStartPos * (1.0f - m_moveLerp) + m_moveTargetPos * m_moveLerp;
            }
        }
    }

    // ==========================================
    // 描画処理: ユニット本体とステータスを表示
    // ==========================================
    void UnitBase::Draw() {
        if (m_state == UnitState::Dead) return;  // 死亡時は描画しない

        DrawUnitGraphic();    // ユニット本体（Player/Enemyで実装が異なる）
        DrawStatusOnMap();    // ステータス表示（現在は非使用）
    }

    // ==========================================
    // 移動開始: 経路に沿って移動アニメーションを開始
    // 引数: finalGrid - 最終到着地点のグリッド座標
    //       path - 経由する画面座標のキュー
    // ==========================================
    void UnitBase::StartMove(IntVector2 finalGrid, std::queue<Vector2> path) {
        // 待機状態かつパスが存在する場合のみ移動可能
        if (m_state != UnitState::Idle || path.empty()) return;

        m_gridPos = finalGrid;              // 最終到着地点を記録
        m_moveStartPos = m_screenPos;       // 移動開始地点
        m_moveTargetPos = path.front();     // 最初の経由地点
        path.pop();
        m_pathQueue = path;                 // 残りの経由地点
        m_moveLerp = 0.0f;                  // 補間値初期化
        m_state = UnitState::Moving;        // 移動状態へ
    }

    // ==========================================
    // 数値設定: 残機変動・死亡判定を含む重要な処理
    // ==========================================
    void UnitBase::SetNumber(int n) {
        m_number = n;

        // ==========================================
        // 残機変動ロジック
        // ==========================================

        // 【ストック回復】数値が10以上になると残機+1
        // 例: 数値15 → 残機+1、数値5
        while (m_number >= 10 && m_currentStocks < m_maxStocks) {
            m_currentStocks++;
            m_number -= 10;
        }

        // 残機満タン時に10以上になった場合の処理
        if (m_number >= 10) {
            m_number %= 10;  // 余剰分は切り捨て
            if (m_number == 0) m_number = 9;  // 0は使わない（最小値は1）
        }

        // 【ストック消費】数値が0以下になると残機-1して「9」に
        // 9 = 全方位移動可能・最大距離3マスの状態
        while (m_number <= 0 && m_currentStocks > 0) {
            m_currentStocks--;
            m_number = 9;  // 一律で最強状態へリセット
        }

        // 【死亡判定】残機0かつ数値0以下でゲームオーバー
        if (m_currentStocks <= 0 && m_number <= 0) {
            m_currentStocks = 0;
            m_state = UnitState::Dead;
            m_number = 0;
        }
    }

    // ==========================================
    // 残機設定: 外部から残機を直接変更（回復・ダメージ処理用）
    // ==========================================
    void UnitBase::SetStocks(int s) {
        // 0 ~ maxStocks の範囲に制限
        m_currentStocks = std::clamp(s, 0, m_maxStocks);

        // 死亡判定
        if (m_currentStocks <= 0 && m_number <= 0)
            m_state = UnitState::Dead;
        // 復活判定
        else if (m_state == UnitState::Dead)
            m_state = UnitState::Idle;
    }

  
    void UnitBase::CycleOp() {
        if (m_currentOp == '+') m_currentOp = '-';
        else if (m_currentOp == '-') m_currentOp = '*';
        else if (m_currentOp == '*') m_currentOp = '/';
        else m_currentOp = '+';
    }

  
    void UnitBase::DrawStatusOnMap() const {
    }

} // namespace App