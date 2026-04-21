#pragma once
#include "../UnitBase.h"

namespace App {

    // ==========================================
    // Player: プレイヤーユニット（1P側）
    // 用途: プレイヤーが操作するキャラクター
    // 継承: UnitBaseの機能を全て持つ + プレイヤー固有の見た目
    // ==========================================
    class Player : public UnitBase {
    public:
        // ==========================================
        // コンストラクタ
        // 引数:
        //   startGrid   - グリッド座標（マス目の位置）
        //   startScreen - 画面座標（ピクセル位置）
        //   number      - 初期数値（体力）
        //   stocks      - 現在の残機数
        //   maxStocks   - 最大残機数
        // ==========================================
        Player(IntVector2 startGrid, Vector2 startScreen, int number, int stocks, int maxStocks);

        // デストラクタ（仮想デストラクタ）
        virtual ~Player() = default;

    protected:
        // ==========================================
        // 描画メソッド: プレイヤーの見た目を描画
        // オーバーライド: UnitBase::DrawUnitGraphic()
        // 用途: プレイヤー専用のグラフィック・色・エフェクト
        // ==========================================
        virtual void DrawUnitGraphic() override;
    };

} // namespace App