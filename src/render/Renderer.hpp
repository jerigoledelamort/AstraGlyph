#pragma once

#include "core/ThreadPool.hpp"
#include "render/AsciiFramebuffer.hpp"
#include "render/Integrator.hpp"
#include "render/RayTracer.hpp"
#include "render/RenderSettings.hpp"
#include "scene/Camera.hpp"
#include "scene/Scene.hpp"

#include <cstddef>
#include <vector>

namespace astraglyph {

struct TileResult {
  RenderMetrics metrics{};
  int maxAccumulatedFrames{0};
  std::uint64_t totalSamplesUsed{0};
};

class Renderer {
public:
  Renderer() = default;

  void render(
      AsciiFramebuffer& framebuffer,
      const Camera& camera,
      const Scene& scene,
      const RenderSettings& settings);
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
      TileResult& result);

  static void mergeMetrics(
      RenderMetrics& dest,
      const RenderMetrics& src);

  RayTracer rayTracer_{};
  Integrator integrator_{};
  ThreadPool threadPool_{};
  RenderMetrics metrics_{};
  RenderSettings appliedSettings_{};
  CameraState appliedCameraState_{};
  bool hasAppliedSettings_{false};
  bool accumulationDirty_{true};
  std::size_t sceneTriangleCount_{0};
};

} // namespace astraglyph
