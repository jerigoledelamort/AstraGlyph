#pragma once

#include "render/AsciiFramebuffer.hpp"
#include "render/Integrator.hpp"
#include "render/RayTracer.hpp"
#include "render/RenderSettings.hpp"
#include "scene/Camera.hpp"
#include "scene/Scene.hpp"

namespace astraglyph {

struct TileResult {
  RenderMetrics metrics{};
  int maxAccumulatedFrames{0};
  std::uint64_t totalSamplesUsed{0};
};

class Renderer {
public:
  void render(
      AsciiFramebuffer& framebuffer,
      const Camera& camera,
      const Scene& scene,
      const RenderSettings& settings) const;
  [[nodiscard]] const RenderMetrics& metrics() const noexcept;
  [[nodiscard]] bool accumulationDirty() const noexcept;
  [[nodiscard]] std::size_t sceneTriangleCount() const noexcept;

private:
  [[nodiscard]] static bool requiresAccumulationReset(
      const RenderSettings& current,
      const RenderSettings& previous) noexcept;
  [[nodiscard]] static bool requiresAccumulationReset(
      const CameraState& current,
      const CameraState& previous) noexcept;

  void renderTile(
      AsciiFramebuffer& framebuffer,
      const Camera& camera,
      const Scene& scene,
      const RenderSettings& settings,
      int yStart,
      int yEnd,
      bool accumulationDirty,
      TileResult& result) const;

  static void mergeMetrics(
      RenderMetrics& dest,
      const RenderMetrics& src);

  mutable RayTracer rayTracer_{};
  Integrator integrator_{};
  mutable RenderMetrics metrics_{};
  mutable RenderSettings appliedSettings_{};
  mutable CameraState appliedCameraState_{};
  mutable bool hasAppliedSettings_{false};
  mutable bool accumulationDirty_{true};
  mutable std::size_t sceneTriangleCount_{0};
};

} // namespace astraglyph
