#pragma once

#include "geometry/Triangle.hpp"
#include "geometry/TriangleSoA.hpp"
#include "math/Aabb.hpp"
#include "math/Ray.hpp"
#include "render/RenderMetrics.hpp"

#include <cstddef>
#include <vector>

// Forward declaration для RayPacket
namespace astraglyph {
struct RayPacket;
struct RayPacketHitInfo;
}

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
  [[nodiscard]] bool intersectAny(
      const Ray& ray,
      float tMax,
      bool backfaceCulling,
      RenderMetrics* metrics = nullptr) const noexcept;
  
  // Packet tracing - пересечение пакета лучей
  [[nodiscard]] bool intersectPacket(
      const RayPacket& packet,
      bool backfaceCulling,
      RenderMetrics* metrics = nullptr) const noexcept;

  [[nodiscard]] std::size_t nodeCount() const noexcept;
  [[nodiscard]] std::size_t leafCount() const noexcept;
  [[nodiscard]] std::size_t triangleCount() const noexcept;

  [[nodiscard]] static float axisComponent(const Vec3& value, int axis) noexcept;

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
  [[nodiscard]] bool intersectNodeChecked(
      int nodeIndex,
      const Ray& ray,
      bool backfaceCulling,
      float& closestT,
      HitInfo& closestHit,
      RenderMetrics* metrics) const noexcept;
  [[nodiscard]] bool intersectAnyNode(
      int nodeIndex,
      const Ray& ray,
      float tMax,
      bool backfaceCulling,
      RenderMetrics* metrics) const noexcept;
  [[nodiscard]] bool intersectAnyNodeChecked(
      int nodeIndex,
      const Ray& ray,
      float tMax,
      bool backfaceCulling,
      RenderMetrics* metrics) const noexcept;

  [[nodiscard]] bool intersectPacketNode(
      int nodeIndex,
      const RayPacket& packet,
      bool backfaceCulling,
      RenderMetrics* metrics) const noexcept;

  std::vector<BvhNode> nodes_{};
  std::vector<Triangle> triangles_{};
  TriangleSoA trianglesSoA_{}; // SoA формат для cache-friendly доступа
  int leafSize_{4};
  std::size_t leafCount_{0};
  bool soaReady_{false}; // Флаг готовности SoA формата
};

} // namespace astraglyph
