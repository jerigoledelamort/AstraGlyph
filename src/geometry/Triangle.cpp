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

  // Compute edges if not cached (for backwards compatibility)
  const Vec3 localEdge1 = edge1.lengthSquared() > epsilon ? edge1 : (v1 - v0);
  const Vec3 localEdge2 = edge2.lengthSquared() > epsilon ? edge2 : (v2 - v0);

  const Vec3 pVec = cross(ray.direction, localEdge2);
  const float determinant = dot(localEdge1, pVec);

  if (backfaceCulling) {
    if (determinant <= epsilon) {
      return false;
    }
  } else if (std::fabs(determinant) <= epsilon) {
    return false;
  }

  const float invDet = 1.0F / determinant;
  const Vec3 tVec = ray.origin - v0;
  const float u = dot(tVec, pVec) * invDet;
  if (u < 0.0F || u > 1.0F) {
    return false;
  }

  const Vec3 qVec = cross(tVec, localEdge1);
  const float v = dot(ray.direction, qVec) * invDet;
  if (v < 0.0F || (u + v) > 1.0F) {
    return false;
  }

  const float t = dot(localEdge2, qVec) * invDet;
  if (t < ray.tMin || t > ray.tMax) {
    return false;
  }

  const float w = 1.0F - u - v;
  const bool hasVertexNormals =
      lengthSquared(n0) > epsilon && lengthSquared(n1) > epsilon && lengthSquared(n2) > epsilon;

  Vec3 shadingNormal = normalize(cross(localEdge1, localEdge2));
  if (hasVertexNormals) {
    shadingNormal = normalize(n0 * w + n1 * u + n2 * v);
  }

  hitInfo.hit = true;
  hitInfo.t = t;
  hitInfo.position = ray.at(t);
  hitInfo.normal = shadingNormal;
  hitInfo.barycentric = Vec3{w, u, v};
  hitInfo.materialId = materialId;
  return true;
}

bool Triangle::intersect(const Ray& ray, float& t) const noexcept
{
  HitInfo hitInfo{};
  if (!intersect(ray, hitInfo, false)) {
    return false;
  }

  t = hitInfo.t;
  return true;
}

bool Triangle::intersectAny(const Ray& ray, bool backfaceCulling) const noexcept
{
  constexpr float epsilon = 1.0e-6F;

  // Compute edges if not cached (for backwards compatibility)
  const Vec3 localEdge1 = edge1.lengthSquared() > epsilon ? edge1 : (v1 - v0);
  const Vec3 localEdge2 = edge2.lengthSquared() > epsilon ? edge2 : (v2 - v0);

  const Vec3 pVec = cross(ray.direction, localEdge2);
  const float determinant = dot(localEdge1, pVec);

  if (backfaceCulling) {
    if (determinant <= epsilon) {
      return false;
    }
  } else if (std::fabs(determinant) <= epsilon) {
    return false;
  }

  const float invDet = 1.0F / determinant;
  const Vec3 tVec = ray.origin - v0;
  const float u = dot(tVec, pVec) * invDet;
  if (u < 0.0F || u > 1.0F) {
    return false;
  }

  const Vec3 qVec = cross(tVec, localEdge1);
  const float v = dot(ray.direction, qVec) * invDet;
  if (v < 0.0F || (u + v) > 1.0F) {
    return false;
  }

  const float t = dot(localEdge2, qVec) * invDet;
  return t >= ray.tMin && t <= ray.tMax;
}

} // namespace astraglyph
