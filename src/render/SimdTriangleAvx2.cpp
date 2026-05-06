#include "render/SimdTriangle.hpp"

#if ASTRAGLYPH_SIMD_AVX2

namespace astraglyph {

// AVX2 SIMD-оптимизированная версия batch intersection
// Обрабатывает до 8 треугольников параллельно
[[nodiscard]] int intersectBatchSimd(
    const TriangleBatch& batch,
    const Ray& ray,
    bool backfaceCulling,
    float* outT,
    float* outU,
    float* outV) noexcept
{
  constexpr float epsilon = 1.0e-6F;
  
  // Загрузка направления луча
  const __m256 rayDirX = _mm256_set1_ps(ray.direction.x);
  const __m256 rayDirY = _mm256_set1_ps(ray.direction.y);
  const __m256 rayDirZ = _mm256_set1_ps(ray.direction.z);
  const __m256 rayOriginX = _mm256_set1_ps(ray.origin.x);
  const __m256 rayOriginY = _mm256_set1_ps(ray.origin.y);
  const __m256 rayOriginZ = _mm256_set1_ps(ray.origin.z);
  const __m256 rayTMin = _mm256_set1_ps(ray.tMin);
  const __m256 rayTMax = _mm256_set1_ps(ray.tMax);
  const __m256 eps = _mm256_set1_ps(epsilon);
  const __m256 zero = _mm256_setzero_ps();
  const __m256 one = _mm256_set1_ps(1.0F);
  
  // Загрузка данных треугольников
  const __m256 v0x = _mm256_load_ps(batch.v0x);
  const __m256 v0y = _mm256_load_ps(batch.v0y);
  const __m256 v0z = _mm256_load_ps(batch.v0z);
  // v1 и v2 не нужны (используются только edge1/edge2)
  (void)batch.v1x; (void)batch.v1y; (void)batch.v1z;
  (void)batch.v2x; (void)batch.v2y; (void)batch.v2z;
  const __m256 edge1x = _mm256_load_ps(batch.edge1x);
  const __m256 edge1y = _mm256_load_ps(batch.edge1y);
  const __m256 edge1z = _mm256_load_ps(batch.edge1z);
  const __m256 edge2x = _mm256_load_ps(batch.edge2x);
  const __m256 edge2y = _mm256_load_ps(batch.edge2y);
  const __m256 edge2z = _mm256_load_ps(batch.edge2z);
  
  // TVector = ray.origin - v0
  const __m256 tVecX = _mm256_sub_ps(rayOriginX, v0x);
  const __m256 tVecY = _mm256_sub_ps(rayOriginY, v0y);
  const __m256 tVecZ = _mm256_sub_ps(rayOriginZ, v0z);
  
  // PVector = cross(ray.direction, edge2)
  // pVecX = rayDirY * edge2Z - rayDirZ * edge2Y
  // pVecY = rayDirZ * edge2X - rayDirX * edge2Z
  // pVecZ = rayDirX * edge2Y - rayDirY * edge2X
  const __m256 pVecX = _mm256_sub_ps(_mm256_mul_ps(rayDirY, edge2z), _mm256_mul_ps(rayDirZ, edge2y));
  const __m256 pVecY = _mm256_sub_ps(_mm256_mul_ps(rayDirZ, edge2x), _mm256_mul_ps(rayDirX, edge2z));
  const __m256 pVecZ = _mm256_sub_ps(_mm256_mul_ps(rayDirX, edge2y), _mm256_mul_ps(rayDirY, edge2x));
  
  // Determinant = dot(edge1, pVec)
  __m256 determinant = _mm256_add_ps(
      _mm256_mul_ps(edge1x, pVecX),
      _mm256_add_ps(_mm256_mul_ps(edge1y, pVecY), _mm256_mul_ps(edge1z, pVecZ)));
  
  // Check determinant
  __m256 detMask;
  if (backfaceCulling) {
    // determinant > epsilon
    detMask = _mm256_cmp_ps(determinant, eps, _CMP_GT_OQ);
  } else {
    // fabs(determinant) > epsilon
    const __m256 absDet = _mm256_and_ps(determinant, _mm256_set1_ps(0x7FFFFFFF));
    detMask = _mm256_cmp_ps(absDet, eps, _CMP_GT_OQ);
  }
  
  // Inverse determinant
  const __m256 invDet = _mm256_div_ps(one, determinant);
  
  // U = dot(tVec, pVec) * invDet
  __m256 u = _mm256_mul_ps(
      _mm256_add_ps(_mm256_mul_ps(tVecX, pVecX), _mm256_add_ps(_mm256_mul_ps(tVecY, pVecY), _mm256_mul_ps(tVecZ, pVecZ))),
      invDet);
  
  // Check u range [0, 1]
  __m256 uMask = _mm256_and_ps(
      _mm256_cmp_ps(u, zero, _CMP_GE_OQ),
      _mm256_cmp_ps(u, one, _CMP_LE_OQ));
  
  // QVector = cross(tVec, edge1)
  // qVecX = tVecY * edge1Z - tVecZ * edge1Y
  // qVecY = tVecZ * edge1X - tVecX * edge1Z
  // qVecZ = tVecX * edge1Y - tVecY * edge1X
  const __m256 qVecX = _mm256_sub_ps(_mm256_mul_ps(tVecY, edge1z), _mm256_mul_ps(tVecZ, edge1y));
  const __m256 qVecY = _mm256_sub_ps(_mm256_mul_ps(tVecZ, edge1x), _mm256_mul_ps(tVecX, edge1z));
  const __m256 qVecZ = _mm256_sub_ps(_mm256_mul_ps(tVecX, edge1y), _mm256_mul_ps(tVecY, edge1x));
  
  // V = dot(ray.direction, qVec) * invDet
  __m256 v = _mm256_mul_ps(
      _mm256_add_ps(_mm256_mul_ps(rayDirX, qVecX), _mm256_add_ps(_mm256_mul_ps(rayDirY, qVecY), _mm256_mul_ps(rayDirZ, qVecZ))),
      invDet);
  
  // Check v >= 0 && u + v <= 1
  const __m256 uvSum = _mm256_add_ps(u, v);
  __m256 vMask = _mm256_and_ps(
      _mm256_cmp_ps(v, zero, _CMP_GE_OQ),
      _mm256_cmp_ps(uvSum, one, _CMP_LE_OQ));
  
  // T = dot(edge2, qVec) * invDet
  __m256 t = _mm256_mul_ps(
      _mm256_add_ps(_mm256_mul_ps(edge2x, qVecX), _mm256_add_ps(_mm256_mul_ps(edge2y, qVecY), _mm256_mul_ps(edge2z, qVecZ))),
      invDet);
  
  // Check t in [ray.tMin, ray.tMax]
  __m256 tMask = _mm256_and_ps(
      _mm256_cmp_ps(t, rayTMin, _CMP_GE_OQ),
      _mm256_cmp_ps(t, rayTMax, _CMP_LE_OQ));
  
  // Combine all masks
  __m256 hitMask = _mm256_and_ps(detMask, _mm256_and_ps(uMask, _mm256_and_ps(vMask, tMask)));
  
  // Convert mask to integer bitmask
  const int maskInt = _mm256_movemask_ps(hitMask);
  
  // Write output for hits (only for first batch.triangleCount triangles)
  if (outT || outU || outV) {
    alignas(32) float tArr[8];
    alignas(32) float uArr[8];
    alignas(32) float vArr[8];
    
    _mm256_store_ps(tArr, t);
    _mm256_store_ps(uArr, u);
    _mm256_store_ps(vArr, v);
    
    for (int i = 0; i < batch.triangleCount; ++i) {
      if (maskInt & (1 << i)) {
        if (outT) outT[i] = tArr[i];
        if (outU) outU[i] = uArr[i];
        if (outV) outV[i] = vArr[i];
      }
    }
  }
  
  // Zero out hits beyond batch.triangleCount
  return maskInt & ((1 << batch.triangleCount) - 1);
}

} // namespace astraglyph

#endif // ASTRAGLYPH_SIMD_AVX2
