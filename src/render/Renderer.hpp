#pragma once

#include "core/ThreadPool.hpp"
#include "render/Integrator.hpp"
#include "render/RayTracer.hpp"
#include "render/RenderMetrics.hpp"
#include "render/TemporalAccumulator.hpp"
#include "scene/Camera.hpp"

#include <atomic>
#include <memory>
#include <vector>

namespace astraglyph {

struct AsciiFramebuffer;
struct RenderSettings;
struct Scene;

struct TileResult {
  RenderMetrics metrics{};
  std::uint64_t totalSamplesUsed{0};
  int maxAccumulatedFrames{0};
};

class Renderer {
public:
  Renderer() = default;
  ~Renderer() = default;

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  void render(
      AsciiFramebuffer& framebuffer,
      const Camera& camera,
      const Scene& scene,
      const RenderSettings& settings);

  const RenderMetrics& metrics() const noexcept;
  void setPresentationProfiling(double finalOutputMs, double totalRenderMs) noexcept;
  bool accumulationDirty() const noexcept;
  std::size_t sceneTriangleCount() const noexcept;

private:
  void renderTile(
      AsciiFramebuffer& framebuffer,
      const Camera& camera,
      const Scene& scene,
      const RenderSettings& settings,
      int yStart,
      int yEnd,
      bool accumulationValid,
      TileResult& result);

  static bool requiresAccumulationReset(
      const RenderSettings& current,
      const RenderSettings& previous) noexcept;
  static bool requiresAccumulationReset(
      const CameraState& current,
      const CameraState& previous) noexcept;
  static void mergeMetrics(RenderMetrics& dest, const RenderMetrics& src);

  Integrator integrator_{};
  RayTracer rayTracer_{};
  RenderMetrics metrics_{};
  RenderSettings appliedSettings_{};
  CameraState appliedCameraState_{};
  bool hasAppliedSettings_{false};
  bool accumulationDirty_{true};
  std::size_t sceneTriangleCount_{0};
  
  TemporalAccumulator temporalAccumulator_;

  // Thread pool для рендеринга (переиспользуется между кадрами)
  ThreadPool threadPool_;
  std::atomic<bool> threadPoolInitialized_{false};
};

} // namespace astraglyph
