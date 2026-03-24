#pragma once
#include <string>
#include <queue>
#include "../../Common/Vector2.h"

namespace App {
    enum class UnitState { Idle, Moving, Dead };

    class UnitBase {
    protected:
        std::string m_name;
        IntVector2  m_gridPos;
        Vector2     m_screenPos;
        int         m_number;
        int         m_currentStocks;
        int         m_maxStocks;      // ストック上限
        UnitState   m_state;
        char        m_currentOp;
        unsigned int m_color;

        std::queue<Vector2> m_pathQueue;
        Vector2 m_moveStartPos, m_moveTargetPos;
        float   m_moveLerp;

    public:
        UnitBase(const std::string& name, IntVector2 startGrid, Vector2 startScreen, int number, int stocks, int maxStocks = 5);
        virtual ~UnitBase() = default;

        virtual void Update();
        virtual void Draw();

        void StartMove(IntVector2 finalGrid, std::queue<Vector2> path);

        // --- 数値・ストック操作（実験場用） ---
        void SetNumber(int n);
        void AddNumber(int v) { SetNumber(m_number + v); }
        void SetStocks(int s);
        void AddStocks(int v) { SetStocks(m_currentStocks + v); }
        void CycleOp();

        // ゲッター
        int GetNumber() const { return m_number; }
        int GetStocks() const { return m_currentStocks; }
        int GetMaxStocks() const { return m_maxStocks; }
        char GetOp() const { return m_currentOp; }
        IntVector2 GetGridPos() const { return m_gridPos; }
        bool IsMoving() const { return m_state == UnitState::Moving; }

    protected:
        virtual void DrawUnitGraphic() = 0;
        void DrawStatusOnMap() const;
    };
}