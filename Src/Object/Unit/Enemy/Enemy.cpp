#include "Enemy.h"
#include <DxLib.h>

namespace App {

    Enemy::Enemy(IntVector2 startGrid, Vector2 startScreen, int number, int stocks, int maxStocks)
        : UnitBase("Enemy", startGrid, startScreen, number, stocks, maxStocks)
    {
        // 敵の色（薄い赤）
        m_color = GetColor(255, 100, 100);
    }

    void Enemy::DrawUnitGraphic() {
        int x = (int)m_screenPos.x;
        int y = (int)m_screenPos.y;

        // 本体（赤い円）
        DrawCircle(x, y, 25, m_color, TRUE);
        DrawCircle(x, y, 25, GetColor(255, 255, 255), FALSE);

        // 敵の識別文字
        DrawFormatString(x - 8, y - 8, GetColor(255, 255, 255), "E");
    }

} // namespace App