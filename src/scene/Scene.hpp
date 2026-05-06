#pragma once

#include "scene/Entity.hpp"
#include "scene/Light.hpp"
#include "scene/Material.hpp"
#include "scene/Mesh.hpp"

#include <filesystem>
#include <vector>

namespace astraglyph {

class Scene {
public:
  void addEntity(Entity entity);
  void addMesh(Mesh mesh);
  void addMeshFromObj(const std::filesystem::path& path, int materialId = 0);
  void addMaterial(Material material);
  void addLight(Light light);

  [[nodiscard]] const std::vector<Entity>& entities() const noexcept;
  [[nodiscard]] const std::vector<Mesh>& meshes() const noexcept;
  [[nodiscard]] const std::vector<Material>& materials() const noexcept;
  [[nodiscard]] std::size_t getMaterialCount() const noexcept;
  [[nodiscard]] const std::vector<Light>& lights() const noexcept;
  [[nodiscard]] const std::vector<Triangle>& triangles() const noexcept;
  [[nodiscard]] const Material& materialFor(int materialId) const noexcept;
  [[nodiscard]] std::vector<Triangle> flattenedTriangles() const;

  [[nodiscard]] static Scene createDefaultScene();

  [[nodiscard]] std::size_t version() const noexcept { return version_; }

private:
  std::vector<Entity> entities_{};
  std::vector<Mesh> meshes_{};
  std::vector<Material> materials_{};
  std::vector<Light> lights_{};
  std::vector<Triangle> triangles_{};
  std::size_t version_{0};
};

} // namespace astraglyph
