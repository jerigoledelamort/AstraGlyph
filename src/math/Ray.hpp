#pragma once

#include "math/Vec3.hpp"

#include <limits>

namespace astraglyph {

struct Ray {
  Vec3 origin{};
  Vec3 direction{0.0F, 0.0F, -1.0F};
  float tMin{0.001F};
  float tMax{std::numeric_limits<float>::infinity()};

  constexpr Ray() noexcept = default;

  constexpr Ray(Vec3 rayOrigin, Vec3 rayDirection) noexcept
      : origin{rayOrigin}, direction{rayDirection}
  {
  }

  constexpr Ray(Vec3 rayOrigin, Vec3 rayDirection, float minDistance, float maxDistance) noexcept
      : origin{rayOrigin}, direction{rayDirection}, tMin{minDistance}, tMax{maxDistance}
  {
  }

  [[nodiscard]] constexpr Vec3 at(float t) const noexcept
  {
    return origin + direction * t;
  }
};

} // namespace astraglyph
