#pragma once

namespace App {

    // --- 盤面（グリッド）座標用 ---
    struct IntVector2 {
        int x;
        int y;

        // 比較演算子（ルートの逆流防止や隣接判定で使用）
        bool operator==(const IntVector2& other) const { return (x == other.x && y == other.y); }
        bool operator!=(const IntVector2& other) const { return !(*this == other); }
    };

    // --- 画面座標・物理計算用 ---
    class Vector2 {
    public:
        float x;
        float y;

        // コンストラクタ
        Vector2();
        Vector2(float vX, float vY);
        ~Vector2();

        // 算術演算子
        Vector2 operator+(const Vector2& v) const;
        Vector2 operator-(const Vector2& v) const;
        Vector2 operator*(float s) const;
        Vector2 operator/(float s) const;

        // 代入演算子
        Vector2& operator+=(const Vector2& v);
        Vector2& operator-=(const Vector2& v);

        // 比較演算子
        bool operator==(const Vector2& v) const;
        bool operator!=(const Vector2& v) const;

        // 数学関数
        float Length() const;
        Vector2 Normalize() const;

        // 静的関数
        static float Distance(const Vector2& v1, const Vector2& v2);
    };

} // namespace App