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
        , m_currentOp('+') // 初期装備は '+' に固定
        , m_moveLerp(0.0f)
    {
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

        DrawUnitGraphic();
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

        // --- 1. ストック変動ロジック ---

        // 10以上になったらストック回復
        while (m_number >= 10 && m_currentStocks < m_maxStocks) {
            m_currentStocks++;
            m_number -= 10;
        }
        // 満タンの時に10以上になった場合は、9で止めるか余りを出す
        if (m_number >= 10) {
            m_number %= 10;
            if (m_number == 0) m_number = 9; // 0にならないよう調整
        }

        // 0以下になったらストック消費して「9（全方位・最大距離）」に覚醒
        while (m_number <= 0 && m_currentStocks > 0) {
            m_currentStocks--;
            m_number = 9; // 一律で最強状態の9へ
        }

        // 死亡判定
        if (m_currentStocks <= 0 && m_number <= 0) {
            m_currentStocks = 0;
            m_state = UnitState::Dead;
            m_number = 0;
        }

        // ※削除した部分※
        // 以前ここにあった「数字によって m_currentOp を書き換える処理」は、
        // マップから演算子アイテムを拾う仕様になったため削除しました！
    }

    void UnitBase::SetStocks(int s) {
        m_currentStocks = std::clamp(s, 0, m_maxStocks);
        if (m_currentStocks <= 0 && m_number <= 0) m_state = UnitState::Dead;
        else if (m_state == UnitState::Dead) m_state = UnitState::Idle;
    }

    void UnitBase::CycleOp() {
        if (m_currentOp == '+') m_currentOp = '-';
        else if (m_currentOp == '-') m_currentOp = '*';
        else if (m_currentOp == '*') m_currentOp = '/';
        else m_currentOp = '+';
    }

    void UnitBase::DrawStatusOnMap() const {
        int x = (int)m_screenPos.x;
        int y = (int)m_screenPos.y - 45;

        // 数字の背景円
        DrawCircle(x, y, 16, GetColor(0, 0, 0), TRUE);
        // 数字本体
        DrawFormatString(x - 5, y - 7, GetColor(255, 255, 0), "%d", m_number);
        // 現在の演算子
        DrawFormatString(x + 18, y - 7, GetColor(255, 255, 255), "[%c]", m_currentOp);
    }

} // namespace App