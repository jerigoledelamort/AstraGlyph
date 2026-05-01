#include "render/RayTracer.hpp"

#include <algorithm>

namespace astraglyph {

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
  accelerationReady_ = true;

  bvh_.setLeafSize(accelerationLeafSize_);
  bvh_.build(triangles);
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

    const bool hit = bvh_.intersect(ray, closest, settings.backfaceCulling, metrics);
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
    return bvh_.intersectAny(ray, ray.tMax, settings.backfaceCulling, metrics);
  }

  for (const Triangle& triangle : triangles) {
    if (metrics != nullptr) {
      ++metrics->triangleTests;
    }
    if (triangle.intersectAny(ray, settings.backfaceCulling)) {
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
    if (!triangle.intersect(triangleRay, candidate, settings.backfaceCulling)) {
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
         accelerationLeafSize_ == std::max(1, settings.bvhLeafSize);
}

} // namespace astraglyph
