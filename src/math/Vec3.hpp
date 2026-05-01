#pragma once

#include <algorithm>
#include <cmath>

namespace astraglyph {

struct Vec3 {
  float x{0.0F};
  float y{0.0F};
  float z{0.0F};

  constexpr Vec3() noexcept = default;
  constexpr Vec3(float xValue, float yValue, float zValue) noexcept
      : x{xValue}, y{yValue}, z{zValue}
  {
  }

  [[nodiscard]] constexpr Vec3 operator+(const Vec3& rhs) const noexcept
  {
    return {x + rhs.x, y + rhs.y, z + rhs.z};
  }

  [[nodiscard]] constexpr Vec3 operator-(const Vec3& rhs) const noexcept
  {
    return {x - rhs.x, y - rhs.y, z - rhs.z};
  }

  [[nodiscard]] constexpr Vec3 operator-() const noexcept
  {
    return {-x, -y, -z};
  }

  [[nodiscard]] constexpr Vec3 operator*(float scalar) const noexcept
  {
    return {x * scalar, y * scalar, z * scalar};
  }

  [[nodiscard]] constexpr Vec3 operator*(const Vec3& rhs) const noexcept
  {
    return {x * rhs.x, y * rhs.y, z * rhs.z};
  }

  [[nodiscard]] constexpr Vec3 operator/(float scalar) const noexcept
  {
    return {x / scalar, y / scalar, z / scalar};
  }

  constexpr Vec3& operator+=(const Vec3& rhs) noexcept
  {
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
  }

  constexpr Vec3& operator-=(const Vec3& rhs) noexcept
  {
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
  }

  constexpr Vec3& operator*=(float scalar) noexcept
  {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
  }

  constexpr Vec3& operator/=(float scalar) noexcept
  {
    x /= scalar;
    y /= scalar;
    z /= scalar;
    return *this;
  }

  [[nodiscard]] constexpr bool operator==(const Vec3& rhs) const noexcept = default;
  [[nodiscard]] constexpr bool operator!=(const Vec3& rhs) const noexcept = default;

  [[nodiscard]] constexpr float lengthSquared() const noexcept
  {
    return x * x + y * y + z * z;
  }

  [[nodiscard]] float length() const noexcept
  {
    return std::sqrt(lengthSquared());
  }

  [[nodiscard]] Vec3 normalized() const noexcept
  {
    const float valueLength = length();
    return valueLength > 0.0F ? (*this / valueLength) : Vec3{};
  }

  void normalize() noexcept
  {
    *this = normalized();
  }
};

[[nodiscard]] constexpr Vec3 operator*(float scalar, const Vec3& vector) noexcept
{
  return vector * scalar;
}

[[nodiscard]] constexpr float dot(const Vec3& lhs, const Vec3& rhs) noexcept
{
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

[[nodiscard]] constexpr Vec3 cross(const Vec3& lhs, const Vec3& rhs) noexcept
{
  return {
      lhs.y * rhs.z - lhs.z * rhs.y,
      lhs.z * rhs.x - lhs.x * rhs.z,
      lhs.x * rhs.y - lhs.y * rhs.x,
  };
}

[[nodiscard]] constexpr Vec3 componentMin(const Vec3& lhs, const Vec3& rhs) noexcept
{
  return {
      lhs.x < rhs.x ? lhs.x : rhs.x,
      lhs.y < rhs.y ? lhs.y : rhs.y,
      lhs.z < rhs.z ? lhs.z : rhs.z,
  };
}

[[nodiscard]] constexpr Vec3 componentMax(const Vec3& lhs, const Vec3& rhs) noexcept
{
  return {
      lhs.x > rhs.x ? lhs.x : rhs.x,
      lhs.y > rhs.y ? lhs.y : rhs.y,
      lhs.z > rhs.z ? lhs.z : rhs.z,
  };
}

[[nodiscard]] inline float length(const Vec3& value) noexcept
{
  return value.length();
}

[[nodiscard]] inline float lengthSquared(const Vec3& value) noexcept
{
  return value.lengthSquared();
}

[[nodiscard]] inline Vec3 normalize(const Vec3& value) noexcept
{
  return value.normalized();
}

[[nodiscard]] constexpr Vec3 reflect(const Vec3& direction, const Vec3& normal) noexcept
{
  return direction - 2.0F * dot(direction, normal) * normal;
}

[[nodiscard]] inline Vec3 clamp(const Vec3& value, const Vec3& minValue, const Vec3& maxValue) noexcept
{
  return {
      std::clamp(value.x, minValue.x, maxValue.x),
      std::clamp(value.y, minValue.y, maxValue.y),
      std::clamp(value.z, minValue.z, maxValue.z),
  };
}

[[nodiscard]] inline Vec3 lerp(const Vec3& a, const Vec3& b, float t) noexcept
{
  return a + (b - a) * t;
}

} // namespace astraglyph
