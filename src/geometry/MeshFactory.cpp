#include "geometry/MeshFactory.hpp"

#include "math/Vec2.hpp"

#include <algorithm>
#include <cmath>

namespace astraglyph {
namespace {

constexpr float kPi = 3.14159265358979323846F;

Triangle makeMeshTriangle(Vec3 v0, Vec3 v1, Vec3 v2, Vec3 n0, Vec3 n1, Vec3 n2, int materialId)
{
  return Triangle{v0, v1, v2, n0, n1, n2, Vec2{}, Vec2{}, Vec2{}, materialId};
}

void addQuad(Mesh& mesh, Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 normal)
{
  mesh.addTriangle(makeMeshTriangle(a, b, c, normal, normal, normal, mesh.materialId()));
  mesh.addTriangle(makeMeshTriangle(a, c, d, normal, normal, normal, mesh.materialId()));
}

Vec3 spherePoint(float radius, float phi, float theta)
{
  const float sinPhi = std::sin(phi);
  return Vec3{
      radius * sinPhi * std::cos(theta),
      radius * std::cos(phi),
      radius * sinPhi * std::sin(theta),
  };
}

} // namespace

Mesh MeshFactory::makeTriangle(Vec3 v0, Vec3 v1, Vec3 v2, int materialId)
{
  Mesh mesh{materialId};
  const Vec3 normal = normalize(cross(v1 - v0, v2 - v0));
  mesh.addTriangle(makeMeshTriangle(v0, v1, v2, normal, normal, normal, materialId));
  return mesh;
}

Mesh MeshFactory::createPlane(float width, float depth, int materialId)
{
  const float halfWidth = std::max(width, 0.0F) * 0.5F;
  const float halfDepth = std::max(depth, 0.0F) * 0.5F;
  const Vec3 normal{0.0F, 1.0F, 0.0F};

  const Vec3 a{-halfWidth, 0.0F, -halfDepth};
  const Vec3 b{halfWidth, 0.0F, -halfDepth};
  const Vec3 c{halfWidth, 0.0F, halfDepth};
  const Vec3 d{-halfWidth, 0.0F, halfDepth};

  Mesh mesh{materialId};
  mesh.addTriangle(Triangle{a, c, b, normal, normal, normal, Vec2{0.0F, 0.0F}, Vec2{1.0F, 1.0F}, Vec2{1.0F, 0.0F}, materialId});
  mesh.addTriangle(Triangle{a, d, c, normal, normal, normal, Vec2{0.0F, 0.0F}, Vec2{0.0F, 1.0F}, Vec2{1.0F, 1.0F}, materialId});
  return mesh;
}

Mesh MeshFactory::createCube(float size, int materialId)
{
  return createCube(Vec3{size, size, size}, materialId);
}

Mesh MeshFactory::createCube(Vec3 dimensions, int materialId)
{
  const Vec3 half{
      std::max(dimensions.x, 0.0F) * 0.5F,
      std::max(dimensions.y, 0.0F) * 0.5F,
      std::max(dimensions.z, 0.0F) * 0.5F,
  };

  const float x0 = -half.x;
  const float x1 = half.x;
  const float y0 = -half.y;
  const float y1 = half.y;
  const float z0 = -half.z;
  const float z1 = half.z;

  Mesh mesh{materialId};
  addQuad(mesh, Vec3{x0, y0, z1}, Vec3{x1, y0, z1}, Vec3{x1, y1, z1}, Vec3{x0, y1, z1}, Vec3{0.0F, 0.0F, 1.0F});
  addQuad(mesh, Vec3{x1, y0, z0}, Vec3{x0, y0, z0}, Vec3{x0, y1, z0}, Vec3{x1, y1, z0}, Vec3{0.0F, 0.0F, -1.0F});
  addQuad(mesh, Vec3{x1, y0, z1}, Vec3{x1, y0, z0}, Vec3{x1, y1, z0}, Vec3{x1, y1, z1}, Vec3{1.0F, 0.0F, 0.0F});
  addQuad(mesh, Vec3{x0, y0, z0}, Vec3{x0, y0, z1}, Vec3{x0, y1, z1}, Vec3{x0, y1, z0}, Vec3{-1.0F, 0.0F, 0.0F});
  addQuad(mesh, Vec3{x0, y1, z1}, Vec3{x1, y1, z1}, Vec3{x1, y1, z0}, Vec3{x0, y1, z0}, Vec3{0.0F, 1.0F, 0.0F});
  addQuad(mesh, Vec3{x0, y0, z0}, Vec3{x1, y0, z0}, Vec3{x1, y0, z1}, Vec3{x0, y0, z1}, Vec3{0.0F, -1.0F, 0.0F});
  return mesh;
}

Mesh MeshFactory::createUvSphere(float radius, int segments, int rings, int materialId)
{
  const float safeRadius = std::max(radius, 0.0F);
  const int safeSegments = std::max(segments, 3);
  const int safeRings = std::max(rings, 2);

  Mesh mesh{materialId};
  for (int ring = 0; ring < safeRings; ++ring) {
    const float phi0 = kPi * static_cast<float>(ring) / static_cast<float>(safeRings);
    const float phi1 = kPi * static_cast<float>(ring + 1) / static_cast<float>(safeRings);

    for (int segment = 0; segment < safeSegments; ++segment) {
      const float theta0 = 2.0F * kPi * static_cast<float>(segment) / static_cast<float>(safeSegments);
      const float theta1 = 2.0F * kPi * static_cast<float>(segment + 1) / static_cast<float>(safeSegments);

      const Vec3 top = spherePoint(safeRadius, phi0, theta0);
      const Vec3 upper0 = spherePoint(safeRadius, phi0, theta0);
      const Vec3 upper1 = spherePoint(safeRadius, phi0, theta1);
      const Vec3 lower0 = spherePoint(safeRadius, phi1, theta0);
      const Vec3 lower1 = spherePoint(safeRadius, phi1, theta1);
      const Vec3 bottom = spherePoint(safeRadius, phi1, theta0);

      if (ring == 0) {
        mesh.addTriangle(makeMeshTriangle(top, lower1, lower0, normalize(top), normalize(lower1), normalize(lower0), materialId));
      } else if (ring + 1 == safeRings) {
        mesh.addTriangle(makeMeshTriangle(upper0, upper1, bottom, normalize(upper0), normalize(upper1), normalize(bottom), materialId));
      } else {
        mesh.addTriangle(makeMeshTriangle(upper0, upper1, lower0, normalize(upper0), normalize(upper1), normalize(lower0), materialId));
        mesh.addTriangle(makeMeshTriangle(upper1, lower1, lower0, normalize(upper1), normalize(lower1), normalize(lower0), materialId));
      }
    }
  }

  return mesh;
}

} // namespace astraglyph
