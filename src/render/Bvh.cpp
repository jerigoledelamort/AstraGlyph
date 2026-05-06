#include "render/Bvh.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <limits>

#include "render/RayPacket.hpp"
#include "render/SimdTriangleSoA.hpp"

#ifdef ASTRAGLYPH_ENABLE_SIMD
#include "render/SimdTriangle.hpp"
#endif

#if defined(_MSC_VER)
#include <intrin.h>
#define popcount __popcnt
#else
#define popcount __builtin_popcount
#endif

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
constexpr int kMinPrimitivesForSah = 4;

struct Splits {
  int bestAxis{-1};
  int bestSplit{-1};
  float bestCost{std::numeric_limits<float>::infinity()};
};

// Fallback: median split - сортировка по центру и разделение посередине
[[nodiscard]] Splits medianSplit(
    int /* firstTriangle */,
    int triangleCount,
    const Aabb& centroidBounds,
    const std::vector<Triangle>& /* triangles */)
{
  Splits splits;

  const float epsilon = 1.0e-6F;

  // Выбираем ось с наибольшей протяжённостью центроидов
  int bestAxis = 0;
  float bestExtent = Bvh::axisComponent(centroidBounds.max, 0) - Bvh::axisComponent(centroidBounds.min, 0);
  for (int axis = 1; axis < 3; ++axis) {
    const float extent = Bvh::axisComponent(centroidBounds.max, axis) - Bvh::axisComponent(centroidBounds.min, axis);
    if (extent > bestExtent) {
      bestExtent = extent;
      bestAxis = axis;
    }
  }

  if (bestExtent <= epsilon) {
    return splits; // Невозможно разделить
  }

  splits.bestAxis = bestAxis;
  splits.bestSplit = triangleCount / 2; // Разделение посередине
  splits.bestCost = std::numeric_limits<float>::infinity(); // Cost не важен для median

  return splits;
}

[[nodiscard]] Splits findBestSplit(
    int firstTriangle,
    int triangleCount,
    const Aabb& centroidBounds,
    const std::vector<Triangle>& triangles)
{
  // Fallback на median split для малого количества примитивов
  if (triangleCount < kMinPrimitivesForSah) {
    return medianSplit(firstTriangle, triangleCount, centroidBounds, triangles);
  }

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

    // Распределяем треугольники по бинам на основе центроида
    // Но в бинах храним полные AABB для правильного SAH cost
    for (int i = 0; i < triangleCount; ++i) {
      const Vec3 centroid = triangles[static_cast<std::size_t>(firstTriangle + i)].bounds().centroid();
      const float coord = Bvh::axisComponent(centroid, axis);
      const int bin = std::min(static_cast<int>((coord - minVal) * invRange * static_cast<float>(binCount)), binCount - 1);
      binBounds[bin].expand(triangles[static_cast<std::size_t>(firstTriangle + i)].bounds()); // Full AABB, не только centroid
      ++binTriCount[bin];
    }

    // Prefix bounds и counts для левого поддерева
    std::array<Aabb, kMaxSahBins> prefixBounds{};
    std::array<int, kMaxSahBins> prefixCount{};

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

    // Suffix bounds и counts для правого поддерева
    std::array<Aabb, kMaxSahBins> suffixBounds{};
    std::array<int, kMaxSahBins> suffixCount{};

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

    // Проходим по возможным split позициям между бинами
    for (int i = 0; i < binCount - 1; ++i) {
      if (prefixCount[i] == 0 || suffixCount[i + 1] == 0) {
        continue;
      }

      // SAH cost = leftCount * leftArea + rightCount * rightArea
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

[[nodiscard]] double elapsedMs(
    std::chrono::high_resolution_clock::time_point start,
    std::chrono::high_resolution_clock::time_point end) noexcept
{
  return std::chrono::duration<double, std::milli>(end - start).count();
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
  trianglesSoA_.clear();
  soaReady_ = false;

  if (triangles_.empty()) {
    return;
  }

  // Build SoA формат для cache-friendly доступа
  trianglesSoA_.reserve(triangles_.size());
  for (const auto& tri : triangles_) {
    trianglesSoA_.pushTriangle(tri);
  }
  soaReady_ = true;

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

bool Bvh::intersectPacket(
    const RayPacket& packet,
    bool backfaceCulling,
    RenderMetrics* metrics) const noexcept
{
  if (nodes_.empty() || !packet.hasActive()) {
    return false;
  }

  return intersectPacketNode(0, packet, backfaceCulling, metrics);
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

  return intersectNodeChecked(nodeIndex, ray, backfaceCulling, closestT, closestHit, metrics);
}

bool Bvh::intersectNodeChecked(
    int nodeIndex,
    const Ray& ray,
    bool backfaceCulling,
    float& closestT,
    HitInfo& closestHit,
    RenderMetrics* metrics) const noexcept
{
  const BvhNode& node = nodes_[static_cast<std::size_t>(nodeIndex)];
  bool hit = false;
  if (node.isLeaf()) {
    if (metrics != nullptr) {
      ++metrics->bvhLeafTests;
    }

#ifdef ASTRAGLYPH_ENABLE_SIMD
    // SIMD-оптимизированный путь для batch intersection
    if (node.triangleCount >= 4) {
      // Используем SIMD для обработки нескольких треугольников
      TriangleBatch batch;
      batch.packTriangles(triangles_, node.firstTriangle, node.triangleCount);
      
      alignas(32) float tArr[8];
      alignas(32) float uArr[8];
      alignas(32) float vArr[8];
      
      const int hitMask = intersectBatchSimd(batch, ray, backfaceCulling, tArr, uArr, vArr);
      
      // Обрабатываем попадания
      for (int i = 0; i < batch.triangleCount; ++i) {
        if (hitMask & (1 << i)) {
          if (metrics != nullptr) {
            ++metrics->triangleTests;
          }
          
          const float t = tArr[i];
          if (t < closestT) {
            closestT = t;
            // Полное заполнение HitInfo для лучшего попадания
            const Triangle& triangle = triangles_[static_cast<std::size_t>(node.firstTriangle + i)];
            HitInfo candidate{};
            Ray triangleRay = ray;
            triangleRay.tMax = closestT;
            if (triangle.intersect(triangleRay, candidate, backfaceCulling)) {
              closestHit = candidate;
              hit = true;
            }
          }
        }
      }

      if (hit) {
        return hit;
      }
    }
#endif

    // Scalar fallback или мелкие leaf nodes
    for (int index = 0; index < node.triangleCount; ++index) {
      if (metrics != nullptr) {
        ++metrics->triangleTests;
      }

      const Triangle& triangle =
          triangles_[static_cast<std::size_t>(node.firstTriangle + index)];
      HitInfo candidate{};
      Ray triangleRay = ray;
      triangleRay.tMax = closestT;
      bool triangleHit = false;
      if (metrics != nullptr && metrics->renderProfilingEnabled) {
        const auto start = std::chrono::high_resolution_clock::now();
        triangleHit = triangle.intersect(triangleRay, candidate, backfaceCulling);
        metrics->triangleIntersectionMs += elapsedMs(start, std::chrono::high_resolution_clock::now());
      } else {
        triangleHit = triangle.intersect(triangleRay, candidate, backfaceCulling);
      }
      if (!triangleHit) {
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
  Ray clippedRay = ray;
  clippedRay.tMax = closestT;
  if (metrics != nullptr) {
    metrics->bvhAabbTests += 2U;
  }

  const bool leftHit = leftNode.bounds.intersect(clippedRay, leftTMin, leftTMax);
  const bool rightHit = rightNode.bounds.intersect(clippedRay, rightTMin, rightTMax);
  const auto visitChild = [&](int childIndex) {
    if (metrics != nullptr) {
      ++metrics->bvhNodeTests;
    }
    return intersectNodeChecked(childIndex, ray, backfaceCulling, closestT, closestHit, metrics);
  };

  if (leftHit && rightHit) {
    const bool leftFirst = leftTMin <= rightTMin;
    const int firstChild = leftFirst ? node.leftChild : node.rightChild;
    const int secondChild = leftFirst ? node.rightChild : node.leftChild;
    const float secondTMin = leftFirst ? rightTMin : leftTMin;

    hit |= visitChild(firstChild);
    if (secondTMin < closestT) {
      hit |= visitChild(secondChild);
    }
    return hit;
  }
  
  if (leftHit) {
    hit |= visitChild(node.leftChild);
  }
  if (rightHit) {
    hit |= visitChild(node.rightChild);
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

  return intersectAnyNodeChecked(nodeIndex, ray, tMax, backfaceCulling, metrics);
}

bool Bvh::intersectAnyNodeChecked(
    int nodeIndex,
    const Ray& ray,
    float tMax,
    bool backfaceCulling,
    RenderMetrics* metrics) const noexcept
{
  const BvhNode& node = nodes_[static_cast<std::size_t>(nodeIndex)];
  if (node.isLeaf()) {
    if (metrics != nullptr) {
      ++metrics->bvhLeafTests;
    }

#ifdef ASTRAGLYPH_ENABLE_SIMD
    // SIMD-оптимизированный путь для batch intersection с использованием SoA
    if (node.triangleCount >= 4) {
      TriangleBatchSoA batch;
      if (soaReady_) {
        batch.packTriangles(trianglesSoA_, static_cast<std::size_t>(node.firstTriangle), node.triangleCount);
      } else {
        batch.packTriangles(triangles_, static_cast<std::size_t>(node.firstTriangle), node.triangleCount);
      }
      
      const int hitMask = intersectBatchSimd(batch, ray, backfaceCulling);
      
      if (hitMask != 0) {
        if (metrics != nullptr) {
          metrics->triangleTests += static_cast<std::size_t>(popcount(hitMask));
        }
        return true;
      }
      
      if (metrics != nullptr) {
        metrics->triangleTests += static_cast<std::size_t>(batch.triangleCount);
      }
    }
#endif
    
    // Scalar fallback
    for (int index = 0; index < node.triangleCount; ++index) {
      if (metrics != nullptr) {
        ++metrics->triangleTests;
      }
      
      const Triangle& triangle =
          triangles_[static_cast<std::size_t>(node.firstTriangle + index)];
      bool triangleHit = false;
      if (metrics != nullptr && metrics->renderProfilingEnabled) {
        const auto start = std::chrono::high_resolution_clock::now();
        triangleHit = triangle.intersectAny(ray, backfaceCulling);
        metrics->triangleIntersectionMs += elapsedMs(start, std::chrono::high_resolution_clock::now());
      } else {
        triangleHit = triangle.intersectAny(ray, backfaceCulling);
      }
      if (triangleHit) {
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
  const auto visitChild = [&](int childIndex) {
    if (metrics != nullptr) {
      ++metrics->bvhNodeTests;
    }
    return intersectAnyNodeChecked(childIndex, ray, tMax, backfaceCulling, metrics);
  };
  
  if (leftHit && rightHit) {
    const int firstChild = leftTMin <= rightTMin ? node.leftChild : node.rightChild;
    const int secondChild = firstChild == node.leftChild ? node.rightChild : node.leftChild;
    if (visitChild(firstChild)) {
      return true;
    }
    return visitChild(secondChild);
  }
  
  if (leftHit) {
    return visitChild(node.leftChild);
  }
  if (rightHit) {
    return visitChild(node.rightChild);
  }
  
  return false;
}

bool Bvh::intersectPacketNode(
    int nodeIndex,
    const RayPacket& packet,
    bool backfaceCulling,
    RenderMetrics* metrics) const noexcept
{
  if (metrics != nullptr) {
    ++metrics->bvhNodeTests;
  }

  const BvhNode& node = nodes_[static_cast<std::size_t>(nodeIndex)];
  
  // AABB intersection for all rays in packet
  // Для простоты используем scalar fallback с проверкой каждого луча
  int activeMask = packet.activeMask;
  
  // Проверяем bounds для каждого луча в пакете
  for (int i = 0; i < packet.count; ++i) {
    if (!(activeMask & (1 << i))) {
      continue;
    }
    
    Ray ray{
      Vec3{packet.originsX[i], packet.originsY[i], packet.originsZ[i]},
      Vec3{packet.directionsX[i], packet.directionsY[i], packet.directionsZ[i]},
      packet.tMins[i],
      packet.tMaxs[i]
    };
    
    float nodeTMin = ray.tMin;
    float nodeTMax = ray.tMax;
    
    if (metrics != nullptr) {
      ++metrics->bvhAabbTests;
    }
    
    if (!node.bounds.intersect(ray, nodeTMin, nodeTMax)) {
      // Луч промахнулся, отключаем его
      activeMask &= ~(1 << i);
    }
  }
  
  // Если все лучи промахнулись
  if (activeMask == 0) {
    return false;
  }
  
  // Обновляем активную маску для дочерних узлов
  RayPacket activePacket = packet;
  activePacket.activeMask = activeMask;
  
  if (node.isLeaf()) {
    if (metrics != nullptr) {
      ++metrics->bvhLeafTests;
    }
    
    // Обработка leaf node для активного пакета лучей
#ifdef ASTRAGLYPH_ENABLE_SIMD
    // SIMD packet intersection если есть поддержка
    if (node.triangleCount >= 4 && activePacket.count >= 4) {
      TriangleBatch batch;
      batch.packTriangles(triangles_, node.firstTriangle, node.triangleCount);
      
      RayPacketHitInfo hitInfo{};
      const int hitMask = intersectPacketBatchSimd(activePacket, batch, backfaceCulling, &hitInfo);
      
      if (hitMask != 0) {
        if (metrics != nullptr) {
          metrics->triangleTests += static_cast<std::size_t>(popcount(hitMask));
        }
        return true;
      }
      
      if (metrics != nullptr) {
        metrics->triangleTests += static_cast<std::size_t>(activePacket.count * batch.triangleCount);
      }
    }
#endif
    
    // Scalar fallback для каждого активного луча
    for (int i = 0; i < activePacket.count; ++i) {
      if (!(activeMask & (1 << i))) {
        continue;
      }
      
      if (metrics != nullptr) {
        ++metrics->triangleTests;
      }
      
      Ray ray{
        Vec3{activePacket.originsX[i], activePacket.originsY[i], activePacket.originsZ[i]},
        Vec3{activePacket.directionsX[i], activePacket.directionsY[i], activePacket.directionsZ[i]},
        activePacket.tMins[i],
        activePacket.tMaxs[i]
      };
      
      bool triangleHit = false;
      for (int index = 0; index < node.triangleCount; ++index) {
        const Triangle& triangle = triangles_[static_cast<std::size_t>(node.firstTriangle + index)];
        triangleHit = triangle.intersectAny(ray, backfaceCulling);
        if (triangleHit) {
          break;
        }
      }
      
      if (triangleHit) {
        return true;
      }
    }
    
    return false;
  }
  
  const BvhNode& leftNode = nodes_[static_cast<std::size_t>(node.leftChild)];
  const BvhNode& rightNode = nodes_[static_cast<std::size_t>(node.rightChild)];
  
  // Проверяем левый и правый узлы
  bool leftHit = false;
  bool rightHit = false;
  
  for (int i = 0; i < packet.count; ++i) {
    if (!(activeMask & (1 << i))) {
      continue;
    }
    
    Ray ray{
      Vec3{packet.originsX[i], packet.originsY[i], packet.originsZ[i]},
      Vec3{packet.directionsX[i], packet.directionsY[i], packet.directionsZ[i]},
      packet.tMins[i],
      packet.tMaxs[i]
    };
    
    float leftTMin = ray.tMin;
    float leftTMax = ray.tMax;
    float rightTMin = ray.tMin;
    float rightTMax = ray.tMax;
    
    if (!leftHit && leftNode.bounds.intersect(ray, leftTMin, leftTMax)) {
      leftHit = true;
    }
    if (!rightHit && rightNode.bounds.intersect(ray, rightTMin, rightTMax)) {
      rightHit = true;
    }
    
    if (leftHit && rightHit) {
      break;
    }
  }
  
  const auto visitChild = [&](int childIndex) {
    if (metrics != nullptr) {
      ++metrics->bvhNodeTests;
    }
    return intersectPacketNode(childIndex, activePacket, backfaceCulling, metrics);
  };
  
  if (leftHit && rightHit) {
    if (visitChild(node.leftChild)) {
      return true;
    }
    return visitChild(node.rightChild);
  }
  
  if (leftHit) {
    return visitChild(node.leftChild);
  }
  if (rightHit) {
    return visitChild(node.rightChild);
  }
  
  return false;
}

} // namespace astraglyph
