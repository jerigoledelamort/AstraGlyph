#pragma once

#include "math/Vec3.hpp"

namespace astraglyph {

struct Transform {
  Vec3 translation{};
  Vec3 rotation{};
  Vec3 scale{1.0F, 1.0F, 1.0F};
};

} // namespace astraglyph
