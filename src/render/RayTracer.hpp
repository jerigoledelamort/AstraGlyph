#pragma once

#include "geometry/Triangle.hpp"
#include "math/Ray.hpp"
#include "render/Bvh.hpp"
#include "render/RenderSettings.hpp"

#include <cstddef>
#include <vector>

namespace astraglyph {

class RayTracer {
public:
  void buildAcceleration(const std::vector<Triangle>& triangles, int leafSize = 4);

  [[nodiscard]] HitInfo traceClosest(
      const Ray& ray,
      const std::vector<Triangle>& triangles,
      const RenderSettings& settings) const noexcept;

  [[nodiscard]] HitInfo traceClosest(
      const Ray& ray,
      const std::vector<Triangle>& triangles,
      const RenderSettings& settings,
      RenderMetrics* metrics) const noexcept;

private:
  [[nodiscard]] HitInfo traceClosestBruteForce(
      const Ray& ray,
      const std::vector<Triangle>& triangles,
      const RenderSettings& settings,
      RenderMetrics* metrics) const noexcept;
  [[nodiscard]] bool canUseAcceleration(
      const std::vector<Triangle>& triangles,
      const RenderSettings& settings) const noexcept;

  Bvh bvh_{};
  const Triangle* accelerationSource_{nullptr};
  std::size_t accelerationTriangleCount_{0};
  int accelerationLeafSize_{4};
  bool accelerationReady_{false};
};

} // namespace astraglyph
