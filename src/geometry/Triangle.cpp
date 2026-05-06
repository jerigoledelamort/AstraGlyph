#include "geometry/Triangle.hpp"

#include <cmath>

namespace astraglyph {

void Triangle::computeCachedEdges() noexcept
{
  edge1 = v1 - v0;
  edge2 = v2 - v0;
}

Aabb Triangle::bounds() const noexcept
{
  Aabb result;
  result.expand(v0);
  result.expand(v1);
  result.expand(v2);
  return result;
}

Vec3 Triangle::faceNormal() const noexcept
{
  return normalize(cross(v1 - v0, v2 - v0));
}

bool Triangle::intersect(const Ray& ray, HitInfo& hitInfo, bool backfaceCulling) const noexcept
{
  constexpr float epsilon = 1.0e-6F;

  const bool hasCachedEdge1 = edge1.lengthSquared() > epsilon;
  const bool hasCachedEdge2 = edge2.lengthSquared() > epsilon;
  const float e1x = hasCachedEdge1 ? edge1.x : v1.x - v0.x;
  const float e1y = hasCachedEdge1 ? edge1.y : v1.y - v0.y;
  const float e1z = hasCachedEdge1 ? edge1.z : v1.z - v0.z;
  const float e2x = hasCachedEdge2 ? edge2.x : v2.x - v0.x;
  const float e2y = hasCachedEdge2 ? edge2.y : v2.y - v0.y;
  const float e2z = hasCachedEdge2 ? edge2.z : v2.z - v0.z;

  const float pVecX = ray.direction.y * e2z - ray.direction.z * e2y;
  const float pVecY = ray.direction.z * e2x - ray.direction.x * e2z;
  const float pVecZ = ray.direction.x * e2y - ray.direction.y * e2x;
  const float determinant = e1x * pVecX + e1y * pVecY + e1z * pVecZ;

  if (backfaceCulling) {
    if (determinant <= epsilon) {
      return false;
    }
  } else if (std::fabs(determinant) <= epsilon) {
    return false;
  }

  const float invDet = 1.0F / determinant;
  const float tVecX = ray.origin.x - v0.x;
  const float tVecY = ray.origin.y - v0.y;
  const float tVecZ = ray.origin.z - v0.z;
  const float u = (tVecX * pVecX + tVecY * pVecY + tVecZ * pVecZ) * invDet;
  if (u < 0.0F || u > 1.0F) {
    return false;
  }

  const float qVecX = tVecY * e1z - tVecZ * e1y;
  const float qVecY = tVecZ * e1x - tVecX * e1z;
  const float qVecZ = tVecX * e1y - tVecY * e1x;
  const float v = (ray.direction.x * qVecX + ray.direction.y * qVecY + ray.direction.z * qVecZ) * invDet;
  if (v < 0.0F || (u + v) > 1.0F) {
    return false;
  }

  const float t = (e2x * qVecX + e2y * qVecY + e2z * qVecZ) * invDet;
  if (t < ray.tMin || t > ray.tMax) {
    return false;
  }

  const float w = 1.0F - u - v;
  const bool hasVertexNormals =
      lengthSquared(n0) > epsilon && lengthSquared(n1) > epsilon && lengthSquared(n2) > epsilon;

  Vec3 shadingNormal = normalize(Vec3{
      e1y * e2z - e1z * e2y,
      e1z * e2x - e1x * e2z,
      e1x * e2y - e1y * e2x,
  });
  if (hasVertexNormals) {
    shadingNormal = normalize(Vec3{
        n0.x * w + n1.x * u + n2.x * v,
        n0.y * w + n1.y * u + n2.y * v,
        n0.z * w + n1.z * u + n2.z * v,
    });
  }

  hitInfo.hit = true;
  hitInfo.t = t;
  hitInfo.position = Vec3{
      ray.origin.x + ray.direction.x * t,
      ray.origin.y + ray.direction.y * t,
      ray.origin.z + ray.direction.z * t,
  };
  hitInfo.normal = shadingNormal;
  hitInfo.barycentric = Vec3{w, u, v};
  hitInfo.uv = Vec2{
      uv0.x * w + uv1.x * u + uv2.x * v,
      uv0.y * w + uv1.y * u + uv2.y * v,
  };
  hitInfo.materialId = materialId;
  return true;
}

bool Triangle::intersect(const Ray& ray, float& t) const noexcept
{
  constexpr float epsilon = 1.0e-6F;

  const bool hasCachedEdge1 = edge1.lengthSquared() > epsilon;
  const bool hasCachedEdge2 = edge2.lengthSquared() > epsilon;
  const float e1x = hasCachedEdge1 ? edge1.x : v1.x - v0.x;
  const float e1y = hasCachedEdge1 ? edge1.y : v1.y - v0.y;
  const float e1z = hasCachedEdge1 ? edge1.z : v1.z - v0.z;
  const float e2x = hasCachedEdge2 ? edge2.x : v2.x - v0.x;
  const float e2y = hasCachedEdge2 ? edge2.y : v2.y - v0.y;
  const float e2z = hasCachedEdge2 ? edge2.z : v2.z - v0.z;

  const float pVecX = ray.direction.y * e2z - ray.direction.z * e2y;
  const float pVecY = ray.direction.z * e2x - ray.direction.x * e2z;
  const float pVecZ = ray.direction.x * e2y - ray.direction.y * e2x;
  const float determinant = e1x * pVecX + e1y * pVecY + e1z * pVecZ;
  if (std::fabs(determinant) <= epsilon) {
    return false;
  }

  const float invDet = 1.0F / determinant;
  const float tVecX = ray.origin.x - v0.x;
  const float tVecY = ray.origin.y - v0.y;
  const float tVecZ = ray.origin.z - v0.z;
  const float u = (tVecX * pVecX + tVecY * pVecY + tVecZ * pVecZ) * invDet;
  if (u < 0.0F || u > 1.0F) {
    return false;
  }

  const float qVecX = tVecY * e1z - tVecZ * e1y;
  const float qVecY = tVecZ * e1x - tVecX * e1z;
  const float qVecZ = tVecX * e1y - tVecY * e1x;
  const float v = (ray.direction.x * qVecX + ray.direction.y * qVecY + ray.direction.z * qVecZ) * invDet;
  if (v < 0.0F || (u + v) > 1.0F) {
    return false;
  }

  t = (e2x * qVecX + e2y * qVecY + e2z * qVecZ) * invDet;
  if (t < ray.tMin || t > ray.tMax) {
    return false;
  }

  return true;
}

bool Triangle::intersectAny(const Ray& ray, bool backfaceCulling) const noexcept
{
  constexpr float epsilon = 1.0e-6F;

  const bool hasCachedEdge1 = edge1.lengthSquared() > epsilon;
  const bool hasCachedEdge2 = edge2.lengthSquared() > epsilon;
  const float e1x = hasCachedEdge1 ? edge1.x : v1.x - v0.x;
  const float e1y = hasCachedEdge1 ? edge1.y : v1.y - v0.y;
  const float e1z = hasCachedEdge1 ? edge1.z : v1.z - v0.z;
  const float e2x = hasCachedEdge2 ? edge2.x : v2.x - v0.x;
  const float e2y = hasCachedEdge2 ? edge2.y : v2.y - v0.y;
  const float e2z = hasCachedEdge2 ? edge2.z : v2.z - v0.z;

  const float pVecX = ray.direction.y * e2z - ray.direction.z * e2y;
  const float pVecY = ray.direction.z * e2x - ray.direction.x * e2z;
  const float pVecZ = ray.direction.x * e2y - ray.direction.y * e2x;
  const float determinant = e1x * pVecX + e1y * pVecY + e1z * pVecZ;

  if (backfaceCulling) {
    if (determinant <= epsilon) {
      return false;
    }
  } else if (std::fabs(determinant) <= epsilon) {
    return false;
  }

  const float invDet = 1.0F / determinant;
  const float tVecX = ray.origin.x - v0.x;
  const float tVecY = ray.origin.y - v0.y;
  const float tVecZ = ray.origin.z - v0.z;
  const float u = (tVecX * pVecX + tVecY * pVecY + tVecZ * pVecZ) * invDet;
  if (u < 0.0F || u > 1.0F) {
    return false;
  }

  const float qVecX = tVecY * e1z - tVecZ * e1y;
  const float qVecY = tVecZ * e1x - tVecX * e1z;
  const float qVecZ = tVecX * e1y - tVecY * e1x;
  const float v = (ray.direction.x * qVecX + ray.direction.y * qVecY + ray.direction.z * qVecZ) * invDet;
  if (v < 0.0F || (u + v) > 1.0F) {
    return false;
  }

  const float t = (e2x * qVecX + e2y * qVecY + e2z * qVecZ) * invDet;
  return t >= ray.tMin && t <= ray.tMax;
}

} // namespace astraglyph
