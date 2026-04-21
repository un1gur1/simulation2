#pragma once
#include "../UnitBase.h"

namespace App {

    // ==========================================
    // Enemy: 敵ユニット（2P側）
    // 用途: プレイヤーと対戦する敵キャラクター
    // 継承: UnitBaseの機能を全て持つ + 敵固有の見た目
    // ==========================================
    class Enemy : public UnitBase {
    public:
        // ==========================================
        // コンストラクタ
        // 引数:
        //   startGrid   - グリッド座標（マス目の位置）
        //   startScreen - 画面座標（ピクセル位置）
        //   number      - 初期数値（体力）
        //   stocks      - 現在の残機数
        //   maxStocks   - 最大残機数（デフォルト: 5）
        // ==========================================
        Enemy(IntVector2 startGrid, Vector2 startScreen, int number, int stocks, int maxStocks = 5);

        // デストラクタ（仮想デストラクタ）
        virtual ~Enemy() = default;

    protected:
        // ==========================================
        // 描画メソッド: 敵の見た目を描画
        // オーバーライド: UnitBase::DrawUnitGraphic()
        // 用途: 敵専用のグラフィック・色・エフェクト
        // ==========================================
        virtual void DrawUnitGraphic() override;
    };

} // namespace App