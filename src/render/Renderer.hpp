#pragma once

#include "render/AsciiFramebuffer.hpp"
#include "render/Integrator.hpp"
#include "render/RayTracer.hpp"
#include "render/RenderSettings.hpp"
#include "scene/Camera.hpp"

namespace astraglyph {

class Renderer {
public:
  void render(AsciiFramebuffer& framebuffer, const Camera& camera, const RenderSettings& settings) const;
  [[nodiscard]] const RenderMetrics& metrics() const noexcept;
  [[nodiscard]] bool accumulationDirty() const noexcept;
  [[nodiscard]] std::size_t sceneTriangleCount() const noexcept;

private:
  [[nodiscard]] static bool requiresAccumulationReset(
      const RenderSettings& current,
      const RenderSettings& previous) noexcept;

  mutable RayTracer rayTracer_{};
  Integrator integrator_{};
  mutable RenderMetrics metrics_{};
  mutable RenderSettings appliedSettings_{};
  mutable bool hasAppliedSettings_{false};
  mutable bool accumulationDirty_{true};
  mutable std::size_t sceneTriangleCount_{0};
};

} // namespace astraglyph
