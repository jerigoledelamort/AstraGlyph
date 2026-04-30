#pragma once

#include "math/Vec3.hpp"

namespace astraglyph {

struct Material {
  Vec3 albedo{1.0F, 1.0F, 1.0F};
  float roughness{0.5F};
  float metallic{0.0F};
  float reflectivity{0.0F};
  Vec3 emissionColor{0.0F, 0.0F, 0.0F};
  float emissionStrength{0.0F};
};

} // namespace astraglyph
