#include "render/Renderer.hpp"

#include "render/AsciiFramebuffer.hpp"
#include "render/AsciiMapper.hpp"
#include "render/Sampler.hpp"
#include "scene/Scene.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

namespace astraglyph {
namespace {

constexpr int kMinTilesPerThread = 8;  // Минимальное количество тайлов на поток

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

[[nodiscard]] double elapsedMs(
    std::chrono::high_resolution_clock::time_point start,
    std::chrono::high_resolution_clock::time_point end)
{
  return std::chrono::duration<double, std::milli>(end - start).count();
}

} // namespace

const RenderMetrics& Renderer::metrics() const noexcept
{
  return metrics_;
}

void Renderer::setPresentationProfiling(double finalOutputMs, double totalRenderMs) noexcept
{
  metrics_.finalOutputMs = finalOutputMs;
  metrics_.totalRenderMs = totalRenderMs;
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
         current.enableRimLight != previous.enableRimLight ||
         current.specularBoost != previous.specularBoost ||
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
  dest.bvhTraversalMs += src.bvhTraversalMs;
  dest.triangleIntersectionMs += src.triangleIntersectionMs;
  dest.shadingMs += src.shadingMs;

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
    bool accumulationValid,
    TileResult& result)
{
  const int targetWidth = settings.gridWidth;
  const bool temporalOn = settings.temporalAccumulation;
  const bool checkerboard = settings.checkerboardSampling && temporalOn && accumulationValid;
  const int frameParity = static_cast<int>(settings.frameIndex) & 1;
  AsciiMapper mapper;
  Sampler sampler;
  const int baseSamples = clampConfiguredSamples(settings.samplesPerCell);
  const int maxSamples = std::max(baseSamples, clampConfiguredSamples(settings.maxSamplesPerCell));

  result.metrics = RenderMetrics{};
  result.metrics.renderProfilingEnabled = settings.enableRenderProfiling;
  result.maxAccumulatedFrames = 0;
  result.totalSamplesUsed = 0;

  // Локальные накопители для минимизации обращений к памяти
  for (int y = yStart; y < yEnd; ++y) {
    for (int x = 0; x < targetWidth; ++x) {
      // Checkerboard: skip half the cells on even/odd frames
      if (checkerboard && ((x + y + frameParity) & 1) != 0) {
        // Keep old cell values; update frame count metric only
        if (temporalOn) {
          result.maxAccumulatedFrames = std::max(
              result.maxAccumulatedFrames,
              temporalAccumulator_.getAccumulatedFrames(x, y));
        }
        continue;
      }
      
      // Локальные переменные для лучшего использования кэша
      Vec3 directSum{};
      Vec3 indirectSum{};
      Vec3 specularSum{};
      Vec3 displaySum{};
      Vec3 normalSum{};
      RunningVariance luminanceVariance{};
      double depthSum = 0.0;
      int depthSamples = 0;
      int normalSamples = 0;
      int sampleCount = 0;

      // Pre-compute ray parameters для этого cell
      const float uBase = static_cast<float>(x) / static_cast<float>(targetWidth);
      const float vBase = static_cast<float>(y) / static_cast<float>(settings.gridHeight);

      const auto traceSample = [&](int sampleIndex) {
        const Sample sample = sampler.generateSubCellSample(x, y, sampleIndex, settings);
        const float u = uBase + sample.uv.x / static_cast<float>(targetWidth);
        const float v = vBase + sample.uv.y / static_cast<float>(settings.gridHeight);
        const Ray ray = camera.generateRay(u, v);
        HitInfo hit{};
        const Vec3 radiance = integrator_.traceRadiance(ray, scene, rayTracer_, settings, result.metrics, &hit);

        // Debug views are already display-space values.
        const Vec3 display = settings.isDebugViewEnabled()
            ? radiance 
            : mapper.applyExposureGamma(radiance, settings.exposure, settings.gamma);
        const float luminance = mapper.radianceToLuminance(display);

        directSum += hit.direct;
        indirectSum += hit.indirect;
        specularSum += hit.specular;
        displaySum += display;
        luminanceVariance.add(luminance);
        if (hit.hit) {
          depthSum += hit.t;
          ++depthSamples;
          normalSum += hit.normal;
          ++normalSamples;
        }
        ++sampleCount;
      };

      // Базовые сэмплы
      for (int sampleIndex = 0; sampleIndex < baseSamples; ++sampleIndex) {
        traceSample(sampleIndex);
      }

      // Адаптивное сэмплирование
      if (settings.adaptiveSampling && sampleCount < maxSamples &&
          luminanceVariance.variance() > settings.adaptiveVarianceThreshold) {
        for (int sampleIndex = sampleCount; sampleIndex < maxSamples; ++sampleIndex) {
          traceSample(sampleIndex);
        }
      }

      if (sampleCount > baseSamples) {
        ++result.metrics.adaptiveCells;
      }

      result.totalSamplesUsed += static_cast<std::uint64_t>(sampleCount);
      result.metrics.maxSamplesUsed = std::max(result.metrics.maxSamplesUsed, sampleCount);

      const float inverseSampleCount = 1.0F / static_cast<float>(sampleCount);
      const Vec3 averageDirect = directSum * inverseSampleCount;
      const Vec3 averageIndirect = indirectSum * inverseSampleCount;
      const Vec3 averageSpecular = specularSum * inverseSampleCount;
      const Vec3 averageDisplay = displaySum * inverseSampleCount;
      const float depth = depthSamples > 0
                              ? static_cast<float>(depthSum / static_cast<double>(depthSamples))
                              : std::numeric_limits<float>::infinity();

      AsciiCell& cell = framebuffer.at(static_cast<std::size_t>(x), static_cast<std::size_t>(y));

      if (normalSamples > 0) {
        cell.normal = normalSum * (1.0F / static_cast<float>(normalSamples));
      } else {
        cell.normal = Vec3{0.0F, 0.0F, 0.0F};
      }

      if (temporalOn) {
        Vec3 outputRadiance = temporalAccumulator_.accumulate(
            x, y, averageDirect, averageIndirect, averageSpecular);
        const int frameCount = temporalAccumulator_.getAccumulatedFrames(x, y);

        // Specular boost after temporal accumulation
        if (settings.specularBoost > 1.0f) {
          const Vec3 specularAccum = temporalAccumulator_.getSpecularRadiance(x, y);
          outputRadiance += specularAccum * (settings.specularBoost - 1.0f);
        }

        const Vec3 outputDisplay = settings.isDebugViewEnabled()
            ? outputRadiance
            : mapper.applyExposureGamma(outputRadiance, settings.exposure, settings.gamma);
        const float outputLuminance = mapper.radianceToLuminance(outputDisplay);

        cell.radiance = outputRadiance;
        cell.luminance = outputLuminance;
        cell.glyph = mapper.mapLuminanceToGlyph(outputLuminance, settings.glyphRampMode);
        cell.fg = outputDisplay;
        cell.bg = Vec3{};
        cell.depth = depth;
        cell.sampleCount = sampleCount;
        cell.accumulatedRadiance = temporalAccumulator_.getAccumulatedRadiance(x, y);
        cell.accumulatedFrames = frameCount;
        cell.accumulatedLuminance = outputLuminance;

        result.maxAccumulatedFrames = std::max(result.maxAccumulatedFrames, frameCount);
      } else {
        Vec3 averageRadiance = averageDirect + averageIndirect + averageSpecular;
        if (settings.specularBoost > 1.0f) {
          averageRadiance += averageSpecular * (settings.specularBoost - 1.0f);
        }
        const Vec3 displayForLuminance = settings.isDebugViewEnabled()
            ? averageRadiance
            : mapper.applyExposureGamma(averageRadiance, settings.exposure, settings.gamma);
        const float luminanceFromRadiance = mapper.radianceToLuminance(displayForLuminance);

        cell.radiance = averageRadiance;
        cell.luminance = luminanceFromRadiance;
        cell.glyph = mapper.mapLuminanceToGlyph(luminanceFromRadiance, settings.glyphRampMode);
        cell.fg = averageDisplay;
        cell.bg = Vec3{};
        cell.depth = depth;
        cell.sampleCount = sampleCount;
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
  const bool profilingEnabled = settings.enableRenderProfiling;
  std::chrono::high_resolution_clock::time_point renderStart{};
  if (profilingEnabled) {
    renderStart = std::chrono::high_resolution_clock::now();
  }

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

  temporalAccumulator_.resize(targetWidth, targetHeight);

  const bool temporalOn = activeSettings.temporalAccumulation;
  const bool accumulationValid = temporalOn && !accumulationDirty_;
  if (!accumulationValid) {
    framebuffer.clear();
  }

  if (accumulationDirty_) {
    temporalAccumulator_.reset();
  }

  metrics_ = RenderMetrics{};
  metrics_.renderProfilingEnabled = activeSettings.enableRenderProfiling;
  metrics_.totalCells = static_cast<std::uint64_t>(targetWidth) * static_cast<std::uint64_t>(targetHeight);
  metrics_.temporalAccumulation = temporalOn;

  sceneTriangleCount_ = scene.triangles().size();
  if (activeSettings.enableBvh) {
    rayTracer_.buildAcceleration(scene.triangles(), activeSettings.bvhLeafSize);
  }

  // Определяем оптимальное количество потоков
  int maxThreads = static_cast<int>(std::thread::hardware_concurrency());
  if (maxThreads <= 0) {
    maxThreads = 1;
  }
  
  int numThreads = activeSettings.enableMultithreading ? activeSettings.threadCount : 1;
  if (numThreads <= 0) {
    numThreads = maxThreads;
  }
  // Ограничиваем количеством тайлов и доступными потоками
  const int totalTiles = targetHeight;
  numThreads = std::min(numThreads, std::max(1, totalTiles / kMinTilesPerThread));
  numThreads = std::min(numThreads, maxThreads);
  numThreads = std::max(numThreads, 1);

  // Инициализация thread pool (однократно)
  if (!threadPoolInitialized_.load()) {
    threadPool_.start(static_cast<std::size_t>(maxThreads));
    threadPoolInitialized_.store(true);
  }
  
  // Убедимся, что thread pool имеет правильное количество потоков
  if (threadPool_.threadCount() != static_cast<std::size_t>(maxThreads)) {
    threadPool_.stop();
    threadPool_.start(static_cast<std::size_t>(maxThreads));
  }

  // Результаты для каждого тайла (локальные, без mutex)
  std::vector<TileResult> results(static_cast<std::size_t>(targetHeight));

  if (numThreads == 1 || targetHeight <= 1) {
    // Single-threaded fallback
    renderTile(framebuffer, camera, scene, activeSettings, 0, targetHeight, accumulationValid, results[0]);
  } else {
    // Вычисляем размер тайла для равномерного распределения нагрузки
    const int rowsPerTask = std::max((targetHeight + numThreads - 1) / numThreads, 1);
    const int numTasks = (targetHeight + rowsPerTask - 1) / rowsPerTask;
    
    for (int task = 0; task < numTasks; ++task) {
      const int yStart = task * rowsPerTask;
      const int yEnd = std::min((task + 1) * rowsPerTask, targetHeight);
      
      if (yStart >= yEnd) {
        continue;
      }
      
      const bool localAccumulationValid = accumulationValid;
      threadPool_.enqueue([this, &framebuffer, &camera, &scene, &activeSettings,
                          &results, yStart, yEnd, localAccumulationValid]() {
        for (int y = yStart; y < yEnd; ++y) {
          TileResult& result = results[static_cast<std::size_t>(y)];
          renderTile(framebuffer, camera, scene, activeSettings, y, y + 1, 
                     localAccumulationValid, result);
        }
      });
    }
    
    // Ждем завершения всех задач
    threadPool_.waitAll();
  }

  // Агрегация результатов (быстрая, без блокировок в hot path)
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

  // Phase 3: contrast-aware glyph mapping + edge enhancement
  if (activeSettings.shapeAwareGlyphs) {
    AsciiMapper mapper;
    for (int y = 0; y < targetHeight; ++y) {
      for (int x = 0; x < targetWidth; ++x) {
        AsciiCell& cell = framebuffer.at(static_cast<std::size_t>(x), static_cast<std::size_t>(y));
        float strength = 0.0F;
        float direction = 0.0F;
        AsciiMapper::computeGradient(framebuffer, x, y, strength, direction);
        cell.glyph = mapper.mapShapeAwareGlyph(
            cell.luminance, strength, direction, activeSettings.glyphRampMode);
      }
    }
    AsciiMapper::enhanceEdges(framebuffer);
  }

  if (activeSettings.enableRenderProfiling) {
    metrics_.totalRenderMs = elapsedMs(renderStart, std::chrono::high_resolution_clock::now());
  }
}

} // namespace astraglyph
