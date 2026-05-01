#include "render/Renderer.hpp"

#include "render/AsciiMapper.hpp"
#include "render/Sampler.hpp"
#include "scene/Scene.hpp"

#include <algorithm>
#include <cstdint>
#include <future>
#include <limits>
#include <vector>

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

bool Renderer::requiresAccumulationReset(
    const CameraState& current,
    const CameraState& previous) noexcept
{
  return current.position.x != previous.position.x ||
         current.position.y != previous.position.y ||
         current.position.z != previous.position.z ||
         current.yaw != previous.yaw ||
         current.pitch != previous.pitch ||
         current.fovY != previous.fovY ||
         current.aspect != previous.aspect;
}

void Renderer::mergeMetrics(
    RenderMetrics& dest,
    const RenderMetrics& src)
{
  dest.primaryRays += src.primaryRays;
  dest.secondaryRays += src.secondaryRays;
  dest.reflectionRays += src.reflectionRays;
  dest.shadowRays += src.shadowRays;
  dest.occludedShadowRays += src.occludedShadowRays;
  dest.shadowQueries += src.shadowQueries;
  dest.bvhNodeTests += src.bvhNodeTests;
  dest.bvhAabbTests += src.bvhAabbTests;
  dest.triangleTests += src.triangleTests;
  dest.bvhLeafTests += src.bvhLeafTests;
  dest.hits += src.hits;
  dest.misses += src.misses;
  dest.adaptiveCells += src.adaptiveCells;
  dest.maxSamplesUsed = std::max(dest.maxSamplesUsed, src.maxSamplesUsed);
  dest.maxBounceReached = std::max(dest.maxBounceReached, src.maxBounceReached);
  dest.bvhNodes = std::max(dest.bvhNodes, src.bvhNodes);
  dest.bvhLeaves = std::max(dest.bvhLeaves, src.bvhLeaves);
  dest.accumulatedFrames = std::max(dest.accumulatedFrames, src.accumulatedFrames);

  if (dest.shadowQueries > 0 && src.shadowQueries > 0) {
    dest.averageShadowSamples =
        (dest.averageShadowSamples * static_cast<float>(dest.shadowQueries - src.shadowQueries) +
         src.averageShadowSamples * static_cast<float>(src.shadowQueries)) /
        static_cast<float>(dest.shadowQueries);
  } else if (src.shadowQueries > 0) {
    dest.averageShadowSamples = src.averageShadowSamples;
  }
}

void Renderer::renderTile(
    AsciiFramebuffer& framebuffer,
    const Camera& camera,
    const Scene& scene,
    const RenderSettings& settings,
    int yStart,
    int yEnd,
    bool accumulationDirty,
    TileResult& result)
{
  const int targetWidth = settings.gridWidth;
  const bool temporalOn = settings.temporalAccumulation;
  AsciiMapper mapper;
  Sampler sampler;
  const int baseSamples = clampConfiguredSamples(settings.samplesPerCell);
  const int maxSamples = std::max(baseSamples, clampConfiguredSamples(settings.maxSamplesPerCell));

  result.metrics = RenderMetrics{};
  result.maxAccumulatedFrames = 0;
  result.totalSamplesUsed = 0;

  for (int y = yStart; y < yEnd; ++y) {
    for (int x = 0; x < targetWidth; ++x) {
      Vec3 radianceSum{};
      Vec3 displaySum{};
      RunningVariance luminanceVariance{};
      double depthSum = 0.0;
      int depthSamples = 0;
      int sampleCount = 0;

      const auto traceSample = [&](int sampleIndex) {
        const Sample sample = sampler.generateSubCellSample(x, y, sampleIndex, settings);
        const float u =
            (static_cast<float>(x) + sample.uv.x) / static_cast<float>(targetWidth);
        const float v =
            (static_cast<float>(y) + sample.uv.y) / static_cast<float>(settings.gridHeight);
        const Ray ray = camera.generateRay(u, v);
        HitInfo hit{};
        const Vec3 radiance = integrator_.traceRadiance(ray, scene, rayTracer_, settings, result.metrics, &hit);

        Vec3 display = mapper.applyExposureGamma(radiance, settings.exposure, settings.gamma);
        const float luminance = mapper.radianceToLuminance(display);
        if (!settings.colorOutput) {
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
      if (settings.adaptiveSampling && sampleCount < maxSamples &&
          luminanceVariance.variance() > settings.adaptiveVarianceThreshold) {
        usedAdaptiveSampling = true;
        for (int sampleIndex = sampleCount; sampleIndex < maxSamples; ++sampleIndex) {
          traceSample(sampleIndex);
        }
      }

      if (usedAdaptiveSampling) {
        ++result.metrics.adaptiveCells;
      }

      result.totalSamplesUsed += static_cast<std::uint64_t>(sampleCount);
      result.metrics.maxSamplesUsed = std::max(result.metrics.maxSamplesUsed, sampleCount);

      const float inverseSampleCount = 1.0F / static_cast<float>(sampleCount);
      const Vec3 averageRadiance = radianceSum * inverseSampleCount;
      const Vec3 averageDisplay = displaySum * inverseSampleCount;
      const float averageLuminance = static_cast<float>(luminanceVariance.mean);
      const float depth = depthSamples > 0
                              ? static_cast<float>(depthSum / static_cast<double>(depthSamples))
                              : std::numeric_limits<float>::infinity();

      AsciiCell& cell = framebuffer.at(static_cast<std::size_t>(x), static_cast<std::size_t>(y));

      if (temporalOn) {
        if (accumulationDirty) {
          cell.accumulatedRadiance = averageRadiance;
          cell.accumulatedFrames = 1;
        } else {
          cell.accumulatedRadiance += averageRadiance;
          ++cell.accumulatedFrames;
        }

        const float invFrames = 1.0F / static_cast<float>(cell.accumulatedFrames);
        const Vec3 outputRadiance = cell.accumulatedRadiance * invFrames;
        const Vec3 outputDisplay = mapper.applyExposureGamma(outputRadiance, settings.exposure, settings.gamma);
        const float outputLuminance = mapper.radianceToLuminance(outputDisplay);

        cell.radiance = outputRadiance;
        cell.luminance = outputLuminance;
        cell.glyph = mapper.mapLuminanceToGlyph(outputLuminance, settings.glyphRampMode);
        cell.fg = settings.colorOutput ? outputDisplay : Vec3{outputLuminance, outputLuminance, outputLuminance};
        cell.bg = Vec3{};
        cell.depth = depth;
        cell.sampleCount = sampleCount;
        cell.accumulatedLuminance = outputLuminance;
        result.maxAccumulatedFrames = std::max(result.maxAccumulatedFrames, cell.accumulatedFrames);
      } else {
        cell.radiance = averageRadiance;
        cell.luminance = averageLuminance;
        cell.glyph = mapper.mapLuminanceToGlyph(averageLuminance, settings.glyphRampMode);
        cell.fg = averageDisplay;
        cell.bg = Vec3{};
        cell.depth = depth;
        cell.sampleCount = sampleCount;
        cell.accumulatedRadiance = Vec3{};
        cell.accumulatedLuminance = 0.0F;
        cell.accumulatedFrames = 0;
      }
    }
  }
}

void Renderer::render(
    AsciiFramebuffer& framebuffer,
    const Camera& camera,
    const Scene& scene,
    const RenderSettings& settings)
{
  RenderSettings activeSettings = settings;
  activeSettings.validate();

  const CameraState cameraState = camera.state();
  accumulationDirty_ = !hasAppliedSettings_ ||
                       activeSettings.accumulationDirty ||
                       requiresAccumulationReset(activeSettings, appliedSettings_) ||
                       requiresAccumulationReset(cameraState, appliedCameraState_);
  appliedSettings_ = activeSettings;
  appliedCameraState_ = cameraState;
  hasAppliedSettings_ = true;

  const int targetWidth = std::max(activeSettings.gridWidth, 1);
  const int targetHeight = std::max(activeSettings.gridHeight, 1);
  if (framebuffer.width() != static_cast<std::size_t>(targetWidth) ||
      framebuffer.height() != static_cast<std::size_t>(targetHeight)) {
    framebuffer.resize(static_cast<std::size_t>(targetWidth), static_cast<std::size_t>(targetHeight));
    accumulationDirty_ = true;
  }

  const bool temporalOn = activeSettings.temporalAccumulation;
  const bool accumulationValid = temporalOn && !accumulationDirty_;
  if (!accumulationValid) {
    framebuffer.clear();
  }

  metrics_ = RenderMetrics{};
  metrics_.totalCells = static_cast<std::uint64_t>(targetWidth) * static_cast<std::uint64_t>(targetHeight);
  metrics_.temporalAccumulation = temporalOn;

  sceneTriangleCount_ = scene.triangles().size();
  if (activeSettings.enableBvh) {
    rayTracer_.buildAcceleration(scene.triangles(), activeSettings.bvhLeafSize);
  }

  int numThreads = activeSettings.enableMultithreading ? activeSettings.threadCount : 1;
  if (numThreads <= 0) {
    numThreads = static_cast<int>(std::thread::hardware_concurrency());
    if (numThreads <= 0) {
      numThreads = 1;
    }
  }
  numThreads = std::min(numThreads, targetHeight);

  std::vector<TileResult> results(static_cast<std::size_t>(numThreads));

  if (numThreads == 1) {
    renderTile(framebuffer, camera, scene, activeSettings, 0, targetHeight, accumulationDirty_, results[0]);
  } else {
    std::vector<std::future<void>> futures;
    futures.reserve(static_cast<std::size_t>(numThreads));
    for (int t = 0; t < numThreads; ++t) {
      const int yStart = (t * targetHeight) / numThreads;
      const int yEnd = ((t + 1) * targetHeight) / numThreads;
      futures.push_back(std::async(std::launch::async,
        [this, &framebuffer, &camera, &scene, &activeSettings, &results, t, yStart, yEnd]() {
          renderTile(framebuffer, camera, scene, activeSettings, yStart, yEnd, this->accumulationDirty_, results[static_cast<std::size_t>(t)]);
        }));
    }
    for (auto& f : futures) {
      f.get();
    }
  }

  std::uint64_t totalSamplesUsed = 0;
  int maxAccumulatedFrames = 0;

  for (const auto& result : results) {
    mergeMetrics(metrics_, result.metrics);
    totalSamplesUsed += result.totalSamplesUsed;
    maxAccumulatedFrames = std::max(maxAccumulatedFrames, result.maxAccumulatedFrames);
  }

  if (metrics_.totalCells > 0U) {
    metrics_.averageSamplesPerCell =
        static_cast<float>(static_cast<double>(totalSamplesUsed) / static_cast<double>(metrics_.totalCells));
  }
  metrics_.accumulatedFrames = maxAccumulatedFrames;
  metrics_.threadCountUsed = numThreads;
  metrics_.enableMultithreading = activeSettings.enableMultithreading;
}

} // namespace astraglyph
