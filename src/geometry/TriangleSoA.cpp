#include "geometry/TriangleSoA.hpp"
#include "geometry/Triangle.hpp"

#include <cstddef>

namespace astraglyph {

void TriangleSoA::reserve(std::size_t count)
{
  v0x.reserve(count); v0y.reserve(count); v0z.reserve(count);
  v1x.reserve(count); v1y.reserve(count); v1z.reserve(count);
  v2x.reserve(count); v2y.reserve(count); v2z.reserve(count);
  n0x.reserve(count); n0y.reserve(count); n0z.reserve(count);
  n1x.reserve(count); n1y.reserve(count); n1z.reserve(count);
  n2x.reserve(count); n2y.reserve(count); n2z.reserve(count);
  uv0x.reserve(count); uv0y.reserve(count);
  uv1x.reserve(count); uv1y.reserve(count);
  uv2x.reserve(count); uv2y.reserve(count);
  materialIds.reserve(count);
  edge1x.reserve(count); edge1y.reserve(count); edge1z.reserve(count);
  edge2x.reserve(count); edge2y.reserve(count); edge2z.reserve(count);
  boundsMinX.reserve(count); boundsMinY.reserve(count); boundsMinZ.reserve(count);
  boundsMaxX.reserve(count); boundsMaxY.reserve(count); boundsMaxZ.reserve(count);
}

void TriangleSoA::pushTriangle(const Triangle& tri)
{
  v0x.push_back(tri.v0.x); v0y.push_back(tri.v0.y); v0z.push_back(tri.v0.z);
  v1x.push_back(tri.v1.x); v1y.push_back(tri.v1.y); v1z.push_back(tri.v1.z);
  v2x.push_back(tri.v2.x); v2y.push_back(tri.v2.y); v2z.push_back(tri.v2.z);
  
  n0x.push_back(tri.n0.x); n0y.push_back(tri.n0.y); n0z.push_back(tri.n0.z);
  n1x.push_back(tri.n1.x); n1y.push_back(tri.n1.y); n1z.push_back(tri.n1.z);
  n2x.push_back(tri.n2.x); n2y.push_back(tri.n2.y); n2z.push_back(tri.n2.z);
  
  uv0x.push_back(tri.uv0.x); uv0y.push_back(tri.uv0.y);
  uv1x.push_back(tri.uv1.x); uv1y.push_back(tri.uv1.y);
  uv2x.push_back(tri.uv2.x); uv2y.push_back(tri.uv2.y);
  
  materialIds.push_back(tri.materialId);
  
  edge1x.push_back(tri.edge1.x); edge1y.push_back(tri.edge1.y); edge1z.push_back(tri.edge1.z);
  edge2x.push_back(tri.edge2.x); edge2y.push_back(tri.edge2.y); edge2z.push_back(tri.edge2.z);
  
  const Aabb bounds = tri.bounds();
  boundsMinX.push_back(bounds.min.x); boundsMinY.push_back(bounds.min.y); boundsMinZ.push_back(bounds.min.z);
  boundsMaxX.push_back(bounds.max.x); boundsMaxY.push_back(bounds.max.y); boundsMaxZ.push_back(bounds.max.z);
}

void TriangleBatchSoA::packTriangles(const TriangleSoA& triangles, std::size_t offset, int count)
{
  triangleCount = std::min(count, 8);
  for (int i = 0; i < 8; ++i) {
    if (i < triangleCount) {
      const std::size_t idx = offset + static_cast<std::size_t>(i);
      v0x[i] = triangles.v0x[idx]; v0y[i] = triangles.v0y[idx]; v0z[i] = triangles.v0z[idx];
      v1x[i] = triangles.v1x[idx]; v1y[i] = triangles.v1y[idx]; v1z[i] = triangles.v1z[idx];
      v2x[i] = triangles.v2x[idx]; v2y[i] = triangles.v2y[idx]; v2z[i] = triangles.v2z[idx];
      edge1x[i] = triangles.edge1x[idx]; edge1y[i] = triangles.edge1y[idx]; edge1z[i] = triangles.edge1z[idx];
      edge2x[i] = triangles.edge2x[idx]; edge2y[i] = triangles.edge2y[idx]; edge2z[i] = triangles.edge2z[idx];
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

void TriangleBatchSoA::packTriangles(const std::vector<Triangle>& triangles, std::size_t offset, int count)
{
  triangleCount = std::min(count, 8);
  for (int i = 0; i < 8; ++i) {
    if (i < triangleCount) {
      const Triangle& tri = triangles[offset + static_cast<std::size_t>(i)];
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
