#include "UnitBase.h"
#include <DxLib.h>
#include <algorithm>

namespace App {

    UnitBase::UnitBase(const std::string& name, IntVector2 startGrid, Vector2 startScreen, int number, int stocks, int maxStocks)
        : m_name(name)
        , m_gridPos(startGrid)
        , m_screenPos(startScreen)
        , m_number(number)
        , m_currentStocks(stocks)
        , m_maxStocks(maxStocks)
        , m_state(UnitState::Idle)
        , m_currentOp('+')
        , m_moveLerp(0.0f)
    {
        // 初期状態の数字に基づいて演算子を決定
        SetNumber(m_number);
    }

    void UnitBase::Update() {
        if (m_state == UnitState::Moving) {
            m_moveLerp += 0.1f; // 移動スピード
            if (m_moveLerp >= 1.0f) {
                m_screenPos = m_moveTargetPos;
                if (!m_pathQueue.empty()) {
                    m_moveStartPos = m_screenPos;
                    m_moveTargetPos = m_pathQueue.front();
                    m_pathQueue.pop();
                    m_moveLerp = 0.0f;
                }
                else {
                    m_state = UnitState::Idle;
                }
            }
            else {
                // 線形補間による移動演出
                m_screenPos = m_moveStartPos * (1.0f - m_moveLerp) + m_moveTargetPos * m_moveLerp;
            }
        }
    }

    void UnitBase::Draw() {
        if (m_state == UnitState::Dead) return;

        // 子クラス（Player/Enemy）固有のグラフィック描画
        DrawUnitGraphic();
        // マップ上の数字・ステータス描画
        DrawStatusOnMap();
    }

    void UnitBase::StartMove(IntVector2 finalGrid, std::queue<Vector2> path) {
        if (m_state != UnitState::Idle || path.empty()) return;

        m_gridPos = finalGrid;
        m_moveStartPos = m_screenPos;
        m_moveTargetPos = path.front();
        path.pop();
        m_pathQueue = path;
        m_moveLerp = 0.0f;
        m_state = UnitState::Moving;
    }

    void UnitBase::SetNumber(int n) {
        m_number = n;

        // --- 1. 連続ストック変動ロジック ---
        // 数字が10以上（チャージ）の判定
        while (m_number >= 10 && m_currentStocks < m_maxStocks) {
            m_currentStocks++;
            m_number -= 10;
        }
        if (m_number >= 10) m_number %= 10; // 満タン時は端数処理

        // 数字が0以下（ダメージ）の判定：連続してストックが減り、9から再出発する
        while (m_number <= 0 && m_currentStocks > 0) {
            m_currentStocks--;
            m_number += 9; // ダメージを受けると 9 に戻る
        }

        // 死亡判定
        if (m_currentStocks <= 0) {
            m_currentStocks = 0;
            m_state = UnitState::Dead;
            m_number = 0;
        }

        // --- 2. ★数字連動による演算子（移動パターン）の自動決定 ---
        // 橋本さん案：プラスは十字(3-5)、マイナスは横(1-2)、カケルは斜め(6-8)、ワルは桂馬(9)
        int digit = m_number % 10;
        if (digit == 0) digit = 10; // 0の時は特殊処理

        if (digit >= 1 && digit <= 2)      m_currentOp = '-'; // 横
        else if (digit >= 3 && digit <= 5) m_currentOp = '+'; // 十字
        else if (digit >= 6 && digit <= 8) m_currentOp = '*'; // 斜め
        else if (digit >= 9)               m_currentOp = '/'; // 桂馬
    }

    void UnitBase::SetStocks(int s) {
        m_currentStocks = std::clamp(s, 0, m_maxStocks);
        if (m_currentStocks <= 0) m_state = UnitState::Dead;
        else if (m_state == UnitState::Dead) m_state = UnitState::Idle;

        // ストックが変わった際も演算子を再計算
        SetNumber(m_number);
    }

    void UnitBase::CycleOp() {
        // 手動切り替え（デバッグ用）
        if (m_currentOp == '+') m_currentOp = '-';
        else if (m_currentOp == '-') m_currentOp = '*';
        else if (m_currentOp == '*') m_currentOp = '/';
        else m_currentOp = '+';
    }

    void UnitBase::DrawStatusOnMap() const {
        int x = (int)m_screenPos.x;
        int y = (int)m_screenPos.y - 45; // ユニットの少し上に表示

        // 数字の背景円
        DrawCircle(x, y, 16, GetColor(0, 0, 0), TRUE);
        // 数字本体
        DrawFormatString(x - 5, y - 7, GetColor(255, 255, 0), "%d", m_number);
        // 現在の型（演算子）
        DrawFormatString(x + 18, y - 7, GetColor(255, 255, 255), "[%c]", m_currentOp);
    }

} // namespace App