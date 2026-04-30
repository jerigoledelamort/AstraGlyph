#include "scene/Mesh.hpp"

#include <utility>

namespace astraglyph {

Mesh::Mesh(int materialId) noexcept
    : materialId_{materialId}
{
}

void Mesh::addTriangle(Triangle triangle)
{
  triangle.materialId = materialId_;
  bounds_.expand(triangle.bounds());
  triangles_.push_back(std::move(triangle));
}

void Mesh::translate(Vec3 offset) noexcept
{
  for (Triangle& triangle : triangles_) {
    triangle.v0 += offset;
    triangle.v1 += offset;
    triangle.v2 += offset;
  }
  recomputeBounds();
}

void Mesh::setMaterialId(int materialId) noexcept
{
  materialId_ = materialId;
  for (Triangle& triangle : triangles_) {
    triangle.materialId = materialId_;
  }
}

void Mesh::setName(std::string name)
{
  name_ = std::move(name);
}

void Mesh::setGroupName(std::string groupName)
{
  groupName_ = std::move(groupName);
}

void Mesh::setSourceMaterialName(std::string materialName)
{
  sourceMaterialName_ = std::move(materialName);
}

void Mesh::setMaterialLibrary(std::string materialLibrary)
{
  materialLibrary_ = std::move(materialLibrary);
}

void Mesh::setSourcePath(std::filesystem::path sourcePath)
{
  sourcePath_ = std::move(sourcePath);
}

int Mesh::materialId() const noexcept
{
  return materialId_;
}

std::size_t Mesh::triangleCount() const noexcept
{
  return triangles_.size();
}

const std::vector<Triangle>& Mesh::triangles() const noexcept
{
  return triangles_;
}

std::vector<Triangle>& Mesh::triangles() noexcept
{
  return triangles_;
}

const Aabb& Mesh::bounds() const noexcept
{
  return bounds_;
}

bool Mesh::empty() const noexcept
{
  return triangles_.empty();
}

const std::string& Mesh::name() const noexcept
{
  return name_;
}

const std::string& Mesh::groupName() const noexcept
{
  return groupName_;
}

const std::string& Mesh::sourceMaterialName() const noexcept
{
  return sourceMaterialName_;
}

const std::string& Mesh::materialLibrary() const noexcept
{
  return materialLibrary_;
}

const std::filesystem::path& Mesh::sourcePath() const noexcept
{
  return sourcePath_;
}

void Mesh::recomputeBounds() noexcept
{
  bounds_ = Aabb{};
  for (const Triangle& triangle : triangles_) {
    bounds_.expand(triangle.bounds());
  }
}

} // namespace astraglyph
