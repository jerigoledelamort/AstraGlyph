#pragma once

#include "math/Aabb.hpp"
#include "math/Vec3.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

namespace astraglyph {

struct Triangle;

// SoA (Structure of Arrays) формат для cache-friendly доступа к геометрии
// Выравнивание 32 байта для AVX/SSE инструкций
struct alignas(32) TriangleSoA {
  // Позиции вершин (9 floats на треугольник)
  alignas(32) std::vector<float> v0x;
  alignas(32) std::vector<float> v0y;
  alignas(32) std::vector<float> v0z;
  alignas(32) std::vector<float> v1x;
  alignas(32) std::vector<float> v1y;
  alignas(32) std::vector<float> v1z;
  alignas(32) std::vector<float> v2x;
  alignas(32) std::vector<float> v2y;
  alignas(32) std::vector<float> v2z;
  
  // Нормали вершин (9 floats на треугольник)
  alignas(32) std::vector<float> n0x;
  alignas(32) std::vector<float> n0y;
  alignas(32) std::vector<float> n0z;
  alignas(32) std::vector<float> n1x;
  alignas(32) std::vector<float> n1y;
  alignas(32) std::vector<float> n1z;
  alignas(32) std::vector<float> n2x;
  alignas(32) std::vector<float> n2y;
  alignas(32) std::vector<float> n2z;
  
  // UV координаты (6 floats на треугольник)
  alignas(32) std::vector<float> uv0x;
  alignas(32) std::vector<float> uv0y;
  alignas(32) std::vector<float> uv1x;
  alignas(32) std::vector<float> uv1y;
  alignas(32) std::vector<float> uv2x;
  alignas(32) std::vector<float> uv2y;
  
  // Material IDs
  alignas(32) std::vector<int> materialIds;
  
  // Cached edges для быстрого доступа (6 floats на треугольник)
  alignas(32) std::vector<float> edge1x;
  alignas(32) std::vector<float> edge1y;
  alignas(32) std::vector<float> edge1z;
  alignas(32) std::vector<float> edge2x;
  alignas(32) std::vector<float> edge2y;
  alignas(32) std::vector<float> edge2z;
  
  // AABB bounds для каждого треугольника (6 floats на треугольник)
  alignas(32) std::vector<float> boundsMinX;
  alignas(32) std::vector<float> boundsMinY;
  alignas(32) std::vector<float> boundsMinZ;
  alignas(32) std::vector<float> boundsMaxX;
  alignas(32) std::vector<float> boundsMaxY;
  alignas(32) std::vector<float> boundsMaxZ;
  
  std::size_t triangleCount() const noexcept { return v0x.size(); }
  
  [[nodiscard]] bool empty() const noexcept { return v0x.empty(); }
  
  void clear() noexcept {
    v0x.clear(); v0y.clear(); v0z.clear();
    v1x.clear(); v1y.clear(); v1z.clear();
    v2x.clear(); v2y.clear(); v2z.clear();
    n0x.clear(); n0y.clear(); n0z.clear();
    n1x.clear(); n1y.clear(); n1z.clear();
    n2x.clear(); n2y.clear(); n2z.clear();
    uv0x.clear(); uv0y.clear();
    uv1x.clear(); uv1y.clear();
    uv2x.clear(); uv2y.clear();
    materialIds.clear();
    edge1x.clear(); edge1y.clear(); edge1z.clear();
    edge2x.clear(); edge2y.clear(); edge2z.clear();
    boundsMinX.clear(); boundsMinY.clear(); boundsMinZ.clear();
    boundsMaxX.clear(); boundsMaxY.clear(); boundsMaxZ.clear();
  }
  
  void reserve(std::size_t count);
  
  // Добавление треугольника из AoS формата (реализация в .cpp)
  void pushTriangle(const Triangle& tri);
  
  // Получение данных для конкретного треугольника (для fallback)
  [[nodiscard]] Vec3 v0(std::size_t index) const noexcept {
    return Vec3{v0x[index], v0y[index], v0z[index]};
  }
  [[nodiscard]] Vec3 v1(std::size_t index) const noexcept {
    return Vec3{v1x[index], v1y[index], v1z[index]};
  }
  [[nodiscard]] Vec3 v2(std::size_t index) const noexcept {
    return Vec3{v2x[index], v2y[index], v2z[index]};
  }
  
  [[nodiscard]] Vec3 edge1(std::size_t index) const noexcept {
    return Vec3{edge1x[index], edge1y[index], edge1z[index]};
  }
  [[nodiscard]] Vec3 edge2(std::size_t index) const noexcept {
    return Vec3{edge2x[index], edge2y[index], edge2z[index]};
  }
  
  [[nodiscard]] Aabb bounds(std::size_t index) const noexcept {
    return Aabb{
      Vec3{boundsMinX[index], boundsMinY[index], boundsMinZ[index]},
      Vec3{boundsMaxX[index], boundsMaxY[index], boundsMaxZ[index]}
    };
  }
  
  [[nodiscard]] int materialId(std::size_t index) const noexcept {
    return materialIds[index];
  }
};

// Бatching структура для SIMD обработки с SoA форматом
struct TriangleBatchSoA {
  // Выровненные данные для SIMD (8 треугольников)
  alignas(32) float v0x[8];
  alignas(32) float v0y[8];
  alignas(32) float v0z[8];
  alignas(32) float v1x[8];
  alignas(32) float v1y[8];
  alignas(32) float v1z[8];
  alignas(32) float v2x[8];
  alignas(32) float v2y[8];
  alignas(32) float v2z[8];
  alignas(32) float edge1x[8];
  alignas(32) float edge1y[8];
  alignas(32) float edge1z[8];
  alignas(32) float edge2x[8];
  alignas(32) float edge2y[8];
  alignas(32) float edge2z[8];
  
  int triangleCount{0};
  
  // Упаковка из TriangleSoA
  void packTriangles(const TriangleSoA& triangles, std::size_t offset, int count);
  
  // Упаковка из std::vector<Triangle> (AoS формат)
  void packTriangles(const std::vector<Triangle>& triangles, std::size_t offset, int count);
};

} // namespace astraglyph
