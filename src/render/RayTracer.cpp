#include "render/RayTracer.hpp"

#include "scene/Scene.hpp"

#include <algorithm>
#include <chrono>

#include "render/RayPacket.hpp"

#ifdef ASTRAGLYPH_ENABLE_SIMD
#include "render/SimdTriangle.hpp"
#endif

namespace astraglyph {
namespace {

[[nodiscard]] double elapsedMs(
    std::chrono::high_resolution_clock::time_point start,
    std::chrono::high_resolution_clock::time_point end) noexcept
{
  return std::chrono::duration<double, std::milli>(end - start).count();
}

} // namespace

void RayTracer::buildAcceleration(const std::vector<Triangle>& triangles, int leafSize)
{
  const int clampedLeafSize = std::max(1, leafSize);
  if (accelerationReady_ &&
      accelerationSource_ == triangles.data() &&
      accelerationTriangleCount_ == triangles.size() &&
      accelerationLeafSize_ == clampedLeafSize) {
    return;
  }

  accelerationLeafSize_ = clampedLeafSize;
  accelerationSource_ = triangles.data();
  accelerationTriangleCount_ = triangles.size();
  accelerationSceneVersion_ = 0; // Сброс версии для vector-потока
  accelerationReady_ = true;

  bvh_.setLeafSize(accelerationLeafSize_);
  bvh_.build(triangles);
}

void RayTracer::buildAcceleration(const Scene& scene, int leafSize)
{
  const int clampedLeafSize = std::max(1, leafSize);
  const std::size_t currentVersion = scene.version();
  
  if (accelerationReady_ &&
      accelerationSceneVersion_ == currentVersion &&
      accelerationLeafSize_ == clampedLeafSize) {
    return;
  }

  accelerationLeafSize_ = clampedLeafSize;
  accelerationSceneVersion_ = currentVersion;
  accelerationSource_ = scene.triangles().data();
  accelerationTriangleCount_ = scene.triangles().size();
  accelerationReady_ = true;

  bvh_.setLeafSize(accelerationLeafSize_);
  bvh_.build(scene.triangles());
}

HitInfo RayTracer::traceClosest(
    const Ray& ray,
    const std::vector<Triangle>& triangles,
    const RenderSettings& settings) const noexcept
{
  return traceClosest(ray, triangles, settings, nullptr);
}

HitInfo RayTracer::traceClosest(
    const Ray& ray,
    const std::vector<Triangle>& triangles,
    const RenderSettings& settings,
    RenderMetrics* metrics) const noexcept
{
  HitInfo closest{};
  if (settings.enableBvh && canUseAcceleration(triangles, settings)) {
    if (metrics != nullptr) {
      metrics->bvhNodes = static_cast<std::uint64_t>(bvh_.nodeCount());
      metrics->bvhLeaves = static_cast<std::uint64_t>(bvh_.leafCount());
    }

    bool hit = false;
    if (metrics != nullptr && metrics->renderProfilingEnabled) {
      const auto start = std::chrono::high_resolution_clock::now();
      hit = bvh_.intersect(ray, closest, settings.backfaceCulling, metrics);
      metrics->bvhTraversalMs += elapsedMs(start, std::chrono::high_resolution_clock::now());
    } else {
      hit = bvh_.intersect(ray, closest, settings.backfaceCulling, metrics);
    }
    if (!hit) {
      closest = HitInfo{};
    }
  } else {
    closest = traceClosestBruteForce(ray, triangles, settings, metrics);
  }

  if (metrics != nullptr) {
    if (closest.hit) {
      ++metrics->hits;
    } else {
      ++metrics->misses;
    }
  }

  return closest;
}

bool RayTracer::traceOcclusion(
    const Ray& ray,
    const std::vector<Triangle>& triangles,
    const RenderSettings& settings,
    RenderMetrics* metrics) const noexcept
{
  if (settings.enableBvh && canUseAcceleration(triangles, settings)) {
    if (metrics != nullptr && metrics->renderProfilingEnabled) {
      const auto start = std::chrono::high_resolution_clock::now();
      const bool hit = bvh_.intersectAny(ray, ray.tMax, settings.backfaceCulling, metrics);
      metrics->bvhTraversalMs += elapsedMs(start, std::chrono::high_resolution_clock::now());
      return hit;
    }
    return bvh_.intersectAny(ray, ray.tMax, settings.backfaceCulling, metrics);
  }

  for (const Triangle& triangle : triangles) {
    if (metrics != nullptr) {
      ++metrics->triangleTests;
    }
    bool hit = false;
    if (metrics != nullptr && metrics->renderProfilingEnabled) {
      const auto start = std::chrono::high_resolution_clock::now();
      hit = triangle.intersectAny(ray, settings.backfaceCulling);
      metrics->triangleIntersectionMs += elapsedMs(start, std::chrono::high_resolution_clock::now());
    } else {
      hit = triangle.intersectAny(ray, settings.backfaceCulling);
    }
    if (hit) {
      return true;
    }
  }

  return false;
}

HitInfo RayTracer::traceClosestBruteForce(
    const Ray& ray,
    const std::vector<Triangle>& triangles,
    const RenderSettings& settings,
    RenderMetrics* metrics) const noexcept
{
  HitInfo closest{};
  for (const Triangle& triangle : triangles) {
    if (metrics != nullptr) {
      ++metrics->triangleTests;
    }

    HitInfo candidate{};
    Ray triangleRay = ray;
    triangleRay.tMax = closest.hit ? std::min(ray.tMax, closest.t) : ray.tMax;
    bool hit = false;
    if (metrics != nullptr && metrics->renderProfilingEnabled) {
      const auto start = std::chrono::high_resolution_clock::now();
      hit = triangle.intersect(triangleRay, candidate, settings.backfaceCulling);
      metrics->triangleIntersectionMs += elapsedMs(start, std::chrono::high_resolution_clock::now());
    } else {
      hit = triangle.intersect(triangleRay, candidate, settings.backfaceCulling);
    }
    if (!hit) {
      continue;
    }

    if (!closest.hit || candidate.t < closest.t) {
      closest = candidate;
    }
  }

  return closest;
}

bool RayTracer::canUseAcceleration(
    const std::vector<Triangle>& triangles,
    const RenderSettings& settings) const noexcept
{
  return accelerationReady_ &&
         accelerationSource_ == triangles.data() &&
         accelerationTriangleCount_ == triangles.size() &&
         accelerationSceneVersion_ == 0 && // Только для vector-потока
         accelerationLeafSize_ == std::max(1, settings.bvhLeafSize);
}

bool RayTracer::canUseAcceleration(
    const Scene& scene,
    const RenderSettings& settings) const noexcept
{
  return accelerationReady_ &&
         accelerationSceneVersion_ == scene.version() &&
         accelerationLeafSize_ == std::max(1, settings.bvhLeafSize);
}

} // namespace astraglyph
