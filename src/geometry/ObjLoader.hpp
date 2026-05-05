#pragma once

#include "scene/Mesh.hpp"
#include "scene/Material.hpp"

#include <filesystem>
#include <string_view>
#include <vector>

namespace astraglyph {

struct ObjLoadOptions {
  int materialId{0};
};

class ObjLoader {
public:
  [[nodiscard]] static Mesh loadFromFile(
      const std::filesystem::path& path,
      const ObjLoadOptions& options = {});
  [[nodiscard]] static Mesh loadFromString(
      std::string_view source,
      const ObjLoadOptions& options = {},
      const std::filesystem::path& sourcePath = {});

  // Load materials from MTL file (with optional texture loading)
  [[nodiscard]] static std::vector<Material> loadMaterials(
      const std::filesystem::path& mtlPath,
      bool loadTextures = true);
};

} // namespace astraglyph
