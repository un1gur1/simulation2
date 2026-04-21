#pragma once

namespace App {

    // ==========================================
    // IntVector2: グリッド座標用の整数ベクトル
    // 用途: マス目の位置管理（例: (3, 5) = 3列目5行目）
    // ==========================================
    struct IntVector2 {
        int x;  // X座標（横）
        int y;  // Y座標（縦）

        // 座標の一致判定（経路探索や位置チェック用）
        bool operator==(const IntVector2& other) const { return (x == other.x && y == other.y); }
        bool operator!=(const IntVector2& other) const { return !(*this == other); }
    };

    // ==========================================
    // Vector2: 画面座標用の浮動小数点ベクトル
    // 用途: ピクセル座標、移動アニメーション、物理計算
    // ==========================================
    class Vector2 {
    public:
        float x;  // X座標
        float y;  // Y座標

        // コンストラクタ
        Vector2();
        Vector2(float vX, float vY);
        ~Vector2();

        // 算術演算子（ベクトル計算用）
        Vector2 operator+(const Vector2& v) const;  // 加算
        Vector2 operator-(const Vector2& v) const;  // 減算
        Vector2 operator*(float s) const;           // スカラー倍
        Vector2 operator/(float s) const;           // スカラー除算

        // 代入演算子（累積計算用）
        Vector2& operator+=(const Vector2& v);
        Vector2& operator-=(const Vector2& v);

        // 比較演算子
        bool operator==(const Vector2& v) const;
        bool operator!=(const Vector2& v) const;

        // 数学関数
        float Length() const;           // ベクトルの長さ
        Vector2 Normalize() const;      // 正規化（長さを1に）

        // 静的関数
        static float Distance(const Vector2& v1, const Vector2& v2);  // 2点間の距離
    };

} // namespace App