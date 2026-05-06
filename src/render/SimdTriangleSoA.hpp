#pragma once

#include "geometry/TriangleSoA.hpp"
#include "math/Ray.hpp"

namespace astraglyph {

// SIMD-оптимизированное пересечение луча с batch треугольников (SoA версия)
// Возвращает bitmask где каждый бит указывает на попадание
#if defined(ASTRAGLYPH_SIMD_AVX2)
[[nodiscard]] int intersectBatchSimd(
    const TriangleBatchSoA& batch,
    const Ray& ray,
    bool backfaceCulling,
    float* outT = nullptr,
    float* outU = nullptr,
    float* outV = nullptr) noexcept;
#elif defined(ASTRAGLYPH_SIMD_SSE3)
[[nodiscard]] int intersectBatchSimd(
    const TriangleBatchSoA& batch,
    const Ray& ray,
    bool backfaceCulling,
    float* outT = nullptr,
    float* outU = nullptr,
    float* outV = nullptr) noexcept;
#else
// Fallback на scalar версию
[[nodiscard]] inline int intersectBatchSimd(
    const TriangleBatchSoA& batch,
    const Ray& ray,
    bool backfaceCulling,
    float* outT,
    float* outU,
    float* outV) noexcept
{
  int hitMask = 0;
  constexpr float epsilon = 1.0e-6F;
  
  for (int i = 0; i < batch.triangleCount; ++i) {
    const float v0x = batch.v0x[i];
    const float v0y = batch.v0y[i];
    const float v0z = batch.v0z[i];
    const float edge1x = batch.edge1x[i];
    const float edge1y = batch.edge1y[i];
    const float edge1z = batch.edge1z[i];
    const float edge2x = batch.edge2x[i];
    const float edge2y = batch.edge2y[i];
    const float edge2z = batch.edge2z[i];
    
    const float pVecX = ray.direction.y * edge2z - ray.direction.z * edge2y;
    const float pVecY = ray.direction.z * edge2x - ray.direction.x * edge2z;
    const float pVecZ = ray.direction.x * edge2y - ray.direction.y * edge2x;
    const float determinant = edge1x * pVecX + edge1y * pVecY + edge1z * pVecZ;
    
    if (backfaceCulling) {
      if (determinant <= epsilon) continue;
    } else if (std::fabs(determinant) <= epsilon) {
      continue;
    }
    
    const float invDet = 1.0F / determinant;
    const float tVecX = ray.origin.x - v0x;
    const float tVecY = ray.origin.y - v0y;
    const float tVecZ = ray.origin.z - v0z;
    const float u = (tVecX * pVecX + tVecY * pVecY + tVecZ * pVecZ) * invDet;
    if (u < 0.0F || u > 1.0F) continue;
    
    const float qVecX = tVecY * edge1z - tVecZ * edge1y;
    const float qVecY = tVecZ * edge1x - tVecX * edge1z;
    const float qVecZ = tVecX * edge1y - tVecY * edge1x;
    const float v = (ray.direction.x * qVecX + ray.direction.y * qVecY + ray.direction.z * qVecZ) * invDet;
    if (v < 0.0F || (u + v) > 1.0F) continue;
    
    const float t = (edge2x * qVecX + edge2y * qVecY + edge2z * qVecZ) * invDet;
    if (t < ray.tMin || t > ray.tMax) continue;
    
    hitMask |= (1 << i);
    if (outT) outT[i] = t;
    if (outU) outU[i] = u;
    if (outV) outV[i] = v;
  }
  return hitMask;
}
#endif

} // namespace astraglyph
