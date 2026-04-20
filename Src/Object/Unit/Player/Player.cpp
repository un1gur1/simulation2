#include "Player.h"
#include <DxLib.h>
#include <cmath>
#include <string>

namespace App {

	Player::Player(IntVector2 startGrid, Vector2 startScreen, int number, int stocks, int maxStocks)
		: UnitBase("Player", startGrid, startScreen, number, stocks, maxStocks)
	{
		// プレイヤーカラー：日本の伝統色「山吹色」
		m_color = GetColor(255, 165, 0);
	}

	void Player::DrawUnitGraphic() {
		double time = GetNowCount() / 1000.0;
		float bobbing = (float)(sin(time * 2.5) * 5.0);
		// ★pulse の計算を削除
		float x = m_screenPos.x;
		float y = m_screenPos.y;
		float unitY = y + bobbing;

		// 1. 影（足元の地面に固定）
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, (int)(110 - (bobbing + 5.0f) * 5.0f));
		DrawOvalAA(x, y + 28.0f, 22.0f - bobbing / 2.0f, 10.0f - bobbing / 4.0f, 64, GetColor(0, 0, 0), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0); // ★追加（影描画後のブレンドモードリセット）

		// 2. 脈動オーラ（★削除）

		// 3. 本体：山吹色クリスタル
		DrawCircleAA(x, unitY, 30.0f, 64, GetColor(80, 40, 0), TRUE);
		DrawCircleAA(x, unitY, 26.0f, 64, m_color, TRUE);

		SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
		DrawCircleAA(x, unitY, 24.0f, 64, GetColor(180, 100, 0), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

		DrawCircleAA(x, unitY, 26.0f, 64, GetColor(255, 255, 200), FALSE, 2.0f);

		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
		DrawCircleAA(x - 8.0f, unitY - 8.0f, 7.0f, 32, GetColor(255, 255, 255), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

		// 4. 数値表示
		int currentNum = GetNumber();
		SetFontSize(36);
		std::string numStr = std::to_string(currentNum);
		int numWidth = GetDrawStringWidth(numStr.c_str(), (int)numStr.length());
		DrawString((int)x - numWidth / 2 + 2, (int)unitY - 16 + 2, numStr.c_str(), GetColor(40, 20, 0));
		DrawString((int)x - numWidth / 2, (int)unitY - 16, numStr.c_str(), GetColor(255, 255, 255));

		// 5. バッジ
		char currentOp = GetOp();
		if (currentOp != '\0') {
			float bx = x + 20.0f;
			float by = unitY + 18.0f;
			SetDrawBlendMode(DX_BLENDMODE_ADD, 150);
			DrawCircleAA(bx, by, 15.0f, 32, GetColor(255, 180, 50), TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
			DrawCircleAA(bx, by, 13.0f, 32, GetColor(255, 255, 255), TRUE);
			DrawCircleAA(bx, by, 13.0f, 32, GetColor(0, 0, 0), FALSE, 2.0f);
			SetFontSize(22);
			char opStr[2] = { currentOp, '\0' };
			int opW = GetDrawStringWidth(opStr, 1);
			DrawString((int)bx - opW / 2, (int)by - 11, opStr, GetColor(0, 0, 0));
		}
	}

} // namespace App