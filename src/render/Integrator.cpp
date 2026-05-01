#include "render/Integrator.hpp"

#include "render/Sampler.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <vector>

namespace astraglyph {
namespace {

constexpr float kLightEpsilon = 1.0e-5F;

Vec3 safeNormal(Vec3 normal) noexcept
{
  const float normalLength = length(normal);
  return normalLength > kLightEpsilon ? normal / normalLength : Vec3{0.0F, 1.0F, 0.0F};
}

Vec3 faceForwardNormal(Vec3 normal, Vec3 incidentDirection) noexcept
{
  const Vec3 normalized = safeNormal(normal);
  return dot(normalized, incidentDirection) > 0.0F ? -normalized : normalized;
}

float safeShadowBias(const RenderSettings& settings) noexcept
{
  return std::max(settings.shadowBias, kLightEpsilon);
}

float safeReflectionBias(const RenderSettings& settings) noexcept
{
  return std::max(settings.reflectionBias, kLightEpsilon);
}

std::uint32_t hashCombine(std::uint32_t seed, std::uint32_t value) noexcept
{
  return seed ^ (value + 0x9E3779B9U + (seed << 6U) + (seed >> 2U));
}

std::uint32_t floatBits(float value) noexcept
{
  return std::bit_cast<std::uint32_t>(value);
}

void recordShadowQuery(RenderMetrics& metrics, int sampleCount) noexcept
{
  ++metrics.shadowQueries;
  const float queryCount = static_cast<float>(metrics.shadowQueries);
  metrics.averageShadowSamples +=
      (static_cast<float>(sampleCount) - metrics.averageShadowSamples) / queryCount;
}

Vec3 sampleAreaLightPoint(const Light& light, Vec2 uv) noexcept
{
  const Vec3 axisRight = safeNormal(light.right);
  const Vec3 axisUp = safeNormal(light.up);
  const float halfWidth = std::max(light.width, 0.0F) * 0.5F;
  const float halfHeight = std::max(light.height, 0.0F) * 0.5F;

  return light.position +
         axisRight * ((uv.x - 0.5F) * (2.0F * halfWidth)) +
         axisUp * ((uv.y - 0.5F) * (2.0F * halfHeight));
}

bool traceShadowVisibility(
    const RayTracer& rayTracer,
    const std::vector<Triangle>& triangles,
    const RenderSettings& settings,
    RenderMetrics& metrics,
    const Vec3& origin,
    const Vec3& direction,
    float tMax) noexcept
{
  const float bias = safeShadowBias(settings);
  ++metrics.shadowRays;
  const Ray shadowRay{
      origin,
      direction,
      bias,
      tMax,
  };
  const bool occluded = rayTracer.traceOcclusion(shadowRay, triangles, settings, &metrics);
  if (occluded) {
    ++metrics.occludedShadowRays;
  }
  return !occluded;
}

Vec3 accumulateAreaLight(
    const Light& light,
    const HitInfo& hit,
    const Vec3& normal,
    const Ray& ray,
    const std::vector<Triangle>& triangles,
    const RayTracer& rayTracer,
    const RenderSettings& settings,
    RenderMetrics& metrics,
    int depth) noexcept
{
  const int sampleCount =
      settings.enableSoftShadows ? std::max(settings.shadowSamples, 1) : 1;
  recordShadowQuery(metrics, sampleCount);

  const Sampler sampler;
  std::uint32_t seedA = hashCombine(floatBits(hit.position.x), floatBits(hit.position.z));
  seedA = hashCombine(seedA, static_cast<std::uint32_t>(hit.materialId));
  std::uint32_t seedB = hashCombine(floatBits(hit.position.y), floatBits(ray.direction.x));
  seedB = hashCombine(seedB, floatBits(ray.direction.y));
  std::uint32_t seedC = hashCombine(floatBits(ray.direction.z), static_cast<std::uint32_t>(depth));
  const std::uint32_t lightSeed = sampler.makeSeed(
      settings.frameIndex,
      seedA,
      seedB,
      seedC ^ static_cast<std::uint32_t>(std::max(static_cast<int>(light.type), 0)));

  Vec3 contribution{};
  const float shadowBias = safeShadowBias(settings);
  const Vec3 shadowOrigin = hit.position + normal * shadowBias;
  for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
    const Vec2 uv = settings.enableSoftShadows
                        ? sampler.sample2D(lightSeed, sampleIndex, sampleCount, settings.jitteredSampling)
                        : Vec2{0.5F, 0.5F};
    const Vec3 lightPoint = sampleAreaLightPoint(light, uv);
    const Vec3 toLight = lightPoint - hit.position;
    const float distanceToLight = length(toLight);
    if (distanceToLight <= kLightEpsilon) {
      continue;
    }

    const Vec3 lightDirection = toLight / distanceToLight;
    const float ndotl = std::max(0.0F, dot(normal, lightDirection));
    if (ndotl <= 0.0F) {
      continue;
    }

    const float lightTMax = std::max(shadowBias, distanceToLight - shadowBias);
    bool visible = true;
    if (settings.enableShadows) {
      visible = traceShadowVisibility(
          rayTracer,
          triangles,
          settings,
          metrics,
          shadowOrigin,
          lightDirection,
          lightTMax);
    }

    if (visible) {
      contribution += light.color * (ndotl * light.intensity);
    }
  }

  return contribution / static_cast<float>(sampleCount);
}

Vec3 accumulateLocalLighting(
    const Ray& ray,
    const HitInfo& hit,
    const Scene& scene,
    const RayTracer& rayTracer,
    const RenderSettings& settings,
    RenderMetrics& metrics,
    int depth) noexcept
{
  const Material& material = scene.materialFor(hit.materialId);
  const Vec3 normal = safeNormal(hit.normal);
  Vec3 result = material.emissionColor * material.emissionStrength;
  result += material.albedo * settings.ambientStrength;

  const std::vector<Triangle>& triangles = scene.triangles();
  for (const Light& light : scene.lights()) {
    if (light.intensity <= 0.0F) {
      continue;
    }

    if (light.type == LightType::Area) {
      result += material.albedo * accumulateAreaLight(
          light,
          hit,
          normal,
          ray,
          triangles,
          rayTracer,
          settings,
          metrics,
          depth);
      continue;
    }

    Vec3 lightDirection{};
    float lightTMax = std::numeric_limits<float>::infinity();

    if (light.type == LightType::Directional) {
      lightDirection = safeNormal(-light.direction);
    } else {
      const Vec3 toLight = light.position - hit.position;
      const float distanceToLight = length(toLight);
      if (distanceToLight <= kLightEpsilon) {
        continue;
      }

      lightDirection = toLight / distanceToLight;
      lightTMax = std::max(safeShadowBias(settings), distanceToLight - safeShadowBias(settings));
    }

    const float ndotl = std::max(0.0F, dot(normal, lightDirection));
    if (ndotl <= 0.0F) {
      continue;
    }

    bool visible = true;
    if (settings.enableShadows) {
      recordShadowQuery(metrics, 1);
      visible = traceShadowVisibility(
          rayTracer,
          triangles,
          settings,
          metrics,
          hit.position + normal * safeShadowBias(settings),
          lightDirection,
          lightTMax);
    }

    if (visible) {
      result += material.albedo * light.color * (ndotl * light.intensity);
    }
  }

  return result;
}

} // namespace

Vec3 Integrator::traceRadiance(
    const Ray& ray,
    const Scene& scene,
    const RayTracer& rayTracer,
    const RenderSettings& settings,
    RenderMetrics& metrics,
    HitInfo* outHit,
    int depth) const
{
  metrics.maxBounceReached = std::max(metrics.maxBounceReached, depth);
  if (depth == 0) {
    ++metrics.primaryRays;
  } else {
    ++metrics.secondaryRays;
  }

  const std::vector<Triangle>& triangles = scene.triangles();
  const HitInfo hit = rayTracer.traceClosest(ray, triangles, settings, &metrics);
  if (outHit != nullptr) {
    *outHit = hit;
  }

  if (!hit.hit) {
    return settings.backgroundColor;
  }

  const Material& material = scene.materialFor(hit.materialId);
  const Vec3 reflectionNormal = faceForwardNormal(hit.normal, ray.direction);
  const Vec3 localRadiance = accumulateLocalLighting(
      ray,
      hit,
      scene,
      rayTracer,
      settings,
      metrics,
      depth);

  const float reflectivity = std::clamp(material.reflectivity, 0.0F, 1.0F);
  const int maxBounces = std::max(settings.maxBounces, 0);
  if (!settings.enableReflections || reflectivity <= 0.0F || depth >= maxBounces) {
    return localRadiance;
  }

  const float reflectionBias = safeReflectionBias(settings);
  const Vec3 reflectedDirection = normalize(reflect(ray.direction, reflectionNormal));
  ++metrics.reflectionRays;
  const Ray reflectedRay{
      hit.position + reflectionNormal * reflectionBias,
      reflectedDirection,
      reflectionBias,
      std::numeric_limits<float>::infinity(),
  };
  const Vec3 reflectedRadiance =
      traceRadiance(reflectedRay, scene, rayTracer, settings, metrics, nullptr, depth + 1);
  return lerp(localRadiance, reflectedRadiance, reflectivity);
}

Vec3 Integrator::radiance(const Scene& scene, const Ray& ray) const
{
  RayTracer rayTracer;
  RenderSettings settings;
  if (settings.enableBvh) {
    rayTracer.buildAcceleration(scene.triangles(), settings.bvhLeafSize);
  }
  RenderMetrics metrics;
  return traceRadiance(ray, scene, rayTracer, settings, metrics);
}

} // namespace astraglyph
