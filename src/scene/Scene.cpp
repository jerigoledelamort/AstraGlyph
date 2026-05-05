#include "scene/Scene.hpp"

#include "geometry/ObjLoader.hpp"
#include "geometry/MeshFactory.hpp"

#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <utility>

namespace astraglyph {
namespace {

[[nodiscard]] std::filesystem::path resolveScenePath(const std::filesystem::path& path)
{
  if (path.is_absolute()) {
    return path;
  }

  std::filesystem::path probe = std::filesystem::current_path();
  for (int depth = 0; depth < 8; ++depth) {
    const std::filesystem::path candidate = probe / path;
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

  return path;
}

} // namespace

void Scene::addEntity(Entity entity)
{
  addMesh(entity.mesh);
  entities_.push_back(std::move(entity));
}

void Scene::addMesh(Mesh mesh)
{
  const auto& meshTriangles = mesh.triangles();
  triangles_.insert(triangles_.end(), meshTriangles.begin(), meshTriangles.end());
  meshes_.push_back(std::move(mesh));
}

void Scene::addMeshFromObj(const std::filesystem::path& path, int materialId)
{
  Mesh mesh = ObjLoader::loadFromFile(resolveScenePath(path), ObjLoadOptions{materialId});
  mesh.setMaterialId(materialId);
  addMesh(std::move(mesh));
}

void Scene::addMaterial(Material material)
{
  materials_.push_back(material);
}

void Scene::addLight(Light light)
{
  lights_.push_back(light);
}

const std::vector<Entity>& Scene::entities() const noexcept
{
  return entities_;
}

const std::vector<Mesh>& Scene::meshes() const noexcept
{
  return meshes_;
}

const std::vector<Material>& Scene::materials() const noexcept
{
  return materials_;
}

std::size_t Scene::getMaterialCount() const noexcept
{
  return materials_.size();
}

const std::vector<Light>& Scene::lights() const noexcept
{
  return lights_;
}

const std::vector<Triangle>& Scene::triangles() const noexcept
{
  return triangles_;
}

const Material& Scene::materialFor(int materialId) const noexcept
{
  static const Material fallback{};
  if (materialId < 0 || static_cast<std::size_t>(materialId) >= materials_.size()) {
    return fallback;
  }

  return materials_[static_cast<std::size_t>(materialId)];
}

std::vector<Triangle> Scene::flattenedTriangles() const
{
  return triangles_;
}

Scene Scene::createDefaultScene()
{
  Scene scene;
  scene.addMaterial(Material{Vec3{0.72F, 0.72F, 0.70F}, 0.8F, 0.0F, 0.0F});
  scene.addMaterial(Material{Vec3{0.90F, 0.58F, 0.35F}, 0.65F, 0.0F, 0.5F});
  scene.addMaterial(Material{Vec3{0.40F, 0.62F, 0.95F}, 0.45F, 0.0F, 1.0F});
  scene.addLight(Light{LightType::Directional, normalize(Vec3{-0.55F, -1.0F, -0.45F}), Vec3{}, Vec3{1.0F, 0.96F, 0.90F}, 0.65F});
  scene.addLight(Light{LightType::Point, Vec3{}, Vec3{1.35F, 1.25F, 1.0F}, Vec3{0.65F, 0.78F, 1.0F}, 0.35F});
  scene.addLight(Light{
      LightType::Area,
      Vec3{},
      Vec3{0.0F, 1.35F, -1.2F},
      Vec3{1.0F, 0.95F, 0.88F},
      1.05F,
      Vec3{1.0F, 0.0F, 0.0F},
      Vec3{0.0F, 0.0F, 1.0F},
      2.0F,
      1.6F,
  });

  Mesh plane = MeshFactory::createPlane(5.0F, 5.0F, 0);
  try {
    plane = ObjLoader::loadFromFile(resolveScenePath("assets/models/test_plane.obj"), ObjLoadOptions{0});
  } catch (const std::exception&) {
  }
  plane.setMaterialId(0);
  plane.translate(Vec3{0.0F, -0.9F, -1.6F});
  scene.addMesh(std::move(plane));

  Mesh cube = MeshFactory::createCube(0.75F, 1);
  try {
    cube = ObjLoader::loadFromFile(resolveScenePath("assets/models/test_cube.obj"), ObjLoadOptions{1});
  } catch (const std::exception&) {
  }
  cube.setMaterialId(1);
  cube.translate(Vec3{-0.65F, -0.45F, -0.9F});
  scene.addMesh(std::move(cube));

  Mesh sphere = MeshFactory::createUvSphere(0.45F, 24, 12, 2);
  sphere.translate(Vec3{0.75F, -0.35F, -1.25F});
  scene.addMesh(std::move(sphere));

  return scene;
}

} // namespace astraglyph
