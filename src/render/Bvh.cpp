#include "render/Bvh.hpp"

#include <algorithm>

namespace astraglyph {

void Bvh::setLeafSize(int leafSize) noexcept
{
  leafSize_ = std::max(1, leafSize);
}

void Bvh::build(const std::vector<Triangle>& triangles)
{
  triangles_ = triangles;
  nodes_.clear();
  leafCount_ = 0;

  if (triangles_.empty()) {
    return;
  }

  nodes_.reserve(triangles_.size() * 2U);
  buildNode(0, static_cast<int>(triangles_.size()));
}

bool Bvh::intersect(
    const Ray& ray,
    HitInfo& hitInfo,
    bool backfaceCulling,
    RenderMetrics* metrics) const noexcept
{
  hitInfo = HitInfo{};
  if (nodes_.empty()) {
    return false;
  }

  float closestT = ray.tMax;
  return intersectNode(0, ray, backfaceCulling, closestT, hitInfo, metrics);
}

std::size_t Bvh::nodeCount() const noexcept
{
  return nodes_.size();
}

std::size_t Bvh::leafCount() const noexcept
{
  return leafCount_;
}

std::size_t Bvh::triangleCount() const noexcept
{
  return triangles_.size();
}

int Bvh::buildNode(int firstTriangle, int triangleCount)
{
  const int nodeIndex = static_cast<int>(nodes_.size());
  nodes_.push_back(BvhNode{});

  BvhNode& node = nodes_.back();
  node.bounds = computeBounds(firstTriangle, triangleCount);
  node.firstTriangle = firstTriangle;
  node.triangleCount = triangleCount;

  if (triangleCount <= leafSize_) {
    ++leafCount_;
    return nodeIndex;
  }

  const Aabb centroidBounds = computeCentroidBounds(firstTriangle, triangleCount);
  const int axis = centroidBounds.longestAxis();
  const int midpoint = firstTriangle + (triangleCount / 2);

  std::nth_element(
      triangles_.begin() + firstTriangle,
      triangles_.begin() + midpoint,
      triangles_.begin() + firstTriangle + triangleCount,
      [axis](const Triangle& lhs, const Triangle& rhs) {
        return axisComponent(lhs.bounds().centroid(), axis) <
               axisComponent(rhs.bounds().centroid(), axis);
      });

  const int leftChild = buildNode(firstTriangle, midpoint - firstTriangle);
  const int rightChild = buildNode(midpoint, firstTriangle + triangleCount - midpoint);

  node = BvhNode{};
  node.bounds = computeBounds(firstTriangle, triangleCount);
  node.leftChild = leftChild;
  node.rightChild = rightChild;
  node.firstTriangle = 0;
  node.triangleCount = 0;
  return nodeIndex;
}

Aabb Bvh::computeBounds(int firstTriangle, int triangleCount) const noexcept
{
  Aabb bounds;
  for (int index = 0; index < triangleCount; ++index) {
    bounds.expand(triangles_[static_cast<std::size_t>(firstTriangle + index)].bounds());
  }
  return bounds;
}

Aabb Bvh::computeCentroidBounds(int firstTriangle, int triangleCount) const noexcept
{
  Aabb centroidBounds;
  for (int index = 0; index < triangleCount; ++index) {
    centroidBounds.expand(
        triangles_[static_cast<std::size_t>(firstTriangle + index)].bounds().centroid());
  }
  return centroidBounds;
}

bool Bvh::intersectNode(
    int nodeIndex,
    const Ray& ray,
    bool backfaceCulling,
    float& closestT,
    HitInfo& closestHit,
    RenderMetrics* metrics) const noexcept
{
  if (metrics != nullptr) {
    ++metrics->bvhNodeTests;
  }

  const BvhNode& node = nodes_[static_cast<std::size_t>(nodeIndex)];
  float nodeTMin = ray.tMin;
  float nodeTMax = closestT;
  Ray clippedRay = ray;
  clippedRay.tMax = closestT;

  if (metrics != nullptr) {
    ++metrics->bvhAabbTests;
  }
  if (!node.bounds.intersect(clippedRay, nodeTMin, nodeTMax)) {
    return false;
  }

  bool hit = false;
  if (node.isLeaf()) {
    if (metrics != nullptr) {
      ++metrics->bvhLeafTests;
    }

    for (int index = 0; index < node.triangleCount; ++index) {
      if (metrics != nullptr) {
        ++metrics->triangleTests;
      }

      const Triangle& triangle =
          triangles_[static_cast<std::size_t>(node.firstTriangle + index)];
      HitInfo candidate{};
      Ray triangleRay = ray;
      triangleRay.tMax = closestT;
      if (!triangle.intersect(triangleRay, candidate, backfaceCulling)) {
        continue;
      }

      if (!closestHit.hit || candidate.t < closestT) {
        closestT = candidate.t;
        closestHit = candidate;
        hit = true;
      }
    }

    return hit;
  }

  const BvhNode& leftNode = nodes_[static_cast<std::size_t>(node.leftChild)];
  const BvhNode& rightNode = nodes_[static_cast<std::size_t>(node.rightChild)];

  float leftTMin = ray.tMin;
  float leftTMax = closestT;
  float rightTMin = ray.tMin;
  float rightTMax = closestT;
  if (metrics != nullptr) {
    metrics->bvhAabbTests += 2U;
  }

  const bool leftHit = leftNode.bounds.intersect(clippedRay, leftTMin, leftTMax);
  const bool rightHit = rightNode.bounds.intersect(clippedRay, rightTMin, rightTMax);

  if (leftHit && rightHit) {
    const int firstChild = leftTMin <= rightTMin ? node.leftChild : node.rightChild;
    const int secondChild = firstChild == node.leftChild ? node.rightChild : node.leftChild;
    hit |= intersectNode(firstChild, ray, backfaceCulling, closestT, closestHit, metrics);
    hit |= intersectNode(secondChild, ray, backfaceCulling, closestT, closestHit, metrics);
    return hit;
  }

  if (leftHit) {
    hit |= intersectNode(node.leftChild, ray, backfaceCulling, closestT, closestHit, metrics);
  }
  if (rightHit) {
    hit |= intersectNode(node.rightChild, ray, backfaceCulling, closestT, closestHit, metrics);
  }

  return hit;
}

float Bvh::axisComponent(const Vec3& value, int axis) noexcept
{
  switch (axis) {
    case 1:
      return value.y;
    case 2:
      return value.z;
    default:
      return value.x;
  }
}

} // namespace astraglyph
