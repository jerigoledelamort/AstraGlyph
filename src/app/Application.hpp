#pragma once

#include "platform/Input.hpp"
#include "platform/Time.hpp"
#include "platform/Window.hpp"
#include "render/AsciiFramebuffer.hpp"
#include "render/Renderer.hpp"
#include "render/RenderSettings.hpp"
#include "scene/Camera.hpp"
#include "scene/Scene.hpp"
#include "scene/SceneLoader.hpp"
#include "ui/UIPanel.hpp"

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
  void renderSceneToFramebuffer();
  void renderGlyph(
      char glyph,
      int x,
      int y,
      int cellWidth,
      int cellHeight,
      const Vec3& fgColor,
      const Vec3& bgColor);
  void renderCellToPixels(
      std::uint8_t* pixels,
      int pitch,
      int windowWidth,
      int windowHeight,
      int x,
      int y,
      int cellWidth,
      int cellHeight,
      char glyph,
      const Vec3& fgColor,
      const Vec3& bgColor,
      bool filled) const;
  void renderOverlayText(
      const std::vector<std::string>& lines,
      int originX,
      int originY,
      const Vec3& textColor,
      const Vec3& backgroundColor,
      int fixedWidth = 0);
  [[nodiscard]] int viewportWidth() const noexcept;

  Window window_;
  Input input_{};
  Time time_{}; 
  Camera camera_{};
  Renderer renderer_{};
  RenderSettings renderSettings_{};
  UIPanel uiPanel_{};
  AsciiFramebuffer framebuffer_{};
  Scene scene_{};
  std::string baseTitle_{};
  int titleUpdateCounter_ = 0;
  bool panelVisible_{true};
  int currentPanelWidth_ = 800;
};

} // namespace astraglyph
