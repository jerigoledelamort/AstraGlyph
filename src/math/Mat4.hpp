#pragma once

#include <array>
#include <cstddef>

#include "math/Vec3.hpp"

namespace astraglyph {

struct Mat4 {
  std::array<float, 16> values{};

  [[nodiscard]] constexpr float& operator()(int row, int column) noexcept
  {
    return values[static_cast<std::size_t>(row * 4 + column)];
  }

  [[nodiscard]] constexpr float operator()(int row, int column) const noexcept
  {
    return values[static_cast<std::size_t>(row * 4 + column)];
  }

  [[nodiscard]] static constexpr Mat4 identity() noexcept
  {
    Mat4 matrix{};
    matrix(0, 0) = 1.0F;
    matrix(1, 1) = 1.0F;
    matrix(2, 2) = 1.0F;
    matrix(3, 3) = 1.0F;
    return matrix;
  }

  [[nodiscard]] static constexpr Mat4 translation(const Vec3& offset) noexcept
  {
    Mat4 matrix = identity();
    matrix(0, 3) = offset.x;
    matrix(1, 3) = offset.y;
    matrix(2, 3) = offset.z;
    return matrix;
  }

  [[nodiscard]] static constexpr Mat4 scale(const Vec3& factors) noexcept
  {
    Mat4 matrix{};
    matrix(0, 0) = factors.x;
    matrix(1, 1) = factors.y;
    matrix(2, 2) = factors.z;
    matrix(3, 3) = 1.0F;
    return matrix;
  }

  [[nodiscard]] constexpr Vec3 transformPoint(const Vec3& point) const noexcept
  {
    return {
        (*this)(0, 0) * point.x + (*this)(0, 1) * point.y + (*this)(0, 2) * point.z + (*this)(0, 3),
        (*this)(1, 0) * point.x + (*this)(1, 1) * point.y + (*this)(1, 2) * point.z + (*this)(1, 3),
        (*this)(2, 0) * point.x + (*this)(2, 1) * point.y + (*this)(2, 2) * point.z + (*this)(2, 3),
    };
  }

  [[nodiscard]] constexpr Vec3 transformVector(const Vec3& vector) const noexcept
  {
    return {
        (*this)(0, 0) * vector.x + (*this)(0, 1) * vector.y + (*this)(0, 2) * vector.z,
        (*this)(1, 0) * vector.x + (*this)(1, 1) * vector.y + (*this)(1, 2) * vector.z,
        (*this)(2, 0) * vector.x + (*this)(2, 1) * vector.y + (*this)(2, 2) * vector.z,
    };
  }

  [[nodiscard]] constexpr Vec3 right() const noexcept
  {
    return {(*this)(0, 0), (*this)(1, 0), (*this)(2, 0)};
  }

  [[nodiscard]] constexpr Vec3 up() const noexcept
  {
    return {(*this)(0, 1), (*this)(1, 1), (*this)(2, 1)};
  }

  [[nodiscard]] constexpr Vec3 forward() const noexcept
  {
    return {(*this)(0, 2), (*this)(1, 2), (*this)(2, 2)};
  }
};

} // namespace astraglyph
