#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

#include "math/Ray.hpp"
#include "math/Vec3.hpp"

namespace astraglyph {

struct Aabb {
  Vec3 min{
      std::numeric_limits<float>::infinity(),
      std::numeric_limits<float>::infinity(),
      std::numeric_limits<float>::infinity(),
  };
  Vec3 max{
      -std::numeric_limits<float>::infinity(),
      -std::numeric_limits<float>::infinity(),
      -std::numeric_limits<float>::infinity(),
  };

  [[nodiscard]] constexpr bool empty() const noexcept
  {
    return min.x > max.x || min.y > max.y || min.z > max.z;
  }

  void expand(const Vec3& point) noexcept
  {
    min = componentMin(min, point);
    max = componentMax(max, point);
  }

  void expand(const Aabb& other) noexcept
  {
    if (other.empty()) {
      return;
    }

    expand(other.min);
    expand(other.max);
  }

  [[nodiscard]] constexpr Vec3 centroid() const noexcept
  {
    return empty() ? Vec3{} : (min + max) * 0.5F;
  }

  [[nodiscard]] constexpr Vec3 extent() const noexcept
  {
    return empty() ? Vec3{} : (max - min);
  }

  [[nodiscard]] constexpr float surfaceArea() const noexcept
  {
    const Vec3 e = extent();
    return 2.0F * (e.x * e.y + e.y * e.z + e.z * e.x);
  }

  [[nodiscard]] constexpr int longestAxis() const noexcept
  {
    const Vec3 e = extent();
    if (e.x >= e.y && e.x >= e.z) {
      return 0;
    }
    if (e.y >= e.z) {
      return 1;
    }
    return 2;
  }

  [[nodiscard]] bool intersect(const Ray& ray, float& outTMin, float& outTMax) const noexcept
  {
    if (empty()) {
      return false;
    }

    float tMin = ray.tMin;
    float tMax = ray.tMax;

    const auto updateAxis = [&](float origin, float direction, float minBound, float maxBound) -> bool {
      constexpr float epsilon = 1.0e-8F;
      if (std::fabs(direction) <= epsilon) {
        return origin >= minBound && origin <= maxBound;
      }

      const float invDirection = 1.0F / direction;
      float t0 = (minBound - origin) * invDirection;
      float t1 = (maxBound - origin) * invDirection;
      if (t0 > t1) {
        std::swap(t0, t1);
      }

      tMin = std::max(tMin, t0);
      tMax = std::min(tMax, t1);
      return tMax >= tMin;
    };

    if (!updateAxis(ray.origin.x, ray.direction.x, min.x, max.x)) {
      return false;
    }
    if (!updateAxis(ray.origin.y, ray.direction.y, min.y, max.y)) {
      return false;
    }
    if (!updateAxis(ray.origin.z, ray.direction.z, min.z, max.z)) {
      return false;
    }

    outTMin = tMin;
    outTMax = tMax;
    return true;
  }
};

} // namespace astraglyph
