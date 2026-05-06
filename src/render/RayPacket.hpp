#pragma once

#include "math/Ray.hpp"
#include "math/Vec3.hpp"

#include <array>
#include <cstddef>
#include <limits>
#include <vector>

namespace astraglyph {

// Конфигурация размера пакета лучей
// 4 для SSE, 8 для AVX2
constexpr int kRayPacketSize = 8;

// Пакет лучей для SIMD-трассировки
struct RayPacket {
  alignas(32) float originsX[kRayPacketSize];
  alignas(32) float originsY[kRayPacketSize];
  alignas(32) float originsZ[kRayPacketSize];
  alignas(32) float directionsX[kRayPacketSize];
  alignas(32) float directionsY[kRayPacketSize];
  alignas(32) float directionsZ[kRayPacketSize];
  alignas(32) float tMins[kRayPacketSize];
  alignas(32) float tMaxs[kRayPacketSize];
  
  int count{0};
  int activeMask{0}; // Битовая маска активных лучей
  
  // Проверка, все ли лучи в пакете идут в одном направлении (coherent)
  [[nodiscard]] bool isCoherent(float threshold = 0.95F) const noexcept;
  
  // Получить количество активных лучей
  [[nodiscard]] int activeCount() const noexcept;
  
  // Проверить, есть ли активные лучи
  [[nodiscard]] bool hasActive() const noexcept;
};

// Результат пересечения пакета лучей с треугольниками
struct RayPacketHitInfo {
  alignas(32) float ts[kRayPacketSize];
  alignas(32) float us[kRayPacketSize];
  alignas(32) float vs[kRayPacketSize];
  int hitMask{0};
};

// Упаковка лучей в пакет
inline void packRays(RayPacket& packet, const std::vector<Ray>& rays, int offset, int count)
{
  packet.count = std::min(count, kRayPacketSize);
  packet.activeMask = (1 << packet.count) - 1;
  
  for (int i = 0; i < kRayPacketSize; ++i) {
    if (i < packet.count) {
      const Ray& ray = rays[static_cast<std::size_t>(offset + i)];
      packet.originsX[i] = ray.origin.x;
      packet.originsY[i] = ray.origin.y;
      packet.originsZ[i] = ray.origin.z;
      packet.directionsX[i] = ray.direction.x;
      packet.directionsY[i] = ray.direction.y;
      packet.directionsZ[i] = ray.direction.z;
      packet.tMins[i] = ray.tMin;
      packet.tMaxs[i] = ray.tMax;
    } else {
      // Padding
      packet.originsX[i] = 0.0F;
      packet.originsY[i] = 0.0F;
      packet.originsZ[i] = 0.0F;
      packet.directionsX[i] = 0.0F;
      packet.directionsY[i] = 0.0F;
      packet.directionsZ[i] = 0.0F;
      packet.tMins[i] = 0.0F;
      packet.tMaxs[i] = std::numeric_limits<float>::infinity();
    }
  }
}

// Упаковка одного луча в пакет (для fallback)
inline void packSingleRay(RayPacket& packet, const Ray& ray)
{
  packet.count = 1;
  packet.activeMask = 1;
  packet.originsX[0] = ray.origin.x;
  packet.originsY[0] = ray.origin.y;
  packet.originsZ[0] = ray.origin.z;
  packet.directionsX[0] = ray.direction.x;
  packet.directionsY[0] = ray.direction.y;
  packet.directionsZ[0] = ray.direction.z;
  packet.tMins[0] = ray.tMin;
  packet.tMaxs[0] = ray.tMax;
  
  for (int i = 1; i < kRayPacketSize; ++i) {
    packet.originsX[i] = 0.0F;
    packet.originsY[i] = 0.0F;
    packet.originsZ[i] = 0.0F;
    packet.directionsX[i] = 0.0F;
    packet.directionsY[i] = 0.0F;
    packet.directionsZ[i] = 0.0F;
    packet.tMins[i] = 0.0F;
    packet.tMaxs[i] = std::numeric_limits<float>::infinity();
  }
}

// Распаковка результата для конкретного луча
inline HitInfo unpackHitInfo(const RayPacketHitInfo& result, int rayIndex, const Ray& ray)
{
  HitInfo info{};
  if (rayIndex >= result.hitMask) {
    return info;
  }
  
  if (!(result.hitMask & (1 << rayIndex))) {
    return info;
  }
  
  info.hit = true;
  info.t = result.ts[rayIndex];
  info.barycentric = Vec3{result.us[rayIndex], result.vs[rayIndex], 1.0F - result.us[rayIndex] - result.vs[rayIndex]};
  info.position = ray.at(info.t);
  
  return info;
}

inline bool RayPacket::isCoherent(float threshold) const noexcept
{
  if (count <= 1) {
    return true;
  }
  
  // Вычисляем среднее направление
  float avgX = 0.0F, avgY = 0.0F, avgZ = 0.0F;
  int active = 0;
  
  for (int i = 0; i < count; ++i) {
    if (activeMask & (1 << i)) {
      avgX += directionsX[i];
      avgY += directionsY[i];
      avgZ += directionsZ[i];
      ++active;
    }
  }
  
  if (active == 0) {
    return true;
  }
  
  avgX /= static_cast<float>(active);
  avgY /= static_cast<float>(active);
  avgZ /= static_cast<float>(active);
  
  const float len = std::sqrt(avgX * avgX + avgY * avgY + avgZ * avgZ);
  if (len < 1.0e-6F) {
    return true;
  }
  
  avgX /= len;
  avgY /= len;
  avgZ /= len;
  
  // Проверяем, насколько каждое направление совпадает со средним
  for (int i = 0; i < count; ++i) {
    if (activeMask & (1 << i)) {
      const float dot = directionsX[i] * avgX + directionsY[i] * avgY + directionsZ[i] * avgZ;
      if (dot < threshold) {
        return false;
      }
    }
  }
  
  return true;
}

inline int RayPacket::activeCount() const noexcept
{
  int count = 0;
  for (int i = 0; i < kRayPacketSize; ++i) {
    if (activeMask & (1 << i)) {
      ++count;
    }
  }
  return count;
}

inline bool RayPacket::hasActive() const noexcept
{
  return activeMask != 0;
}

} // namespace astraglyph
