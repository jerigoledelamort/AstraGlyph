#pragma once

#include "geometry/Triangle.hpp"
#include "math/Aabb.hpp"
#include "math/Ray.hpp"
#include "render/RenderMetrics.hpp"

#include <cstddef>
#include <vector>

namespace astraglyph {

struct BvhNode {
  Aabb bounds{};
  int leftChild{-1};
  int rightChild{-1};
  int firstTriangle{0};
  int triangleCount{0};

  [[nodiscard]] bool isLeaf() const noexcept
  {
    return leftChild < 0 && rightChild < 0;
  }
};

class Bvh {
public:
  void setLeafSize(int leafSize) noexcept;
  void build(const std::vector<Triangle>& triangles);
  [[nodiscard]] bool intersect(
      const Ray& ray,
      HitInfo& hitInfo,
      bool backfaceCulling,
      RenderMetrics* metrics = nullptr) const noexcept;
  [[nodiscard]] std::size_t nodeCount() const noexcept;
  [[nodiscard]] std::size_t leafCount() const noexcept;
  [[nodiscard]] std::size_t triangleCount() const noexcept;

private:
  int buildNode(int firstTriangle, int triangleCount);
  [[nodiscard]] Aabb computeBounds(int firstTriangle, int triangleCount) const noexcept;
  [[nodiscard]] Aabb computeCentroidBounds(int firstTriangle, int triangleCount) const noexcept;
  [[nodiscard]] bool intersectNode(
      int nodeIndex,
      const Ray& ray,
      bool backfaceCulling,
      float& closestT,
      HitInfo& closestHit,
      RenderMetrics* metrics) const noexcept;
  [[nodiscard]] static float axisComponent(const Vec3& value, int axis) noexcept;

  std::vector<BvhNode> nodes_{};
  std::vector<Triangle> triangles_{};
  int leafSize_{4};
  std::size_t leafCount_{0};
};

} // namespace astraglyph
