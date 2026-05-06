#pragma once

#include "geometry/Triangle.hpp"
#include "math/Ray.hpp"
#include "math/Vec3.hpp"

#include <vector>

// Forward declaration для RayPacket
namespace astraglyph {
struct RayPacket;
struct RayPacketHitInfo;
}

// SIMD оптимизация включается через флаг ASTRAGLYPH_ENABLE_SIMD
#if defined(ASTRAGLYPH_ENABLE_SIMD)
  #if defined(__AVX2__) && defined(__FMA__)
    #define ASTRAGLYPH_SIMD_AVX2 1
    #include <immintrin.h>
  #elif defined(__SSE3__)
    #define ASTRAGLYPH_SIMD_SSE3 1
    #include <pmmintrin.h>
  #endif
#endif

namespace astraglyph {

// Бatching-структура для SIMD-обработки нескольких треугольников
struct TriangleBatch {
  // Выровненные данные для SIMD (8 треугольников для AVX2, 4 для SSE)
  alignas(32) float v0x[8];
  alignas(32) float v0y[8];
  alignas(32) float v0z[8];
  alignas(32) float v1x[8];
  alignas(32) float v1y[8];
  alignas(32) float v1z[8];
  alignas(32) float v2x[8];
  alignas(32) float v2y[8];
  alignas(32) float v2z[8];
  alignas(32) float edge1x[8];
  alignas(32) float edge1y[8];
  alignas(32) float edge1z[8];
  alignas(32) float edge2x[8];
  alignas(32) float edge2y[8];
  alignas(32) float edge2z[8];
  
  int triangleCount{0};
  
  void packTriangles(const std::vector<Triangle>& triangles, int offset, int count);
};

// SIMD-оптимизированное пересечение луча с batch треугольников
// Возвращает bitmask где каждый бит указывает на попадание
#if defined(ASTRAGLYPH_SIMD_AVX2)
[[nodiscard]] int intersectBatchSimd(
    const TriangleBatch& batch,
    const Ray& ray,
    bool backfaceCulling,
    float* outT = nullptr,
    float* outU = nullptr,
    float* outV = nullptr) noexcept;
#elif defined(ASTRAGLYPH_SIMD_SSE3)
[[nodiscard]] int intersectBatchSimd(
    const TriangleBatch& batch,
    const Ray& ray,
    bool backfaceCulling,
    float* outT = nullptr,
    float* outU = nullptr,
    float* outV = nullptr) noexcept;
#else
// Fallback на scalar версию (полная идентичность с Triangle::intersect)
[[nodiscard]] inline int intersectBatchSimd(
    const TriangleBatch& batch,
    const Ray& ray,
    bool backfaceCulling,
    float* outT = nullptr,
    float* outU = nullptr,
    float* outV = nullptr) noexcept
{
  int hitMask = 0;
  constexpr float epsilon = 1.0e-6F;
  
  for (int i = 0; i < batch.triangleCount; ++i) {
    // Данные уже упакованы в batch
    const float v0x = batch.v0x[i];
    const float v0y = batch.v0y[i];
    const float v0z = batch.v0z[i];
    // v1 и v2 не нужны для intersection (используются только edge1/edge2)
    (void)batch.v1x; (void)batch.v1y; (void)batch.v1z;
    (void)batch.v2x; (void)batch.v2y; (void)batch.v2z;
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

// Утилита для упаковки треугольников в batch
inline void TriangleBatch::packTriangles(const std::vector<Triangle>& triangles, int offset, int count)
{
  triangleCount = std::min(count, 8);
  for (int i = 0; i < 8; ++i) {
    if (i < triangleCount) {
      const Triangle& tri = triangles[static_cast<std::size_t>(offset + i)];
      v0x[i] = tri.v0.x; v0y[i] = tri.v0.y; v0z[i] = tri.v0.z;
      v1x[i] = tri.v1.x; v1y[i] = tri.v1.y; v1z[i] = tri.v1.z;
      v2x[i] = tri.v2.x; v2y[i] = tri.v2.y; v2z[i] = tri.v2.z;
      
      const bool hasCachedEdge1 = tri.edge1.lengthSquared() > 1.0e-6F;
      const bool hasCachedEdge2 = tri.edge2.lengthSquared() > 1.0e-6F;
      
      edge1x[i] = hasCachedEdge1 ? tri.edge1.x : tri.v1.x - tri.v0.x;
      edge1y[i] = hasCachedEdge1 ? tri.edge1.y : tri.v1.y - tri.v0.y;
      edge1z[i] = hasCachedEdge1 ? tri.edge1.z : tri.v1.z - tri.v0.z;
      edge2x[i] = hasCachedEdge2 ? tri.edge2.x : tri.v2.x - tri.v0.x;
      edge2y[i] = hasCachedEdge2 ? tri.edge2.y : tri.v2.y - tri.v0.y;
      edge2z[i] = hasCachedEdge2 ? tri.edge2.z : tri.v2.z - tri.v0.z;
    } else {
      // Padding zeros
      v0x[i] = v0y[i] = v0z[i] = 0.0F;
      v1x[i] = v1y[i] = v1z[i] = 0.0F;
      v2x[i] = v2y[i] = v2z[i] = 0.0F;
      edge1x[i] = edge1y[i] = edge1z[i] = 0.0F;
      edge2x[i] = edge2y[i] = edge2z[i] = 0.0F;
    }
  }
}

} // namespace astraglyph

// Быстрая проверка пересечения (без barycentric coords) для intersectAny
#if defined(ASTRAGLYPH_SIMD_AVX2)
namespace astraglyph {
[[nodiscard]] bool intersectAnyBatchSimd(
    const TriangleBatch& batch,
    const Ray& ray,
    bool backfaceCulling) noexcept;
}
#elif defined(ASTRAGLYPH_SIMD_SSE3)
namespace astraglyph {
[[nodiscard]] bool intersectAnyBatchSimd(
    const TriangleBatch& batch,
    const Ray& ray,
    bool backfaceCulling) noexcept;
}
#else
namespace astraglyph {
[[nodiscard]] inline bool intersectAnyBatchSimd(
    const TriangleBatch& batch,
    const Ray& ray,
    bool backfaceCulling) noexcept
{
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
    if (t >= ray.tMin && t <= ray.tMax) return true;
  }
  return false;
}
} // namespace astraglyph
#endif

// SIMD packet tracing - пересечение пакета лучей с batch треугольников
// Возвращает hitMask для каждого луча в пакете
// Реализация в SimdTriangleSse3.cpp / SimdTriangleAvx2.cpp
#if defined(ASTRAGLYPH_SIMD_AVX2)
namespace astraglyph {
[[nodiscard]] int intersectPacketBatchSimd(
    const RayPacket& packet,
    const TriangleBatch& batch,
    bool backfaceCulling,
    RayPacketHitInfo* outInfo = nullptr) noexcept;
}
#elif defined(ASTRAGLYPH_SIMD_SSE3)
namespace astraglyph {
[[nodiscard]] int intersectPacketBatchSimd(
    const RayPacket& packet,
    const TriangleBatch& batch,
    bool backfaceCulling,
    RayPacketHitInfo* outInfo = nullptr) noexcept;
}
#else
// Нет SIMD поддержки - возвращаем 0
namespace astraglyph {
[[nodiscard]] inline int intersectPacketBatchSimd(
    const RayPacket& /* packet */,
    const TriangleBatch& /* batch */,
    bool /* backfaceCulling */,
    RayPacketHitInfo* /* outInfo */) noexcept
{
  return 0; // Нет SIMD поддержки
}
} // namespace astraglyph
#endif
