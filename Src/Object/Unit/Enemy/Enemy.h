#pragma once
#include "../UnitBase.h"

namespace App {

    class Enemy : public UnitBase {
    public:
        // 引数を5つに増やしました：グリッド, 画面, 数字, 初期ストック, 最大ストック
        Enemy(IntVector2 startGrid, Vector2 startScreen, int number, int stocks, int maxStocks = 5);
        virtual ~Enemy() = default;

    protected:
        // 敵の見た目を描画
        virtual void DrawUnitGraphic() override;
    };

} // namespace App