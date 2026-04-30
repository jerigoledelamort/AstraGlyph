#pragma once

#include "platform/Input.hpp"
#include "platform/Time.hpp"
#include "platform/Window.hpp"
#include "render/AsciiFramebuffer.hpp"
#include "render/Renderer.hpp"
#include "render/RenderSettings.hpp"
#include "scene/Camera.hpp"
#include "ui/DebugOverlay.hpp"
#include "ui/SettingsPanel.hpp"

#include <string>
#include <vector>

namespace astraglyph {

class Application {
public:
  Application();
  int run();

private:
  void update(double dt);
  void render();
  void handleRuntimeSettingsInput();
  void updateWindowTitle();
  void renderOverlayText(
      const std::vector<std::string>& lines,
      int originX,
      int originY,
      const Vec3& textColor,
      const Vec3& backgroundColor);

  Window window_;
  Input input_{};
  Time time_{}; 
  Camera camera_{};
  Renderer renderer_{};
  RenderSettings renderSettings_{};
  DebugOverlay debugOverlay_{};
  SettingsPanel settingsPanel_{};
  AsciiFramebuffer framebuffer_{};
  std::string baseTitle_{};
};

} // namespace astraglyph
