#include "scene/Mesh.hpp"

#include <cmath>
#include <utility>

namespace astraglyph {
namespace {

[[nodiscard]] Vec3 rotatePoint(const Vec3& p, float yaw, float pitch, float roll) noexcept
{
  const float cy = std::cos(yaw);
  const float sy = std::sin(yaw);
  const float cp = std::cos(pitch);
  const float sp = std::sin(pitch);
  const float cr = std::cos(roll);
  const float sr = std::sin(roll);

  const float x1 = cy * p.x + sy * p.z;
  const float z1 = -sy * p.x + cy * p.z;
  const float y1 = p.y;

  const float x2 = x1;
  const float y2 = cp * y1 - sp * z1;
  const float z2 = sp * y1 + cp * z1;

  const float x3 = cr * x2 - sr * y2;
  const float y3 = sr * x2 + cr * y2;
  const float z3 = z2;

  return Vec3{x3, y3, z3};
}

} // namespace

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
    triangle.computeCachedEdges();
  }
  recomputeBounds();
}

void Mesh::scale(Vec3 factor) noexcept
{
  for (Triangle& triangle : triangles_) {
    triangle.v0.x *= factor.x; triangle.v0.y *= factor.y; triangle.v0.z *= factor.z;
    triangle.v1.x *= factor.x; triangle.v1.y *= factor.y; triangle.v1.z *= factor.z;
    triangle.v2.x *= factor.x; triangle.v2.y *= factor.y; triangle.v2.z *= factor.z;
    triangle.computeCachedEdges();
  }
  recomputeBounds();
}

void Mesh::rotate(float yaw, float pitch, float roll) noexcept
{
  for (Triangle& triangle : triangles_) {
    triangle.v0 = rotatePoint(triangle.v0, yaw, pitch, roll);
    triangle.v1 = rotatePoint(triangle.v1, yaw, pitch, roll);
    triangle.v2 = rotatePoint(triangle.v2, yaw, pitch, roll);
    triangle.n0 = normalize(rotatePoint(triangle.n0, yaw, pitch, roll));
    triangle.n1 = normalize(rotatePoint(triangle.n1, yaw, pitch, roll));
    triangle.n2 = normalize(rotatePoint(triangle.n2, yaw, pitch, roll));
    triangle.computeCachedEdges();
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
