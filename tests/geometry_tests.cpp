#include "geometry/MeshFactory.hpp"
#include "scene/Scene.hpp"

#include <cassert>
#include <cstddef>
#include <cmath>
#include <vector>

namespace {

bool almostEqual(float lhs, float rhs, float epsilon = 1.0e-4F)
{
  return std::fabs(lhs - rhs) <= epsilon;
}

void assertVec3Near(const astraglyph::Vec3& value, const astraglyph::Vec3& expected)
{
  assert(almostEqual(value.x, expected.x));
  assert(almostEqual(value.y, expected.y));
  assert(almostEqual(value.z, expected.z));
}

} // namespace

int main()
{
  using namespace astraglyph;

  const Mesh plane = MeshFactory::createPlane(4.0F, 6.0F, 3);
  assert(plane.triangleCount() == 2U);
  assertVec3Near(plane.bounds().min, Vec3{-2.0F, 0.0F, -3.0F});
  assertVec3Near(plane.bounds().max, Vec3{2.0F, 0.0F, 3.0F});
  assert(dot(plane.triangles().front().faceNormal(), Vec3{0.0F, 1.0F, 0.0F}) > 0.99F);

  const Mesh cube = MeshFactory::createCube(2.0F, 5);
  assert(cube.triangleCount() == 12U);
  assertVec3Near(cube.bounds().min, Vec3{-1.0F, -1.0F, -1.0F});
  assertVec3Near(cube.bounds().max, Vec3{1.0F, 1.0F, 1.0F});

  constexpr int segments = 8;
  constexpr int rings = 4;
  constexpr float radius = 1.5F;
  const Mesh sphere = MeshFactory::createUvSphere(radius, segments, rings, 7);
  assert(sphere.triangleCount() == static_cast<std::size_t>(segments * 2 * (rings - 1)));
  assertVec3Near(sphere.bounds().min, Vec3{-radius, -radius, -radius});
  assertVec3Near(sphere.bounds().max, Vec3{radius, radius, radius});

  Scene scene;
  scene.addMesh(plane);
  scene.addMesh(cube);
  scene.addMesh(sphere);
  const std::vector<Triangle> triangles = scene.flattenedTriangles();
  assert(triangles.size() == plane.triangleCount() + cube.triangleCount() + sphere.triangleCount());

  const Scene defaultScene = Scene::createDefaultScene();
  assert(defaultScene.meshes().size() == 3U);
  assert(defaultScene.flattenedTriangles().size() > cube.triangleCount());
}
