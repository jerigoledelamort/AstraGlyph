#pragma once

#include "scene/Material.hpp"
#include "scene/Mesh.hpp"
#include "scene/Transform.hpp"

namespace astraglyph {

struct Entity {
  Transform transform{};
  Mesh mesh{};
  Material material{};
};

} // namespace astraglyph
