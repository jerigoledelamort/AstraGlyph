#pragma once

#include "math/Vec3.hpp"
#include "render/RenderMetrics.hpp"
#include "render/RenderSettings.hpp"

#include <functional>
#include <string>
#include <vector>

namespace astraglyph {

class Window;

class UIPanel {
public:
  void buildFromSettings(const RenderSettings& settings);
  void updateFromSettings(const RenderSettings& settings,
                          const RenderMetrics& metrics,
                          double fps,
                          int triangleCount,
                          Vec3 cameraPos);
  void render(Window& window, int panelX, int panelY, int panelWidth, const RenderSettings& settings);
  void handleMouseClick(int mouseX, int mouseY, RenderSettings& settings, int direction);
  void handleMouseWheel(int delta);
  void setHovered(int mouseX, int mouseY);

private:
  enum class WidgetType { Label, Checkbox, Slider, Button };

  struct Widget {
    WidgetType type;
    std::string text;
    int y = 0;
    int height = 0;
    std::function<bool(const RenderSettings&)> getBool;
    std::function<void(RenderSettings&)> toggle;
    std::function<int(const RenderSettings&)> getInt;
    std::function<void(RenderSettings&, int)> adjust;
    int minVal = 0;
    int maxVal = 0;
    std::function<std::string()> getDynamicText;
    std::string hotkey;
    std::function<void(RenderSettings&)> action;
    std::function<std::string(const RenderSettings&)> getDynamicTextWithSettings;
  };

  void drawGlyphString(Window& window,
                       std::string_view text,
                       int x,
                       int y,
                       const Vec3& color) const;
  [[nodiscard]] int glyphStringWidth(std::string_view text) const noexcept;

  static constexpr int kScale = 2;
  static constexpr int kGlyphPixelWidth = 4;
  static constexpr int kGlyphPixelHeight = 5;
  static constexpr int kGlyphSpacing = 1;
  static constexpr int kLineSpacing = 2;
  static constexpr int kPadding = 12;
  static constexpr int kLineHeight = (kGlyphPixelHeight + kLineSpacing) * kScale;
  static constexpr int kWidgetHeight = kLineHeight + 6;
  static constexpr int kScrollbarWidth = 8;

  int scrollOffset_ = 0;
  int panelWidth_ = 0;
  int panelHeight_ = 0;
  int totalContentHeight_ = 0;
  int hoveredWidgetIndex_ = -1;

  double fps_ = 0.0;
  int triangleCount_ = 0;
  Vec3 cameraPos_{};
  float avgSamples_ = 0.0F;
  int gridW_ = 0;
  int gridH_ = 0;
  int samples_ = 0;

  std::vector<Widget> widgets_;
};

} // namespace astraglyph
