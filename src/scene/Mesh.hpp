#pragma once

#include "geometry/Triangle.hpp"
#include "math/Aabb.hpp"
#include "math/Vec3.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace astraglyph {

class Mesh {
public:
  explicit Mesh(int materialId = 0) noexcept;

  void addTriangle(Triangle triangle);
  void translate(Vec3 offset) noexcept;
  void setMaterialId(int materialId) noexcept;
  void setName(std::string name);
  void setGroupName(std::string groupName);
  void setSourceMaterialName(std::string materialName);
  void setMaterialLibrary(std::string materialLibrary);
  void setSourcePath(std::filesystem::path sourcePath);

  [[nodiscard]] int materialId() const noexcept;
  [[nodiscard]] std::size_t triangleCount() const noexcept;
  [[nodiscard]] const std::vector<Triangle>& triangles() const noexcept;
  [[nodiscard]] std::vector<Triangle>& triangles() noexcept;
  [[nodiscard]] const Aabb& bounds() const noexcept;
  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] const std::string& name() const noexcept;
  [[nodiscard]] const std::string& groupName() const noexcept;
  [[nodiscard]] const std::string& sourceMaterialName() const noexcept;
  [[nodiscard]] const std::string& materialLibrary() const noexcept;
  [[nodiscard]] const std::filesystem::path& sourcePath() const noexcept;

  void recomputeBounds() noexcept;

private:
  std::vector<Triangle> triangles_{};
  Aabb bounds_{};
  int materialId_{0};
  std::string name_{};
  std::string groupName_{};
  std::string sourceMaterialName_{};
  std::string materialLibrary_{};
  std::filesystem::path sourcePath_{};
};

} // namespace astraglyph
