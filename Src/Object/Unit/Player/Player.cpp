#include "Player.h"
#include <DxLib.h>

namespace App {

    Player::Player(IntVector2 startGrid, Vector2 startScreen, int number, int stocks, int maxStocks)
        : UnitBase("Player", startGrid, startScreen, number, stocks, maxStocks)
    {
        m_color = GetColor(100, 200, 255); // プレイヤーの色（水色）
    }

    void Player::DrawUnitGraphic() {
        int x = (int)m_screenPos.x;
        int y = (int)m_screenPos.y;

        // プレイヤーの本体描画
        DrawCircle(x, y, 25, m_color, TRUE);
        DrawCircle(x, y, 25, GetColor(255, 255, 255), FALSE);
        DrawFormatString(x - 8, y - 8, GetColor(255, 255, 255), "P");
    }

} // namespace App