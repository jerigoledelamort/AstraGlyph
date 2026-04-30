#include "geometry/ObjLoader.hpp"
#include "scene/Scene.hpp"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <stdexcept>
#include <string_view>

namespace {

bool almostEqual(float lhs, float rhs, float epsilon = 1.0e-5F)
{
  return std::fabs(lhs - rhs) <= epsilon;
}

void assertVec2Near(const astraglyph::Vec2& value, const astraglyph::Vec2& expected)
{
  assert(almostEqual(value.x, expected.x));
  assert(almostEqual(value.y, expected.y));
}

void assertVec3Near(const astraglyph::Vec3& value, const astraglyph::Vec3& expected)
{
  assert(almostEqual(value.x, expected.x));
  assert(almostEqual(value.y, expected.y));
  assert(almostEqual(value.z, expected.z));
}

std::filesystem::path resolveWorkspacePath(const std::filesystem::path& relativePath)
{
  std::filesystem::path probe = std::filesystem::current_path();
  for (int depth = 0; depth < 8; ++depth) {
    const std::filesystem::path candidate = probe / relativePath;
    if (std::filesystem::exists(candidate)) {
      return candidate;
    }

    if (!probe.has_parent_path()) {
      break;
    }
    const std::filesystem::path parent = probe.parent_path();
    if (parent == probe) {
      break;
    }
    probe = parent;
  }

  throw std::runtime_error("Failed to resolve path: " + relativePath.string());
}

} // namespace

int main()
{
  using namespace astraglyph;

  {
    constexpr std::string_view source =
        "mtllib sample.mtl\n"
        "o SampleObject\n"
        "g SampleGroup\n"
        "usemtl Bronze\n"
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "vt 0 0\n"
        "vt 1 0\n"
        "vt 0 1\n"
        "vn 0 0 2\n"
        "vn 0 0 3\n"
        "vn 0 0 4\n"
        "f 1/1/1 2/2/2 3/3/3\n";

    const Mesh mesh = ObjLoader::loadFromString(source, ObjLoadOptions{7}, "inline_full.obj");
    assert(mesh.triangleCount() == 1U);
    assert(mesh.materialId() == 7);
    assert(mesh.name() == "SampleObject");
    assert(mesh.groupName() == "SampleGroup");
    assert(mesh.sourceMaterialName() == "Bronze");
    assert(mesh.materialLibrary() == "sample.mtl");
    assert(mesh.sourcePath() == std::filesystem::path{"inline_full.obj"});

    const Triangle& triangle = mesh.triangles().front();
    assertVec2Near(triangle.uv0, Vec2{0.0F, 0.0F});
    assertVec2Near(triangle.uv1, Vec2{1.0F, 0.0F});
    assertVec2Near(triangle.uv2, Vec2{0.0F, 1.0F});
    assertVec3Near(triangle.n0, Vec3{0.0F, 0.0F, 1.0F});
    assertVec3Near(triangle.n1, Vec3{0.0F, 0.0F, 1.0F});
    assertVec3Near(triangle.n2, Vec3{0.0F, 0.0F, 1.0F});
  }

  {
    constexpr std::string_view source =
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "f 1 2 3\n";

    const Mesh mesh = ObjLoader::loadFromString(source);
    assert(mesh.triangleCount() == 1U);
    const Triangle& triangle = mesh.triangles().front();
    assertVec3Near(triangle.n0, Vec3{0.0F, 0.0F, 1.0F});
    assertVec3Near(triangle.n1, Vec3{0.0F, 0.0F, 1.0F});
    assertVec3Near(triangle.n2, Vec3{0.0F, 0.0F, 1.0F});
    assertVec2Near(triangle.uv0, Vec2{0.0F, 0.0F});
    assertVec2Near(triangle.uv1, Vec2{0.0F, 0.0F});
    assertVec2Near(triangle.uv2, Vec2{0.0F, 0.0F});
  }

  {
    constexpr std::string_view source =
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "vt 0.25 0.5\n"
        "vt 0.75 0.5\n"
        "vt 0.25 1.0\n"
        "f 1/1 2/2 3/3\n";

    const Mesh mesh = ObjLoader::loadFromString(source);
    const Triangle& triangle = mesh.triangles().front();
    assertVec2Near(triangle.uv0, Vec2{0.25F, 0.5F});
    assertVec2Near(triangle.uv1, Vec2{0.75F, 0.5F});
    assertVec2Near(triangle.uv2, Vec2{0.25F, 1.0F});
    assertVec3Near(triangle.n0, Vec3{0.0F, 0.0F, 1.0F});
  }

  {
    constexpr std::string_view source =
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "vn 0 0 1\n"
        "vn 0 0 1\n"
        "vn 0 0 1\n"
        "f 1//1 2//2 3//3\n";

    const Mesh mesh = ObjLoader::loadFromString(source);
    const Triangle& triangle = mesh.triangles().front();
    assertVec3Near(triangle.n0, Vec3{0.0F, 0.0F, 1.0F});
    assertVec3Near(triangle.n1, Vec3{0.0F, 0.0F, 1.0F});
    assertVec3Near(triangle.n2, Vec3{0.0F, 0.0F, 1.0F});
    assertVec2Near(triangle.uv0, Vec2{0.0F, 0.0F});
  }

  {
    constexpr std::string_view source =
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "vt 0 0\n"
        "vt 1 0\n"
        "vt 0 1\n"
        "vn 0 0 1\n"
        "vn 0 0 1\n"
        "vn 0 0 1\n"
        "f 1/1/1 2/2/2 3/3/3\n";

    const Mesh mesh = ObjLoader::loadFromString(source);
    const Triangle& triangle = mesh.triangles().front();
    assertVec2Near(triangle.uv1, Vec2{1.0F, 0.0F});
    assertVec3Near(triangle.n2, Vec3{0.0F, 0.0F, 1.0F});
  }

  {
    constexpr std::string_view source =
        "v -1 -1 0\n"
        "v 1 -1 0\n"
        "v 1 1 0\n"
        "v -1 1 0\n"
        "f 1 2 3 4\n";

    const Mesh mesh = ObjLoader::loadFromString(source);
    assert(mesh.triangleCount() == 2U);
    assertVec3Near(mesh.triangles()[0].faceNormal(), Vec3{0.0F, 0.0F, 1.0F});
    assertVec3Near(mesh.triangles()[1].faceNormal(), Vec3{0.0F, 0.0F, 1.0F});
    assertVec3Near(mesh.triangles()[1].v0, Vec3{-1.0F, -1.0F, 0.0F});
    assertVec3Near(mesh.triangles()[1].v1, Vec3{1.0F, 1.0F, 0.0F});
    assertVec3Near(mesh.triangles()[1].v2, Vec3{-1.0F, 1.0F, 0.0F});
  }

  {
    constexpr std::string_view source =
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 1 1 0\n"
        "v 0 1 0\n"
        "f -4 -3 -2 -1\n";

    const Mesh mesh = ObjLoader::loadFromString(source);
    assert(mesh.triangleCount() == 2U);
    assertVec3Near(mesh.triangles()[0].v0, Vec3{0.0F, 0.0F, 0.0F});
    assertVec3Near(mesh.triangles()[0].v1, Vec3{1.0F, 0.0F, 0.0F});
    assertVec3Near(mesh.triangles()[0].v2, Vec3{1.0F, 1.0F, 0.0F});
  }

  {
    constexpr std::string_view source =
        "v 0 0 0\n"
        "v 0 1 0\n"
        "v 1 0 0\n"
        "f 1 2 3\n";

    const Mesh mesh = ObjLoader::loadFromString(source);
    const Vec3 expectedNormal = normalize(cross(
        mesh.triangles().front().v1 - mesh.triangles().front().v0,
        mesh.triangles().front().v2 - mesh.triangles().front().v0));
    assertVec3Near(mesh.triangles().front().n0, expectedNormal);
    assert(almostEqual(length(mesh.triangles().front().n0), 1.0F));
  }

  {
    const std::filesystem::path cubePath = resolveWorkspacePath("assets/models/test_cube.obj");
    const Mesh cube = ObjLoader::loadFromFile(cubePath, ObjLoadOptions{3});
    assert(cube.triangleCount() == 12U);
    assert(cube.materialId() == 3);
    assert(cube.name() == "TestCube");
    assert(cube.groupName() == "Cube");
    assert(cube.sourceMaterialName() == "DefaultMaterial");
    assert(cube.materialLibrary() == "test_cube.mtl");
    assert(cube.sourcePath() == cubePath);

    Scene scene;
    scene.addMeshFromObj("assets/models/test_cube.obj", 4);
    assert(scene.meshes().size() == 1U);
    assert(scene.triangles().size() == 12U);
    assert(scene.meshes().front().materialId() == 4);
  }

  {
    const std::filesystem::path planePath = resolveWorkspacePath("assets/models/test_plane.obj");
    const Mesh plane = ObjLoader::loadFromFile(planePath, ObjLoadOptions{1});
    assert(plane.triangleCount() == 2U);
    assertVec3Near(plane.bounds().min, Vec3{-2.5F, 0.0F, -2.5F});
    assertVec3Near(plane.bounds().max, Vec3{2.5F, 0.0F, 2.5F});
  }
}
