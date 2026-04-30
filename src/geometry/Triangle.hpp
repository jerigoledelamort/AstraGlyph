#pragma once

#include "math/Aabb.hpp"
#include "math/Ray.hpp"
#include "math/Vec2.hpp"
#include "math/Vec3.hpp"

#include <limits>

namespace astraglyph {

struct HitInfo {
  bool hit{false};
  float t{std::numeric_limits<float>::infinity()};
  Vec3 position{};
  Vec3 normal{};
  Vec3 barycentric{};
  int materialId{-1};
};

struct Triangle {
  Vec3 v0{};
  Vec3 v1{};
  Vec3 v2{};
  Vec3 n0{};
  Vec3 n1{};
  Vec3 n2{};
  Vec2 uv0{};
  Vec2 uv1{};
  Vec2 uv2{};
  int materialId{0};

  [[nodiscard]] Aabb bounds() const noexcept;
  [[nodiscard]] Vec3 faceNormal() const noexcept;
  [[nodiscard]] bool intersect(const Ray& ray, HitInfo& hitInfo, bool backfaceCulling) const noexcept;
  [[nodiscard]] bool intersect(const Ray& ray, float& t) const noexcept;
};

} // namespace astraglyph
