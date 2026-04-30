#pragma once

#include "math/Ray.hpp"
#include "math/Vec3.hpp"
#include "render/RenderMetrics.hpp"
#include "render/RayTracer.hpp"
#include "render/RenderSettings.hpp"
#include "scene/Scene.hpp"

namespace astraglyph {

class Integrator {
public:
  [[nodiscard]] Vec3 traceRadiance(
      const Ray& ray,
      const Scene& scene,
      const RayTracer& rayTracer,
      const RenderSettings& settings,
      RenderMetrics& metrics,
      HitInfo* outHit = nullptr,
      int depth = 0) const;

  [[nodiscard]] Vec3 radiance(const Scene& scene, const Ray& ray) const;
};

} // namespace astraglyph
