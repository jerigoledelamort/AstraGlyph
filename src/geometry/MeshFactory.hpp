#pragma once

#include "math/Vec3.hpp"
#include "scene/Mesh.hpp"

namespace astraglyph {

class MeshFactory {
public:
  [[nodiscard]] static Mesh makeTriangle(Vec3 v0, Vec3 v1, Vec3 v2, int materialId = 0);
  [[nodiscard]] static Mesh createPlane(float width, float depth, int materialId = 0);
  [[nodiscard]] static Mesh createCube(float size, int materialId = 0);
  [[nodiscard]] static Mesh createCube(Vec3 dimensions, int materialId = 0);
  [[nodiscard]] static Mesh createUvSphere(float radius, int segments, int rings, int materialId = 0);
};

} // namespace astraglyph
