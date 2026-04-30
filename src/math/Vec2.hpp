#pragma once

#include <algorithm>
#include <cmath>

namespace astraglyph {

struct Vec2 {
  float x{0.0F};
  float y{0.0F};

  constexpr Vec2() noexcept = default;
  constexpr Vec2(float xValue, float yValue) noexcept
      : x{xValue}, y{yValue}
  {
  }

  [[nodiscard]] constexpr Vec2 operator+(const Vec2& rhs) const noexcept
  {
    return {x + rhs.x, y + rhs.y};
  }

  [[nodiscard]] constexpr Vec2 operator-(const Vec2& rhs) const noexcept
  {
    return {x - rhs.x, y - rhs.y};
  }

  [[nodiscard]] constexpr Vec2 operator*(float scalar) const noexcept
  {
    return {x * scalar, y * scalar};
  }

  [[nodiscard]] constexpr Vec2 operator/(float scalar) const noexcept
  {
    return {x / scalar, y / scalar};
  }

  constexpr Vec2& operator+=(const Vec2& rhs) noexcept
  {
    x += rhs.x;
    y += rhs.y;
    return *this;
  }

  constexpr Vec2& operator-=(const Vec2& rhs) noexcept
  {
    x -= rhs.x;
    y -= rhs.y;
    return *this;
  }

  constexpr Vec2& operator*=(float scalar) noexcept
  {
    x *= scalar;
    y *= scalar;
    return *this;
  }

  constexpr Vec2& operator/=(float scalar) noexcept
  {
    x /= scalar;
    y /= scalar;
    return *this;
  }

  [[nodiscard]] constexpr float lengthSquared() const noexcept
  {
    return x * x + y * y;
  }

  [[nodiscard]] float length() const noexcept
  {
    return std::sqrt(lengthSquared());
  }

  [[nodiscard]] Vec2 normalized() const noexcept
  {
    const float valueLength = length();
    return valueLength > 0.0F ? (*this / valueLength) : Vec2{};
  }

  void normalize() noexcept
  {
    *this = normalized();
  }
};

[[nodiscard]] constexpr Vec2 operator*(float scalar, const Vec2& vector) noexcept
{
  return vector * scalar;
}

[[nodiscard]] constexpr float dot(const Vec2& lhs, const Vec2& rhs) noexcept
{
  return lhs.x * rhs.x + lhs.y * rhs.y;
}

[[nodiscard]] inline float lengthSquared(const Vec2& value) noexcept
{
  return value.lengthSquared();
}

[[nodiscard]] inline float length(const Vec2& value) noexcept
{
  return value.length();
}

[[nodiscard]] inline Vec2 normalize(const Vec2& value) noexcept
{
  return value.normalized();
}

[[nodiscard]] inline Vec2 lerp(const Vec2& a, const Vec2& b, float t) noexcept
{
  return a + (b - a) * t;
}

[[nodiscard]] inline Vec2 clamp(const Vec2& value, const Vec2& minValue, const Vec2& maxValue) noexcept
{
  return {
      std::clamp(value.x, minValue.x, maxValue.x),
      std::clamp(value.y, minValue.y, maxValue.y),
  };
}

} // namespace astraglyph
