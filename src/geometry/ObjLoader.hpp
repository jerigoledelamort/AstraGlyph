#pragma once

#include "scene/Mesh.hpp"

#include <filesystem>
#include <string_view>

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
};

} // namespace astraglyph
