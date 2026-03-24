#pragma once
#include "../UnitBase.h"

namespace App {

    class Player : public UnitBase {
    public:
        // 引数を5つに：グリッド位置、画面位置、数字、初期ストック, 最大ストック
        Player(IntVector2 startGrid, Vector2 startScreen, int number, int stocks, int maxStocks);
        virtual ~Player() = default;

    protected:
        // プレイヤーの見た目（青い円など）を描画
        virtual void DrawUnitGraphic() override;
    };

} // namespace App