#include "render/SimdTriangleSoA.hpp"

#if ASTRAGLYPH_SIMD_SSE3

namespace astraglyph {

// SSE3 SIMD-оптимизированная версия batch intersection (SoA версия)
[[nodiscard]] int intersectBatchSimd(
    const TriangleBatchSoA& batch,
    const Ray& ray,
    bool backfaceCulling,
    float* outT,
    float* outU,
    float* outV) noexcept
{
  constexpr float epsilon = 1.0e-6F;
  
  // Загрузка направления луча (4 float)
  const __m128 rayDirX = _mm_set1_ps(ray.direction.x);
  const __m128 rayDirY = _mm_set1_ps(ray.direction.y);
  const __m128 rayDirZ = _mm_set1_ps(ray.direction.z);
  const __m128 rayOriginX = _mm_set1_ps(ray.origin.x);
  const __m128 rayOriginY = _mm_set1_ps(ray.origin.y);
  const __m128 rayOriginZ = _mm_set1_ps(ray.origin.z);
  const __m128 rayTMin = _mm_set1_ps(ray.tMin);
  const __m128 rayTMax = _mm_set1_ps(ray.tMax);
  const __m128 eps = _mm_set1_ps(epsilon);
  const __m128 zero = _mm_setzero_ps();
  const __m128 one = _mm_set1_ps(1.0F);
  
  // Загрузка данных треугольников (4 треугольника)
  const __m128 v0x = _mm_load_ps(batch.v0x);
  const __m128 v0y = _mm_load_ps(batch.v0y);
  const __m128 v0z = _mm_load_ps(batch.v0z);
  const __m128 edge1x = _mm_load_ps(batch.edge1x);
  const __m128 edge1y = _mm_load_ps(batch.edge1y);
  const __m128 edge1z = _mm_load_ps(batch.edge1z);
  const __m128 edge2x = _mm_load_ps(batch.edge2x);
  const __m128 edge2y = _mm_load_ps(batch.edge2y);
  const __m128 edge2z = _mm_load_ps(batch.edge2z);
  
  // TVector = ray.origin - v0
  const __m128 tVecX = _mm_sub_ps(rayOriginX, v0x);
  const __m128 tVecY = _mm_sub_ps(rayOriginY, v0y);
  const __m128 tVecZ = _mm_sub_ps(rayOriginZ, v0z);
  
  // PVector = cross(ray.direction, edge2)
  const __m128 pVecX = _mm_sub_ps(_mm_mul_ps(rayDirY, edge2z), _mm_mul_ps(rayDirZ, edge2y));
  const __m128 pVecY = _mm_sub_ps(_mm_mul_ps(rayDirZ, edge2x), _mm_mul_ps(rayDirX, edge2z));
  const __m128 pVecZ = _mm_sub_ps(_mm_mul_ps(rayDirX, edge2y), _mm_mul_ps(rayDirY, edge2x));
  
  // Determinant = dot(edge1, pVec)
  __m128 determinant = _mm_add_ps(
      _mm_mul_ps(edge1x, pVecX),
      _mm_add_ps(_mm_mul_ps(edge1y, pVecY), _mm_mul_ps(edge1z, pVecZ)));
  
  // Check determinant
  __m128 detMask;
  if (backfaceCulling) {
    detMask = _mm_cmpgt_ps(determinant, eps);
  } else {
    const __m128 absDet = _mm_and_ps(determinant, _mm_set1_ps(0x7FFFFFFF));
    detMask = _mm_cmpgt_ps(absDet, eps);
  }
  
  // Inverse determinant
  const __m128 invDet = _mm_div_ps(one, determinant);
  
  // U = dot(tVec, pVec) * invDet
  __m128 u = _mm_mul_ps(
      _mm_add_ps(_mm_mul_ps(tVecX, pVecX), _mm_add_ps(_mm_mul_ps(tVecY, pVecY), _mm_mul_ps(tVecZ, pVecZ))),
      invDet);
  
  // Check u range [0, 1]
  __m128 uMask = _mm_and_ps(
      _mm_cmpge_ps(u, zero),
      _mm_cmple_ps(u, one));
  
  // QVector = cross(tVec, edge1)
  const __m128 qVecX = _mm_sub_ps(_mm_mul_ps(tVecY, edge1z), _mm_mul_ps(tVecZ, edge1y));
  const __m128 qVecY = _mm_sub_ps(_mm_mul_ps(tVecZ, edge1x), _mm_mul_ps(tVecX, edge1z));
  const __m128 qVecZ = _mm_sub_ps(_mm_mul_ps(tVecX, edge1y), _mm_mul_ps(tVecY, edge1x));
  
  // V = dot(ray.direction, qVec) * invDet
  __m128 v = _mm_mul_ps(
      _mm_add_ps(_mm_mul_ps(rayDirX, qVecX), _mm_add_ps(_mm_mul_ps(rayDirY, qVecY), _mm_mul_ps(rayDirZ, qVecZ))),
      invDet);
  
  // Check v >= 0 && u + v <= 1
  const __m128 uvSum = _mm_add_ps(u, v);
  __m128 vMask = _mm_and_ps(
      _mm_cmpge_ps(v, zero),
      _mm_cmple_ps(uvSum, one));
  
  // T = dot(edge2, qVec) * invDet
  __m128 t = _mm_mul_ps(
      _mm_add_ps(_mm_mul_ps(edge2x, qVecX), _mm_add_ps(_mm_mul_ps(edge2y, qVecY), _mm_mul_ps(edge2z, qVecZ))),
      invDet);
  
  // Check t in [ray.tMin, ray.tMax]
  __m128 tMask = _mm_and_ps(
      _mm_cmpge_ps(t, rayTMin),
      _mm_cmple_ps(t, rayTMax));
  
  // Combine all masks
  __m128 hitMask = _mm_and_ps(detMask, _mm_and_ps(uMask, _mm_and_ps(vMask, tMask)));
  
  // Convert mask to integer bitmask
  const int maskInt = _mm_movemask_ps(hitMask);
  
  // Write output for hits
  if (outT || outU || outV) {
    alignas(16) float tArr[4];
    alignas(16) float uArr[4];
    alignas(16) float vArr[4];
    
    _mm_store_ps(tArr, t);
    _mm_store_ps(uArr, u);
    _mm_store_ps(vArr, v);
    
    for (int i = 0; i < batch.triangleCount; ++i) {
      if (maskInt & (1 << i)) {
        if (outT) outT[i] = tArr[i];
        if (outU) outU[i] = uArr[i];
        if (outV) outV[i] = vArr[i];
      }
    }
  }
  
  return maskInt & ((1 << batch.triangleCount) - 1);
}

} // namespace astraglyph

#endif // ASTRAGLYPH_SIMD_SSE3
