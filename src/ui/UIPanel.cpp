#include "ui/UIPanel.hpp"

#include "platform/Window.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace astraglyph {
namespace {

constexpr int kGlyphPixelWidth = 4;
constexpr int kGlyphPixelHeight = 5;

[[nodiscard]] std::array<std::string_view, kGlyphPixelHeight> glyphPattern(char character) noexcept
{
  switch (character) {
    case 'A': return {"0110", "1001", "1111", "1001", "1001"};
    case 'B': return {"1110", "1001", "1110", "1001", "1110"};
    case 'C': return {"0111", "1000", "1000", "1000", "0111"};
    case 'D': return {"1110", "1001", "1001", "1001", "1110"};
    case 'E': return {"1111", "1000", "1110", "1000", "1111"};
    case 'F': return {"1111", "1000", "1110", "1000", "1000"};
    case 'G': return {"0111", "1000", "1011", "1001", "0111"};
    case 'H': return {"1001", "1001", "1111", "1001", "1001"};
    case 'I': return {"1111", "0010", "0010", "0010", "1111"};
    case 'J': return {"0011", "0001", "0001", "1001", "0110"};
    case 'K': return {"1001", "1010", "1100", "1010", "1001"};
    case 'L': return {"1000", "1000", "1000", "1000", "1111"};
    case 'M': return {"1001", "1111", "1111", "1001", "1001"};
    case 'N': return {"1001", "1101", "1011", "1001", "1001"};
    case 'O': return {"0110", "1001", "1001", "1001", "0110"};
    case 'P': return {"1110", "1001", "1110", "1000", "1000"};
    case 'Q': return {"0110", "1001", "1001", "1011", "0111"};
    case 'R': return {"1110", "1001", "1110", "1010", "1001"};
    case 'S': return {"0111", "1000", "0110", "0001", "1110"};
    case 'T': return {"1111", "0010", "0010", "0010", "0010"};
    case 'U': return {"1001", "1001", "1001", "1001", "0110"};
    case 'V': return {"1001", "1001", "1001", "0101", "0010"};
    case 'W': return {"1001", "1001", "1111", "1111", "1001"};
    case 'X': return {"1001", "0101", "0010", "0101", "1001"};
    case 'Y': return {"1001", "0101", "0010", "0010", "0010"};
    case 'Z': return {"1111", "0001", "0010", "0100", "1111"};
    case '0': return {"0110", "1001", "1001", "1001", "0110"};
    case '1': return {"0010", "0110", "0010", "0010", "0111"};
    case '2': return {"1110", "0001", "0110", "1000", "1111"};
    case '3': return {"1110", "0001", "0110", "0001", "1110"};
    case '4': return {"1001", "1001", "1111", "0001", "0001"};
    case '5': return {"1111", "1000", "1110", "0001", "1110"};
    case '6': return {"0111", "1000", "1110", "1001", "0110"};
    case '7': return {"1111", "0001", "0010", "0100", "0100"};
    case '8': return {"0110", "1001", "0110", "1001", "0110"};
    case '9': return {"0110", "1001", "0111", "0001", "1110"};
    case ':': return {"0000", "0010", "0000", "0010", "0000"};
    case '.': return {"0000", "0000", "0000", "0010", "0010"};
    case ',': return {"0000", "0000", "0000", "0010", "0100"};
    case '-': return {"0000", "0000", "1111", "0000", "0000"};
    case '/': return {"0001", "0010", "0010", "0100", "1000"};
    case '[': return {"0111", "0100", "0100", "0100", "0111"};
    case ']': return {"1110", "0010", "0010", "0010", "1110"};
    case '=': return {"0000", "1111", "0000", "1111", "0000"};
    case '#': return {"0101", "1111", "0101", "1111", "0101"};
    case ' ': return {"0000", "0000", "0000", "0000", "0000"};
    default:  return {"1111", "0001", "0010", "0000", "0010"};
  }
}

} // namespace

void UIPanel::buildFromSettings(const RenderSettings& /*settings*/)
{
  widgets_.clear();
  int y = kPadding;

  auto add = [&](Widget w) {
    w.y = y;
    w.height = kWidgetHeight;
    widgets_.push_back(std::move(w));
    y += kWidgetHeight;
  };

  add({WidgetType::Label, "RENDER SETTINGS"});

  add({WidgetType::Checkbox, "SHADOWS", 0, 0, nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr, "(1)", nullptr});
  widgets_.back().getBool = [](const RenderSettings& s) { return s.enableShadows; };
  widgets_.back().toggle = [](RenderSettings& s) { s.toggleShadows(); };

  add({WidgetType::Checkbox, "SOFT SHADOWS", 0, 0, nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr, "(2)", nullptr});
  widgets_.back().getBool = [](const RenderSettings& s) { return s.enableSoftShadows; };
  widgets_.back().toggle = [](RenderSettings& s) { s.toggleSoftShadows(); };

  add({WidgetType::Slider, "SHADOW SAMPLES", 0, 0, nullptr, nullptr,
       [](const RenderSettings& s) { return s.shadowSamples; },
       [](RenderSettings& s, int d) { s.adjustShadowSamples(d); },
       1, 16, nullptr, "(- =)"});

  add({WidgetType::Checkbox, "REFLECTIONS", 0, 0, nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr, "(3)", nullptr});
  widgets_.back().getBool = [](const RenderSettings& s) { return s.enableReflections; };
  widgets_.back().toggle = [](RenderSettings& s) { s.toggleReflections(); };

  add({WidgetType::Slider, "MAX BOUNCES", 0, 0, nullptr, nullptr,
       [](const RenderSettings& s) { return s.maxBounces; },
       [](RenderSettings& s, int d) { s.adjustMaxBounces(d); },
       0, 8, nullptr, "(- =)"});

  add({WidgetType::Checkbox, "BVH", 0, 0, nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr, "(5)", nullptr});
  widgets_.back().getBool = [](const RenderSettings& s) { return s.enableBvh; };
  widgets_.back().toggle = [](RenderSettings& s) { s.toggleBvh(); };

  add({WidgetType::Checkbox, "ADAPTIVE SAMPLING", 0, 0, nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr, "(4)", nullptr});
  widgets_.back().getBool = [](const RenderSettings& s) { return s.adaptiveSampling; };
  widgets_.back().toggle = [](RenderSettings& s) { s.toggleAdaptiveSampling(); };

  add({WidgetType::Checkbox, "TEMPORAL ACCUMULATION", 0, 0, nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr, "(T)", nullptr});
  widgets_.back().getBool = [](const RenderSettings& s) { return s.temporalAccumulation; };
  widgets_.back().toggle = [](RenderSettings& s) { s.toggleTemporalAccumulation(); };

  add({WidgetType::Slider, "SAMPLES PER CELL", 0, 0, nullptr, nullptr,
       [](const RenderSettings& s) { return s.samplesPerCell; },
       [](RenderSettings& s, int d) { s.adjustSamplesPerCell(d); },
       1, 16, nullptr, "([ ])"});

  add({WidgetType::Checkbox, "MULTITHREADING", 0, 0, nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr, "", nullptr});
  widgets_.back().getBool = [](const RenderSettings& s) { return s.enableMultithreading; };
  widgets_.back().toggle = [](RenderSettings& s) { s.enableMultithreading = !s.enableMultithreading; };

  add({WidgetType::Checkbox, "COLOR OUTPUT", 0, 0, nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr, "", nullptr});
  widgets_.back().getBool = [](const RenderSettings& s) { return s.colorOutput; };
  widgets_.back().toggle = [](RenderSettings& s) { s.colorOutput = !s.colorOutput; };

  add({WidgetType::Checkbox, "SHOW FPS", 0, 0, nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr, "", nullptr});
  widgets_.back().getBool = [](const RenderSettings& s) { return s.showFps; };
  widgets_.back().toggle = [](RenderSettings& s) { s.toggleShowFps(); };

  add({WidgetType::Checkbox, "SHOW DEBUG INFO", 0, 0, nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr, "(SPACE)", nullptr});
  widgets_.back().getBool = [](const RenderSettings& s) { return s.showDebugInfo; };
  widgets_.back().toggle = [](RenderSettings& s) { s.toggleShowDebugInfo(); };

  add({WidgetType::Checkbox, "RENDER PROFILING", 0, 0, nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr, "(F3)", nullptr});
  widgets_.back().getBool = [](const RenderSettings& s) { return s.enableRenderProfiling; };
  widgets_.back().toggle = [](RenderSettings& s) { s.toggleRenderProfiling(); };

  add({WidgetType::Button, "[ RESET ALL SETTINGS ]", 0, 0, nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr, "",
       [](RenderSettings& s) { s.resetToDefaults(); }});

  add({WidgetType::Label, "DISPLAY SETTINGS"});

  add({WidgetType::Label, "", 0, 0, nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr, "", nullptr,
       [](const RenderSettings& s) {
         return "RESOLUTION: " + std::to_string(s.windowWidth) + "x" + std::to_string(s.windowHeight);
       }});

  add({WidgetType::Button, "[ NEXT RESOLUTION ]", 0, 0, nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr, "",
       [](RenderSettings& s) {
         s.resolutionIndex = (s.resolutionIndex + 1) % 4;
         s.applyResolutionIndex();
       }});

  add({WidgetType::Slider, "CELL SIZE", 0, 0, nullptr, nullptr,
       [](const RenderSettings& s) { return s.cellPixelSize; },
       [](RenderSettings& s, int d) {
         s.cellPixelSize = std::clamp(s.cellPixelSize + d, 4, 32);
         s.updateGridFromWindowAndCell();
         s.windowSizeDirty = true;
         s.markChanged(RenderSettings::DirtyAccumulation | RenderSettings::DirtyPresentation);
       },
       4, 32, nullptr, ""});

  add({WidgetType::Label, "STATISTICS"});

  add({WidgetType::Label, "", 0, 0, nullptr, nullptr, nullptr, nullptr, 0, 0,
       [this]() {
         std::ostringstream ss;
         ss << "FPS: " << std::fixed << std::setprecision(1) << fps_;
         return ss.str();
       }});

  add({WidgetType::Label, "", 0, 0, nullptr, nullptr, nullptr, nullptr, 0, 0,
       [this]() {
         return "GRID: " + std::to_string(gridW_) + "X" + std::to_string(gridH_);
       }});

  add({WidgetType::Label, "", 0, 0, nullptr, nullptr, nullptr, nullptr, 0, 0,
       [this]() {
         return "SAMPLES: " + std::to_string(samples_);
       }});

  add({WidgetType::Label, "", 0, 0, nullptr, nullptr, nullptr, nullptr, 0, 0,
       [this]() {
         std::ostringstream ss;
         ss << "AVG SAMPLES: " << std::fixed << std::setprecision(2) << avgSamples_;
         return ss.str();
       }});

  add({WidgetType::Label, "", 0, 0, nullptr, nullptr, nullptr, nullptr, 0, 0,
       [this]() {
         return std::string("TRIANGLES: ") + std::to_string(triangleCount_);
       }});

  add({WidgetType::Label, "", 0, 0, nullptr, nullptr, nullptr, nullptr, 0, 0,
       [this]() {
         std::ostringstream ss;
         ss << "CAM: " << std::fixed << std::setprecision(2)
            << cameraPos_.x << ", " << cameraPos_.y << ", " << cameraPos_.z;
         return ss.str();
       }});

  add({WidgetType::Label, "HOTKEYS"});
  add({WidgetType::Label, "F1 PANEL  F FULLSCREEN"});
  add({WidgetType::Label, "F3 PROFILE  1-6 TOGGLES"});
  add({WidgetType::Label, "T TEMPORAL  SPACE DEBUG"});
  add({WidgetType::Label, "[ ] SAMPLES  - = QUALITY"});

  totalContentHeight_ = y + kPadding;
}

void UIPanel::updateFromSettings(const RenderSettings& settings,
                                 const RenderMetrics& metrics,
                                 double fps,
                                 int triangleCount,
                                 Vec3 cameraPos)
{
  fps_ = fps;
  triangleCount_ = triangleCount;
  cameraPos_ = cameraPos;
  avgSamples_ = metrics.averageSamplesPerCell;
  gridW_ = settings.gridWidth;
  gridH_ = settings.gridHeight;
  samples_ = settings.samplesPerCell;
}

void UIPanel::render(Window& window, int panelX, int panelY, int panelWidth, const RenderSettings& settings)
{
  panelWidth_ = panelWidth;
  panelHeight_ = window.height();

  // Background
  window.drawFilledRect(panelX, panelY, panelWidth_, panelHeight_, Vec3{0.08F, 0.08F, 0.11F});

  // Border
  const Vec3 borderColor{0.25F, 0.25F, 0.35F};
  window.drawFilledRect(panelX, panelY, panelWidth_, 2, borderColor);
  window.drawFilledRect(panelX, panelY + panelHeight_ - 2, panelWidth_, 2, borderColor);
  window.drawFilledRect(panelX, panelY, 2, panelHeight_, borderColor);
  window.drawFilledRect(panelX + panelWidth_ - 2, panelY, 2, panelHeight_, borderColor);

  // Scrollbar
  if (totalContentHeight_ > panelHeight_) {
    const int scrollbarX = panelX + panelWidth_ - kScrollbarWidth - 4;
    const int scrollbarY = panelY;
    const int scrollbarH = panelHeight_;
    window.drawFilledRect(scrollbarX, scrollbarY, kScrollbarWidth, scrollbarH, Vec3{0.05F, 0.05F, 0.07F});

    const int thumbHeight = std::max(8, panelHeight_ * panelHeight_ / totalContentHeight_);
    const int maxScroll = std::max(1, totalContentHeight_ - panelHeight_);
    const int thumbY = scrollbarY + (scrollOffset_ * (scrollbarH - thumbHeight) / maxScroll);
    window.drawFilledRect(scrollbarX, thumbY, kScrollbarWidth, thumbHeight, Vec3{0.35F, 0.35F, 0.45F});
  }

  for (std::size_t i = 0; i < widgets_.size(); ++i) {
    const auto& w = widgets_[i];
    int screenY = panelY + w.y - scrollOffset_;
    if (screenY + w.height < panelY || screenY > panelY + panelHeight_) {
      continue;
    }

    int textX = panelX + kPadding;
    int textY = screenY + (w.height - kLineHeight) / 2;

    // Hover background
    if (static_cast<int>(i) == hoveredWidgetIndex_ &&
        (w.type == WidgetType::Checkbox || w.type == WidgetType::Slider || w.type == WidgetType::Button)) {
      window.drawFilledRect(
          panelX + 2, screenY, panelWidth_ - 4, w.height, Vec3{0.12F, 0.12F, 0.18F});
    }

    switch (w.type) {
      case WidgetType::Label: {
        Vec3 color{0.7F, 0.7F, 0.8F};
        std::string text = w.text;
        if (w.getDynamicText) {
          text = w.getDynamicText();
        } else if (w.getDynamicTextWithSettings) {
          text = w.getDynamicTextWithSettings(settings);
        }
        if (!text.empty()) {
          drawGlyphString(window, text, textX, textY, color);
        }
        break;
      }
      case WidgetType::Checkbox: {
        bool val = w.getBool(settings);
        std::string text = val ? "[X] " + w.text : "[ ] " + w.text;
        if (!w.hotkey.empty()) {
          text += " " + w.hotkey;
        }
        drawGlyphString(window, text, textX, textY, Vec3{0.9F, 0.9F, 0.9F});
        break;
      }
      case WidgetType::Slider: {
        int val = w.getInt(settings);
        std::string text = w.text + ": <" + std::to_string(val) + ">";
        if (!w.hotkey.empty()) {
          text += " " + w.hotkey;
        }
        drawGlyphString(window, text, textX, textY, Vec3{0.9F, 0.9F, 0.9F});
        break;
      }
      case WidgetType::Button: {
        drawGlyphString(window, w.text, textX, textY, Vec3{1.0F, 0.85F, 0.6F});
        break;
      }
    }
  }
}

void UIPanel::setHovered(int mouseX, int mouseY)
{
  if (mouseX < kPadding || mouseX > panelWidth_ - kPadding || mouseY < 0 || mouseY > panelHeight_) {
    hoveredWidgetIndex_ = -1;
    return;
  }

  for (std::size_t i = 0; i < widgets_.size(); ++i) {
    int widgetTop = widgets_[i].y - scrollOffset_;
    int widgetBottom = widgetTop + widgets_[i].height;
    if (mouseY >= widgetTop && mouseY < widgetBottom) {
      hoveredWidgetIndex_ = static_cast<int>(i);
      return;
    }
  }
  hoveredWidgetIndex_ = -1;
}

void UIPanel::handleMouseClick(int mouseX, int mouseY, RenderSettings& settings, int direction)
{
  if (mouseX < kPadding || mouseX > panelWidth_ - kPadding) {
    return;
  }

  for (const auto& w : widgets_) {
    int widgetTop = w.y - scrollOffset_;
    int widgetBottom = widgetTop + w.height;

    if (mouseY >= widgetTop && mouseY < widgetBottom) {
      if (w.type == WidgetType::Checkbox && direction > 0 && w.toggle) {
        w.toggle(settings);
      } else if (w.type == WidgetType::Slider && w.adjust) {
        int cur = w.getInt(settings);
        int next = cur + direction;
        if (next > w.maxVal) {
          next = w.minVal;
        }
        if (next < w.minVal) {
          next = w.maxVal;
        }
        w.adjust(settings, next - cur);
      } else if (w.type == WidgetType::Button && direction > 0 && w.action) {
        w.action(settings);
      }
      break;
    }
  }
}

void UIPanel::handleMouseWheel(int delta)
{
  const int maxScroll = std::max(0, totalContentHeight_ - panelHeight_);
  scrollOffset_ -= delta * 30;
  scrollOffset_ = std::clamp(scrollOffset_, 0, maxScroll);
}

void UIPanel::drawGlyphString(Window& window,
                              std::string_view text,
                              int x,
                              int y,
                              const Vec3& color) const
{
  const int glyphAdvance = (kGlyphPixelWidth + kGlyphSpacing) * kScale;
  int cx = x;
  for (char raw : text) {
    const char ch = static_cast<char>(std::toupper(static_cast<unsigned char>(raw)));
    const auto pattern = glyphPattern(ch);
    for (int row = 0; row < kGlyphPixelHeight; ++row) {
      for (int col = 0; col < kGlyphPixelWidth; ++col) {
        if (pattern[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] != '1') {
          continue;
        }
        window.drawFilledRect(
            cx + col * kScale,
            y + row * kScale,
            kScale,
            kScale,
            color);
      }
    }
    cx += glyphAdvance;
  }
}

[[nodiscard]] int UIPanel::glyphStringWidth(std::string_view text) const noexcept
{
  if (text.empty()) {
    return 0;
  }
  const int glyphAdvance = (kGlyphPixelWidth + kGlyphSpacing) * kScale;
  return static_cast<int>(text.size()) * glyphAdvance - (kGlyphSpacing * kScale);
}

} // namespace astraglyph
