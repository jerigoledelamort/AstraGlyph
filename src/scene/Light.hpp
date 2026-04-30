#pragma once

#include "math/Vec3.hpp"

namespace astraglyph {

enum class LightType {
  Directional,
  Point,
  Area,
};

struct Light {
  LightType type{LightType::Directional};
  Vec3 direction{0.0F, -1.0F, 0.0F};
  Vec3 position{};
  Vec3 color{1.0F, 1.0F, 1.0F};
  float intensity{1.0F};
  Vec3 right{1.0F, 0.0F, 0.0F};
  Vec3 up{0.0F, 0.0F, 1.0F};
  float width{1.0F};
  float height{1.0F};
};

} // namespace astraglyph
