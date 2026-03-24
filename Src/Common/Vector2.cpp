#include "Vector2.h"
#include <cmath>

namespace App {

    // --- コンストラクタ ---
    Vector2::Vector2() : x(0.0f), y(0.0f) {}
    Vector2::Vector2(float vX, float vY) : x(vX), y(vY) {}
    Vector2::~Vector2() {}

    // --- 演算子オーバーロード ---
    Vector2 Vector2::operator+(const Vector2& v) const { return Vector2(x + v.x, y + v.y); }
    Vector2 Vector2::operator-(const Vector2& v) const { return Vector2(x - v.x, y - v.y); }
    Vector2 Vector2::operator*(float s) const { return Vector2(x * s, y * s); }
    Vector2 Vector2::operator/(float s) const { return (s != 0) ? Vector2(x / s, y / s) : Vector2(0, 0); }

    Vector2& Vector2::operator+=(const Vector2& v) {
        x += v.x; y += v.y;
        return *this;
    }

    Vector2& Vector2::operator-=(const Vector2& v) {
        x -= v.x; y -= v.y;
        return *this;
    }

    bool Vector2::operator==(const Vector2& v) const { return (x == v.x && y == v.y); }
    bool Vector2::operator!=(const Vector2& v) const { return !(*this == v); }

    // --- 数学関数 ---

    // ベクトルの長さ: $L = \sqrt{x^2 + y^2}$
    float Vector2::Length() const {
        return std::sqrt(x * x + y * y);
    }

    // 正規化（長さを1にする）
    Vector2 Vector2::Normalize() const {
        float len = Length();
        if (len > 0) {
            return *this * (1.0f / len);
        }
        return Vector2(0.0f, 0.0f);
    }

    // 2点間の距離: $D = \sqrt{(x_2-x_1)^2 + (y_2-y_1)^2}$
    float Vector2::Distance(const Vector2& v1, const Vector2& v2) {
        float dx = v2.x - v1.x;
        float dy = v2.y - v1.y;
        return std::sqrt(dx * dx + dy * dy);
    }

} // namespace App