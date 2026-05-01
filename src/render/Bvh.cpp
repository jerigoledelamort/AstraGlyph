#include "render/Bvh.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace astraglyph {

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

namespace {

constexpr int kMaxSahBins = 8;

[[nodiscard]] float axisComponent(const Vec3& value, int axis) noexcept
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

struct Splits {
  int bestAxis{-1};
  int bestSplit{-1};
  float bestCost{std::numeric_limits<float>::infinity()};
};

[[nodiscard]] Splits findBestSplit(
    int firstTriangle,
    int triangleCount,
    const Aabb& centroidBounds,
    const std::vector<Triangle>& triangles)
{
  Splits splits;

  const float epsilon = 1.0e-6F;

  for (int axis = 0; axis < 3; ++axis) {
    if (Bvh::axisComponent(centroidBounds.max, axis) - Bvh::axisComponent(centroidBounds.min, axis) <= epsilon) {
      continue;
    }

    const int binCount = std::min(kMaxSahBins, triangleCount);
    std::array<Aabb, kMaxSahBins> binBounds{};
    std::array<int, kMaxSahBins> binTriCount{};

    for (int i = 0; i < binCount; ++i) {
      binBounds[i] = Aabb{};
      binTriCount[i] = 0;
    }

    const float minVal = Bvh::axisComponent(centroidBounds.min, axis);
    const float invRange = 1.0F / std::max(Bvh::axisComponent(centroidBounds.max, axis) - minVal, epsilon);

    for (int i = 0; i < triangleCount; ++i) {
      const Vec3 centroid = triangles[static_cast<std::size_t>(firstTriangle + i)].bounds().centroid();
      const float coord = Bvh::axisComponent(centroid, axis);
      const int bin = std::min(static_cast<int>((coord - minVal) * invRange * static_cast<float>(binCount)), binCount - 1);
      binBounds[bin].expand(centroid);
      ++binTriCount[bin];
    }

    std::array<Aabb, kMaxSahBins> prefixBounds{};
    std::array<int, kMaxSahBins> prefixCount{};
    std::array<Aabb, kMaxSahBins> suffixBounds{};
    std::array<int, kMaxSahBins> suffixCount{};

    Aabb currentBounds{};
    int currentCount = 0;
    for (int i = 0; i < binCount; ++i) {
      if (binTriCount[i] > 0) {
        currentBounds.expand(binBounds[i]);
        currentCount += binTriCount[i];
      }
      prefixBounds[i] = currentBounds;
      prefixCount[i] = currentCount;
    }

    currentBounds = Aabb{};
    currentCount = 0;
    for (int i = binCount - 1; i >= 0; --i) {
      if (binTriCount[i] > 0) {
        currentBounds.expand(binBounds[i]);
        currentCount += binTriCount[i];
      }
      suffixBounds[i] = currentBounds;
      suffixCount[i] = currentCount;
    }

    for (int i = 0; i < binCount - 1; ++i) {
      if (prefixCount[i] == 0 || suffixCount[i + 1] == 0) {
        continue;
      }

      const float leftCost = static_cast<float>(prefixCount[i]) * prefixBounds[i].surfaceArea();
      const float rightCost = static_cast<float>(suffixCount[i + 1]) * suffixBounds[i + 1].surfaceArea();
      const float cost = leftCost + rightCost;

      if (cost < splits.bestCost) {
        splits.bestCost = cost;
        splits.bestAxis = axis;
        splits.bestSplit = i + 1;
      }
    }
  }

  return splits;
}

} // namespace

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

bool Bvh::intersectAny(
    const Ray& ray,
    float tMax,
    bool backfaceCulling,
    RenderMetrics* metrics) const noexcept
{
  if (nodes_.empty()) {
    return false;
  }

  return intersectAnyNode(0, ray, tMax, backfaceCulling, metrics);
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
  const Splits splits = findBestSplit(firstTriangle, triangleCount, centroidBounds, triangles_);

  if (splits.bestAxis < 0 || splits.bestSplit <= 0) {
    ++leafCount_;
    return nodeIndex;
  }

  const Vec3 centroidMin = centroidBounds.min;
  const float minVal = Bvh::axisComponent(centroidMin, splits.bestAxis);
  const float maxVal = Bvh::axisComponent(centroidBounds.max, splits.bestAxis);
  const float invRange = 1.0F / std::max(maxVal - minVal, 1.0e-6F);

  const auto getBin = [&](const Triangle& tri) {
    const Vec3 centroid = tri.bounds().centroid();
    const float coord = Bvh::axisComponent(centroid, splits.bestAxis);
    return std::min(static_cast<int>((coord - minVal) * invRange * static_cast<float>(kMaxSahBins)), kMaxSahBins - 1);
  };

  const int splitBin = splits.bestSplit;
  std::stable_partition(
      triangles_.begin() + firstTriangle,
      triangles_.begin() + firstTriangle + triangleCount,
      [&getBin, splitBin](const Triangle& tri) {
        return getBin(tri) < splitBin;
      });

  const int splitIndex = static_cast<int>(
      std::find_if(
          triangles_.begin() + firstTriangle + std::max(0, splitBin - 1),
          triangles_.begin() + firstTriangle + triangleCount,
          [&getBin, splitBin](const Triangle& tri) {
            return getBin(tri) >= splitBin;
          }) - triangles_.begin());

  const int leftCount = splitIndex - firstTriangle;
  const int rightCount = triangleCount - leftCount;

  if (leftCount == 0 || rightCount == 0) {
    ++leafCount_;
    return nodeIndex;
  }

  const int leftChild = buildNode(firstTriangle, leftCount);
  const int rightChild = buildNode(splitIndex, rightCount);

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

bool Bvh::intersectAnyNode(
    int nodeIndex,
    const Ray& ray,
    float tMax,
    bool backfaceCulling,
    RenderMetrics* metrics) const noexcept
{
  if (metrics != nullptr) {
    ++metrics->bvhNodeTests;
  }

  const BvhNode& node = nodes_[static_cast<std::size_t>(nodeIndex)];
  float nodeTMin = ray.tMin;
  float nodeTMax = tMax;

  if (metrics != nullptr) {
    ++metrics->bvhAabbTests;
  }
  if (!node.bounds.intersect(ray, nodeTMin, nodeTMax)) {
    return false;
  }

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
      if (triangle.intersectAny(ray, backfaceCulling)) {
        return true;
      }
    }

    return false;
  }

  const BvhNode& leftNode = nodes_[static_cast<std::size_t>(node.leftChild)];
  const BvhNode& rightNode = nodes_[static_cast<std::size_t>(node.rightChild)];

  float leftTMin = ray.tMin;
  float leftTMax = tMax;
  float rightTMin = ray.tMin;
  float rightTMax = tMax;
  if (metrics != nullptr) {
    metrics->bvhAabbTests += 2U;
  }

  const bool leftHit = leftNode.bounds.intersect(ray, leftTMin, leftTMax);
  const bool rightHit = rightNode.bounds.intersect(ray, rightTMin, rightTMax);

  if (leftHit && rightHit) {
    const int firstChild = leftTMin <= rightTMin ? node.leftChild : node.rightChild;
    const int secondChild = firstChild == node.leftChild ? node.rightChild : node.leftChild;
    if (intersectAnyNode(firstChild, ray, tMax, backfaceCulling, metrics)) {
      return true;
    }
    return intersectAnyNode(secondChild, ray, tMax, backfaceCulling, metrics);
  }

  if (leftHit) {
    return intersectAnyNode(node.leftChild, ray, tMax, backfaceCulling, metrics);
  }
  if (rightHit) {
    return intersectAnyNode(node.rightChild, ray, tMax, backfaceCulling, metrics);
  }

  return false;
}

} // namespace astraglyph
