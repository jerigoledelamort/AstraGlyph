#include "render/Integrator.hpp"

#include "render/Sampler.hpp"
#include "scene/Material.hpp"

#include <algorithm>
#include <bit>
#include <chrono>
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
    // 🥇 4. РАННИЙ ВЫХОД: если surface не освещён → не считаем тень
    if (ndotl <= 0.0F) {
      continue;
    }

    const float lightTMax = std::max(shadowBias, distanceToLight - shadowBias);
    bool visible = true;
    if (settings.enableShadows) {
      // 🥇 1. EARLY EXIT ДЛЯ ТЕНЕЙ: используем быстрый intersectAny
      // Вместо полного traceShadowVisibility используем только occlusion check
      const Ray shadowRay{
          shadowOrigin,
          lightDirection,
          shadowBias,
          lightTMax,
      };
      visible = !rayTracer.traceOcclusion(shadowRay, triangles, settings, &metrics);
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
    Vec3& outSpecular) noexcept
{
  const Material& material = scene.materialFor(hit.materialId);
  const Vec3 normal = safeNormal(hit.normal);
  const Vec3 viewDirection = safeNormal(ray.origin - hit.position);

  Vec3 diffuseResult = Vec3{};
  Vec3 specularResult = Vec3{};
  const std::vector<Triangle>& triangles = scene.triangles();

  for (const Light& light : scene.lights()) {
    const float intensity = std::clamp(light.intensity, 0.0F, 10.0F);
    if (intensity <= 0.0F) {
      continue;
    }

    Vec3 lightDirection{};
    float lightTMax = std::numeric_limits<float>::infinity();

    if (light.type == LightType::Directional) {
      lightDirection = -light.direction;
      const float dirLen = length(lightDirection);
      if (dirLen > 0.001F && dirLen < 0.99F) {
        lightDirection = lightDirection / dirLen;
      }
    } else {
      const Vec3 toLight = light.position - hit.position;
      const float distanceToLight = length(toLight);
      if (distanceToLight <= kLightEpsilon) {
        continue;
      }
      lightDirection = toLight / distanceToLight;
      lightTMax = std::max(safeShadowBias(settings), distanceToLight - safeShadowBias(settings));
    }

    const float NdotL = std::max(0.0F, dot(normal, lightDirection));
    if (NdotL <= 0.0F) {
      continue;
    }

    bool visible = true;
    if (settings.enableShadows) {
      const Ray shadowRay{
          hit.position + normal * safeShadowBias(settings),
          lightDirection,
          safeShadowBias(settings),
          lightTMax,
      };
      visible = !rayTracer.traceOcclusion(shadowRay, triangles, settings, &metrics);
    }

    if (visible) {
      const Vec3 albedo = sampleAlbedoTextureLinear(material, hit.uv);
      const float metallic = sampleMetallicTexture(material, hit.uv);

      diffuseResult += albedo * 0.2F * (1.0F - metallic); // Ambient

      const Vec3 lighting = light.color * intensity * NdotL;
      diffuseResult += albedo * (1.0F - metallic) * lighting;

      const Vec3 halfDirection = safeNormal(lightDirection + viewDirection);
      const float NdotH = std::max(0.0F, dot(normal, halfDirection));
      float specFactor = NdotH * NdotH;
      specFactor *= specFactor;
      specFactor *= specFactor;
      specFactor *= specFactor;
      specFactor *= specFactor;

      specularResult += light.color * specFactor * metallic;
    }
  }

  // Rim light: Fresnel-like edge glow
  if (settings.enableRimLight) {
    const float rim = 1.0F - std::abs(dot(normal, viewDirection));
    const float rimStrength = rim * rim * rim * 0.5F;
    const float metallic = sampleMetallicTexture(material, hit.uv);
    for (const Light& light : scene.lights()) {
      specularResult += light.color * rimStrength * metallic * light.intensity * 0.3F;
    }
  }

  outSpecular = specularResult;
  return diffuseResult;
}

[[nodiscard]] double elapsedMs(
    std::chrono::high_resolution_clock::time_point start,
    std::chrono::high_resolution_clock::time_point end) noexcept
{
  return std::chrono::duration<double, std::milli>(end - start).count();
}

float debugLightNdotL(const HitInfo& hit, const Scene& scene) noexcept
{
  // 🥇 5. СЭКОНОМЛЕНА NORMALIZATION: нормаль уже нормализована в hit.normal
  const Vec3 normal = hit.normal;
  float result = 0.0F;
  for (const Light& light : scene.lights()) {
    const float intensity = std::clamp(light.intensity, 0.0F, 10.0F);
    if (intensity <= 0.0F) {
      continue;
    }

    Vec3 lightDirection{};
    if (light.type == LightType::Directional) {
      lightDirection = -light.direction;
      // Быстрая проверка нормализованности
      const float dirLen = length(lightDirection);
      if (dirLen > 0.001F && dirLen < 0.99F) {
        lightDirection = lightDirection / dirLen;
      }
    } else {
      const Vec3 toLight = light.position - hit.position;
      const float distLen = length(toLight);
      if (distLen <= kLightEpsilon) {
        continue;
      }
      lightDirection = toLight / distLen;
    }

    result = std::max(result, std::max(0.0F, dot(normal, lightDirection)));
  }
  return result;
}

float debugDepthMaxDistance(const Ray& ray, const Scene& scene, const HitInfo& hit) noexcept
{
  if (std::isfinite(ray.tMax)) {
    return std::max(ray.tMax, kLightEpsilon);
  }

  float maxDistance = hit.t;
  for (const Triangle& triangle : scene.triangles()) {
    maxDistance = std::max(maxDistance, dot(triangle.v0 - ray.origin, ray.direction));
    maxDistance = std::max(maxDistance, dot(triangle.v1 - ray.origin, ray.direction));
    maxDistance = std::max(maxDistance, dot(triangle.v2 - ray.origin, ray.direction));
  }

  return std::max(maxDistance, kLightEpsilon);
}

Vec3 debugColor(
    const Ray& ray,
    const HitInfo& hit,
    const Material& material,
    const Scene& scene,
    DebugViewMode mode) noexcept
{
  switch (mode) {
    case DebugViewMode::Albedo:
      return sampleAlbedoTexture(material, hit.uv);
    case DebugViewMode::Normals: {
      const Vec3 normal = safeNormal(hit.normal);
      return normal * 0.5F + Vec3{0.5F, 0.5F, 0.5F};
    }
    case DebugViewMode::Depth: {
      const float maxDistance = debugDepthMaxDistance(ray, scene, hit);
      const float depth = std::clamp(hit.t / maxDistance, 0.0F, 1.0F);
      return Vec3{depth, depth, depth};
    }
    case DebugViewMode::Lighting: {
      const float NdotL = debugLightNdotL(hit, scene);
      return Vec3{NdotL, NdotL, NdotL};
    }
    default:
      return Vec3{};
  }
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

  const bool profilingEnabled = metrics.renderProfilingEnabled;
  std::chrono::high_resolution_clock::time_point shadingStart{};
  if (profilingEnabled) {
    shadingStart = std::chrono::high_resolution_clock::now();
  }

  Vec3 direct{};
  Vec3 indirect{};
  Vec3 specularLocal{};

  if (!hit.hit) {
    direct = settings.backgroundColor;
  } else {
    // 🥇 5. СЭКОНОМЛЕНА TEXTURE SAMPLE: не загружаем материал для debug mode пока
    const DebugViewMode debugMode = settings.activeDebugViewMode();
    if (debugMode != DebugViewMode::Off) {
      const Material& material = scene.materialFor(hit.materialId);
      direct = debugColor(ray, hit, material, scene, debugMode);
    } else {
      // 🥇 4. ТЕКСТУРЫ СЧИТАЕМ ТОЛЬКО ЗДЕСЬ: внутри accumulateLocalLighting
      direct = accumulateLocalLighting(
          ray,
          hit,
          scene,
          rayTracer,
          settings,
          metrics,
          specularLocal);

      // Emission is direct light
      const Material& material = scene.materialFor(hit.materialId);
      direct += material.emissionColor * material.emissionStrength;

      // Indirect: reflections
      const Vec3 reflectionNormal = faceForwardNormal(hit.normal, ray.direction);
      const float metallic = sampleMetallicTexture(material, hit.uv);
      const float roughness = sampleRoughnessTexture(material, hit.uv);
      const float reflectivity = std::clamp(metallic * (1.0F - roughness * 0.5F), 0.0F, 1.0F);
      const int maxBounces = std::max(settings.maxBounces, 0);
      if (settings.enableReflections && reflectivity > 0.01F && depth < maxBounces) {
        const float reflectionBias = safeReflectionBias(settings);
        const Vec3 reflectedDirection = normalize(reflect(ray.direction, reflectionNormal));
        ++metrics.reflectionRays;
        const Ray reflectedRay{
            hit.position + reflectionNormal * reflectionBias,
            reflectedDirection,
            reflectionBias,
            std::numeric_limits<float>::infinity(),
        };
        HitInfo reflectedHit{};
        const Vec3 reflectedRadiance = traceRadiance(
            reflectedRay, scene, rayTracer, settings, metrics, &reflectedHit, depth + 1);
        const Vec3 reflectionColor = lerp(
            Vec3{1.0F, 1.0F, 1.0F},
            sampleAlbedoTextureLinear(material, hit.uv),
            1.0F - metallic);
        indirect = reflectedRadiance * reflectionColor * reflectivity;
      }
    }
  }

  if (profilingEnabled) {
    metrics.shadingMs += elapsedMs(shadingStart, std::chrono::high_resolution_clock::now());
  }

  if (outHit != nullptr) {
    *outHit = hit;
    outHit->direct = direct;
    outHit->indirect = indirect;
    outHit->specular = specularLocal;
  }

  return direct + indirect + specularLocal;
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
