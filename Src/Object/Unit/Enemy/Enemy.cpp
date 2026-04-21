#include "Enemy.h"
#include <DxLib.h>
#include <cmath>
#include <string>

namespace App {

	// ==========================================
	// コンストラクタ: 敵ユニットの初期化
	// ==========================================
	Enemy::Enemy(IntVector2 startGrid, Vector2 startScreen, int number, int stocks, int maxStocks)
		: UnitBase("Enemy", startGrid, startScreen, number, stocks, maxStocks)
	{
		// 敵の色: 鮮やかな紺色（プレイヤーのオレンジと対比）
		m_color = GetColor(20, 40, 160);
	}

	// ==========================================
	// 敵ユニットの描画: 紺色クリスタルの浮遊表現
	// ==========================================
	void Enemy::DrawUnitGraphic() {
		// 時間に基づくアニメーション計算
		double time = GetNowCount() / 1000.0;  // 現在時刻（秒）
		float bobbing = (float)(sin(time * 2.0) * 4.0);  // 上下浮遊（-4.0 ~ +4.0）

		// 座標計算
		float x = m_screenPos.x;        // 画面X座標
		float y = m_screenPos.y;        // 画面Y座標（基準位置）
		float unitY = y + bobbing;      // 浮遊を加えた実際のY座標

		// ==========================================
		// 1. 影の描画（足元に固定）
		// ==========================================
		// 浮遊に応じて影の濃さと大きさを変化させる
		// 高く浮くほど影は薄く小さくなる
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, (int)(130 - (bobbing + 4.0f) * 5.0f));
		DrawOvalAA(
			x, y + 28.0f,                        // 影の位置（足元固定）
			24.0f - bobbing / 2.0f,              // 横幅（浮遊で変化）
			12.0f - bobbing / 4.0f,              // 縦幅（浮遊で変化）
			64,                                   // 分割数（滑らかさ）
			GetColor(0, 0, 0),                   // 黒
			TRUE                                  // 塗りつぶし
		);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

		// ==========================================
		// 2. 本体: 紺色クリスタルの描画
		// ==========================================

		// 外側の暗い縁取り（立体感）
		DrawCircleAA(x, unitY, 30.0f, 64, GetColor(10, 15, 50), TRUE);

		// メインボディ（紺色）
		DrawCircleAA(x, unitY, 26.0f, 64, m_color, TRUE);

		// 内側の光（加算合成で光沢表現）
		SetDrawBlendMode(DX_BLENDMODE_ADD, 100);
		DrawCircleAA(x, unitY, 24.0f, 64, GetColor(20, 60, 150), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

		// 輪郭線（明るい青で縁取り）
		DrawCircleAA(x, unitY, 26.0f, 64, GetColor(180, 220, 255), FALSE, 2.0f);

		// ハイライト（左上の光反射表現）
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 160);
		DrawCircleAA(x - 8.0f, unitY - 8.0f, 7.0f, 32, GetColor(200, 220, 255), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

		// ==========================================
		// 3. 数値表示: ユニットの現在値
		// ==========================================
		int currentNum = GetNumber();
		SetFontSize(36);
		std::string numStr = std::to_string(currentNum);
		int numWidth = GetDrawStringWidth(numStr.c_str(), (int)numStr.length());

		// 影付きテキスト（視認性向上）
		DrawString(
			(int)x - numWidth / 2 + 2,          // 影のX座標（少しずらす）
			(int)unitY - 16 + 2,                // 影のY座標（少しずらす）
			numStr.c_str(),
			GetColor(0, 0, 40)                   // 暗い青（影）
		);
		DrawString(
			(int)x - numWidth / 2,              // メインのX座標（中央揃え）
			(int)unitY - 16,                    // メインのY座標
			numStr.c_str(),
			GetColor(255, 255, 255)              // 白（本体）
		);

		// ==========================================
		// 4. 演算子バッジ: 取得中のアイテム表示
		// ==========================================
		char currentOp = GetOp();
		if (currentOp != '\0') {  // 演算子を持っている場合のみ描画
			// バッジの配置（右下）
			float bx = x + 20.0f;
			float by = unitY + 18.0f;

			// 後光エフェクト（加算合成で発光）
			SetDrawBlendMode(DX_BLENDMODE_ADD, 130);
			DrawCircleAA(bx, by, 15.0f, 32, GetColor(255, 220, 50), TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

			// バッジ本体（白い円）
			DrawCircleAA(bx, by, 13.0f, 32, GetColor(255, 255, 255), TRUE);

			// バッジの縁取り（暗い青）
			DrawCircleAA(bx, by, 13.0f, 32, GetColor(0, 0, 40), FALSE, 2.0f);

			// 演算子記号の描画
			SetFontSize(22);
			char opStr[2] = { currentOp, '\0' };  // 文字列化
			int opW = GetDrawStringWidth(opStr, 1);
			DrawString(
				(int)bx - opW / 2,              // 中央揃え
				(int)by - 11,                   // 垂直中央
				opStr,
				GetColor(0, 0, 0)                // 黒（視認性）
			);
		}
	}

} // namespace App