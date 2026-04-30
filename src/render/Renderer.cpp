#include "render/Renderer.hpp"

#include "render/AsciiMapper.hpp"
#include "render/Sampler.hpp"
#include "scene/Scene.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace astraglyph {
namespace {

struct RunningVariance {
  int count{0};
  double mean{0.0};
  double m2{0.0};

  void add(float sample) noexcept
  {
    ++count;
    const double delta = static_cast<double>(sample) - mean;
    mean += delta / static_cast<double>(count);
    const double delta2 = static_cast<double>(sample) - mean;
    m2 += delta * delta2;
  }

  [[nodiscard]] float variance() const noexcept
  {
    if (count < 2) {
      return 0.0F;
    }
    return static_cast<float>(m2 / static_cast<double>(count - 1));
  }
};

[[nodiscard]] int clampConfiguredSamples(int value) noexcept
{
  return std::clamp(value, 1, 16);
}

} // namespace

const RenderMetrics& Renderer::metrics() const noexcept
{
  return metrics_;
}

bool Renderer::accumulationDirty() const noexcept
{
  return accumulationDirty_;
}

std::size_t Renderer::sceneTriangleCount() const noexcept
{
  return sceneTriangleCount_;
}

bool Renderer::requiresAccumulationReset(
    const RenderSettings& current,
    const RenderSettings& previous) noexcept
{
  return current.gridWidth != previous.gridWidth ||
         current.gridHeight != previous.gridHeight ||
         current.samplesPerCell != previous.samplesPerCell ||
         current.maxSamplesPerCell != previous.maxSamplesPerCell ||
         current.jitteredSampling != previous.jitteredSampling ||
         current.adaptiveSampling != previous.adaptiveSampling ||
         current.adaptiveVarianceThreshold != previous.adaptiveVarianceThreshold ||
         current.temporalAccumulation != previous.temporalAccumulation ||
         current.enableShadows != previous.enableShadows ||
         current.enableSoftShadows != previous.enableSoftShadows ||
         current.shadowSamples != previous.shadowSamples ||
         current.enableReflections != previous.enableReflections ||
         current.maxBounces != previous.maxBounces ||
         current.enableBvh != previous.enableBvh ||
         current.backfaceCulling != previous.backfaceCulling ||
         current.bvhLeafSize != previous.bvhLeafSize ||
         current.exposure != previous.exposure ||
         current.gamma != previous.gamma ||
         current.colorOutput != previous.colorOutput ||
         current.glyphRampMode != previous.glyphRampMode ||
         current.ambientStrength != previous.ambientStrength ||
         current.shadowBias != previous.shadowBias ||
         current.reflectionBias != previous.reflectionBias ||
         current.backgroundColor.x != previous.backgroundColor.x ||
         current.backgroundColor.y != previous.backgroundColor.y ||
         current.backgroundColor.z != previous.backgroundColor.z;
}

void Renderer::render(AsciiFramebuffer& framebuffer, const Camera& camera, const RenderSettings& settings) const
{
  RenderSettings activeSettings = settings;
  activeSettings.validate();

  accumulationDirty_ = !hasAppliedSettings_ ||
                       activeSettings.accumulationDirty ||
                       requiresAccumulationReset(activeSettings, appliedSettings_);
  appliedSettings_ = activeSettings;
  hasAppliedSettings_ = true;

  const int targetWidth = std::max(activeSettings.gridWidth, 1);
  const int targetHeight = std::max(activeSettings.gridHeight, 1);
  if (framebuffer.width() != static_cast<std::size_t>(targetWidth) ||
      framebuffer.height() != static_cast<std::size_t>(targetHeight)) {
    framebuffer.resize(static_cast<std::size_t>(targetWidth), static_cast<std::size_t>(targetHeight));
  }

  framebuffer.clear();
  metrics_ = RenderMetrics{};
  metrics_.totalCells = static_cast<std::uint64_t>(targetWidth) * static_cast<std::uint64_t>(targetHeight);

  static const Scene scene = Scene::createDefaultScene();
  sceneTriangleCount_ = scene.triangles().size();
  if (activeSettings.enableBvh) {
    rayTracer_.buildAcceleration(scene.triangles(), activeSettings.bvhLeafSize);
  }

  AsciiMapper mapper;
  Sampler sampler;
  const int baseSamples = clampConfiguredSamples(activeSettings.samplesPerCell);
  const int maxSamples = std::max(baseSamples, clampConfiguredSamples(activeSettings.maxSamplesPerCell));
  std::uint64_t totalSamplesUsed = 0;

  for (int y = 0; y < targetHeight; ++y) {
    for (int x = 0; x < targetWidth; ++x) {
      Vec3 radianceSum{};
      Vec3 displaySum{};
      RunningVariance luminanceVariance{};
      double depthSum = 0.0;
      int depthSamples = 0;
      int sampleCount = 0;

      const auto traceSample = [&](int sampleIndex) {
        const Sample sample = sampler.generateSubCellSample(x, y, sampleIndex, activeSettings);
        const float u =
            (static_cast<float>(x) + sample.uv.x) / static_cast<float>(targetWidth);
        const float v =
            (static_cast<float>(y) + sample.uv.y) / static_cast<float>(targetHeight);
        const Ray ray = camera.generateRay(u, v);
        HitInfo hit{};
        const Vec3 radiance = integrator_.traceRadiance(ray, scene, rayTracer_, activeSettings, metrics_, &hit);

        Vec3 display = mapper.applyExposureGamma(radiance, activeSettings.exposure, activeSettings.gamma);
        const float luminance = mapper.radianceToLuminance(display);
        if (!activeSettings.colorOutput) {
          display = Vec3{luminance, luminance, luminance};
        }

        radianceSum += radiance;
        displaySum += display;
        luminanceVariance.add(luminance);
        if (hit.hit) {
          depthSum += hit.t;
          ++depthSamples;
        }
        ++sampleCount;
      };

      for (int sampleIndex = 0; sampleIndex < baseSamples; ++sampleIndex) {
        traceSample(sampleIndex);
      }

      bool usedAdaptiveSampling = false;
      if (activeSettings.adaptiveSampling && sampleCount < maxSamples &&
          luminanceVariance.variance() > activeSettings.adaptiveVarianceThreshold) {
        usedAdaptiveSampling = true;
        for (int sampleIndex = sampleCount; sampleIndex < maxSamples; ++sampleIndex) {
          traceSample(sampleIndex);
        }
      }

      if (usedAdaptiveSampling) {
        ++metrics_.adaptiveCells;
      }

      totalSamplesUsed += static_cast<std::uint64_t>(sampleCount);
      metrics_.maxSamplesUsed = std::max(metrics_.maxSamplesUsed, sampleCount);

      const float inverseSampleCount = 1.0F / static_cast<float>(sampleCount);
      const Vec3 averageRadiance = radianceSum * inverseSampleCount;
      const Vec3 averageDisplay = displaySum * inverseSampleCount;
      const float averageLuminance = static_cast<float>(luminanceVariance.mean);
      const float depth = depthSamples > 0
                              ? static_cast<float>(depthSum / static_cast<double>(depthSamples))
                              : std::numeric_limits<float>::infinity();

      AsciiCell& cell = framebuffer.at(static_cast<std::size_t>(x), static_cast<std::size_t>(y));
      cell.radiance = averageRadiance;
      cell.luminance = averageLuminance;
      cell.glyph = mapper.mapLuminanceToGlyph(averageLuminance, activeSettings.glyphRampMode);
      cell.fg = averageDisplay;
      cell.bg = Vec3{};
      cell.depth = depth;
      cell.sampleCount = sampleCount;
    }
  }

  if (metrics_.totalCells > 0U) {
    metrics_.averageSamplesPerCell =
        static_cast<float>(static_cast<double>(totalSamplesUsed) / static_cast<double>(metrics_.totalCells));
  }
}

} // namespace astraglyph
