#include "Vector2.h"
#include <cmath>

namespace App {

    // ==========================================
    // コンストラクタ
    // ==========================================

    // デフォルトコンストラクタ: 原点(0, 0)で初期化
    Vector2::Vector2() : x(0.0f), y(0.0f) {}

    // 引数付きコンストラクタ: 指定した座標で初期化
    Vector2::Vector2(float vX, float vY) : x(vX), y(vY) {}

    // デストラクタ
    Vector2::~Vector2() {}

    // ==========================================
    // 演算子オーバーロード: ベクトル同士の計算
    // ==========================================

    // 加算: 2つのベクトルを足す (例: (1,2) + (3,4) = (4,6))
    Vector2 Vector2::operator+(const Vector2& v) const {
        return Vector2(x + v.x, y + v.y);
    }

    // 減算: ベクトルを引く (例: (5,7) - (2,3) = (3,4))
    Vector2 Vector2::operator-(const Vector2& v) const {
        return Vector2(x - v.x, y - v.y);
    }

    // スカラー乗算: ベクトルを定数倍する (例: (2,3) * 2 = (4,6))
    Vector2 Vector2::operator*(float s) const {
        return Vector2(x * s, y * s);
    }

    // スカラー除算: ベクトルを定数で割る（ゼロ除算時は原点を返す）
    Vector2 Vector2::operator/(float s) const {
        return (s != 0) ? Vector2(x / s, y / s) : Vector2(0, 0);
    }

    // ==========================================
    // 複合代入演算子: 自身を変更する演算
    // ==========================================

    // 加算代入: 自身に別のベクトルを加算 (例: v1 += v2)
    Vector2& Vector2::operator+=(const Vector2& v) {
        x += v.x;
        y += v.y;
        return *this;
    }

    // 減算代入: 自身から別のベクトルを減算 (例: v1 -= v2)
    Vector2& Vector2::operator-=(const Vector2& v) {
        x -= v.x;
        y -= v.y;
        return *this;
    }

    // ==========================================
    // 比較演算子: ベクトルの等価判定
    // ==========================================

    // 等価演算子: 2つのベクトルが完全に一致するか判定
    bool Vector2::operator==(const Vector2& v) const {
        return (x == v.x && y == v.y);
    }

    // 不等価演算子: 2つのベクトルが異なるか判定
    bool Vector2::operator!=(const Vector2& v) const {
        return !(*this == v);
    }

    // ==========================================
    // 数学関数: ベクトルの幾何学的計算
    // ==========================================

    // ベクトルの長さ（大きさ）を計算: √(x? + y?)
    // 例: (3, 4)の長さは √(9 + 16) = 5
    float Vector2::Length() const {
        return std::sqrt(x * x + y * y);
    }

    // 正規化: ベクトルの向きは保ったまま長さを1にする
    // 用途: 方向だけが必要で大きさは不要な場合（移動方向など）
    // 例: (3, 4) → (0.6, 0.8)
    Vector2 Vector2::Normalize() const {
        float len = Length();

        // 長さが0でない場合のみ正規化（ゼロベクトルは原点のまま）
        if (len > 0) {
            return *this * (1.0f / len);
        }
        return Vector2(0.0f, 0.0f);
    }

    // 2点間の距離を計算（静的関数）: √((x?-x?)? + (y?-y?)?)
    // 用途: 2つのオブジェクト間の距離を測る（衝突判定、射程チェックなど）
    // 例: (1,2)と(4,6)の距離 = √((4-1)? + (6-2)?) = √(9+16) = 5
    float Vector2::Distance(const Vector2& v1, const Vector2& v2) {
        float dx = v2.x - v1.x;
        float dy = v2.y - v1.y;
        return std::sqrt(dx * dx + dy * dy);
    }

} // namespace App