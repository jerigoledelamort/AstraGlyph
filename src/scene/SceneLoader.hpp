#pragma once

#include "render/RenderSettings.hpp"
#include "scene/Camera.hpp"
#include "scene/Scene.hpp"

#include <filesystem>
#include <string>

namespace astraglyph {

struct SceneLoadResult {
  Scene scene;
  Camera camera;
  RenderSettings renderSettings;
  bool success = false;
  std::string errorMessage;
};

class SceneLoader {
public:
  [[nodiscard]] static SceneLoadResult loadFromFile(const std::filesystem::path& path);
  [[nodiscard]] static SceneLoadResult loadDefaultScene();

private:
  [[nodiscard]] static std::filesystem::path resolveAssetPath(const std::filesystem::path& path);
};

} // namespace astraglyph
