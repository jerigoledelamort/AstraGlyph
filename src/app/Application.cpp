#include "app/Application.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string_view>

#include "terminus_ttf_data.h"

namespace astraglyph {
namespace {

constexpr int kGlyphPixelWidth = 4;
constexpr int kGlyphPixelHeight = 5;
constexpr int kGlyphSpacing = 1;
constexpr int kLineSpacing = 2;

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
    default: return {"1111", "0001", "0010", "0000", "0010"};
  }
}

[[nodiscard]] int linePixelWidth(const std::string& line, int scale) noexcept
{
  if (line.empty()) {
    return 0;
  }

  const int glyphAdvance = (kGlyphPixelWidth + kGlyphSpacing) * scale;
  return static_cast<int>(line.size()) * glyphAdvance - (kGlyphSpacing * scale);
}

[[nodiscard]] double elapsedMs(
    std::chrono::high_resolution_clock::time_point start,
    std::chrono::high_resolution_clock::time_point end) noexcept
{
  return std::chrono::duration<double, std::milli>(end - start).count();
}

} // namespace

Application::Application()
    : window_{1280, 720, "AstraGlyph / AsciiTracer"}, baseTitle_{"AstraGlyph / AsciiTracer"}, glyphAtlas_{font_data, font_data_len}
{
  // Load samurai_girl scene (original with textures)
  const auto result = SceneLoader::loadFromFile("assets/scenes/cornell_box_scene.json");
  scene_ = std::move(result.scene);
  camera_ = result.camera;
  renderSettings_ = result.renderSettings;

  camera_.setAspect(static_cast<float>(window_.width()) / static_cast<float>(window_.height()));
  uiPanel_.buildFromSettings(renderSettings_);

  const int windowWidth = window_.width();
  const int windowHeight = window_.height();
  const int gridWidth = std::max(renderSettings_.gridWidth, 1);
  const int gridHeight = std::max(renderSettings_.gridHeight, 1);
  const int cellW = std::max(1, windowWidth / gridWidth);
  const int cellH = std::max(1, windowHeight / gridHeight);
  rebuildGlyphAtlas(cellW, cellH);
  lastCellW_ = cellW;
  lastCellH_ = cellH;
}

int Application::run()
{
  while (!window_.shouldClose()) {
    window_.updateSize();
    time_.update();
    input_.beginFrame();
    input_.pollEvents(window_);
    update(static_cast<float>(time_.deltaTime()));
    render();
    if (++titleUpdateCounter_ >= 6) {
      titleUpdateCounter_ = 0;
      updateWindowTitle();
    }
  }

  return 0;
}

void Application::update(double dt)
{
  handleRuntimeSettingsInput();

  if (renderSettings_.windowSizeDirty) {
    window_.setSize(renderSettings_.windowWidth, renderSettings_.windowHeight);
    renderSettings_.windowSizeDirty = false;
  }

  camera_.setAspect(static_cast<float>(window_.width()) / static_cast<float>(window_.height()));

  currentPanelWidth_ = std::max(300, std::min(800, static_cast<int>(window_.width() * 0.25)));

  const MouseState ms = input_.mouseState();
  input_.setMouseInPanel(panelVisible_ && ms.x >= viewportWidth());
  input_.applyMouseCapture(window_);
  
  if (panelVisible_ && ms.x >= viewportWidth()) {
    const int localX = ms.x - (window_.width() - currentPanelWidth_);
    if (input_.wasMouseButtonPressed(MouseButton::Left)) {
      uiPanel_.handleMouseClick(localX, ms.y, renderSettings_, 1);
    }
    if (input_.wasMouseButtonPressed(MouseButton::Right)) {
      uiPanel_.handleMouseClick(localX, ms.y, renderSettings_, -1);
    }
    if (ms.wheelDelta != 0) {
      uiPanel_.handleMouseWheel(ms.wheelDelta);
    }
    uiPanel_.setHovered(localX, ms.y);
  } else {
    uiPanel_.setHovered(-1, -1);
  }

  camera_.updateFromInput(input_, static_cast<float>(dt));
}

void Application::render()
{
  if (showGlyphAtlas_) {
    renderGlyphAtlasDebug();
    window_.present();
    return;
  }

  renderSceneToFramebuffer();

  if (panelVisible_) {
    uiPanel_.updateFromSettings(
        renderSettings_,
        renderer_.metrics(),
        time_.fps(),
        static_cast<int>(renderer_.sceneTriangleCount()),
        camera_.position());
    uiPanel_.render(window_, window_.width() - currentPanelWidth_, 0, currentPanelWidth_, renderSettings_);
  }

  window_.present();
}

void Application::renderSceneToFramebuffer()
{
  const bool profilingEnabled = renderSettings_.enableRenderProfiling;
  const auto frameStart = std::chrono::high_resolution_clock::now();
  renderSettings_.validate();
  renderer_.render(framebuffer_, camera_, scene_, renderSettings_);
  renderSettings_.clearDirtyState();
  ++renderSettings_.frameIndex;

  const int gridWidth = std::max(renderSettings_.gridWidth, 1);
  const int gridHeight = std::max(renderSettings_.gridHeight, 1);
  const int windowWidth = std::max(window_.width(), 1);
  const int windowHeight = std::max(window_.height(), 1);

  window_.beginFrame(Vec3{0.02F, 0.02F, 0.03F});
  const auto outputStart = std::chrono::high_resolution_clock::now();

  void* pixels = nullptr;
  int pitch = 0;
  const bool pixelBufferReady = window_.lockFramePixels(windowWidth, windowHeight, &pixels, &pitch);

  if (pixelBufferReady) {
    auto* pixelData = static_cast<std::uint8_t*>(pixels);

    for (int y = 0; y < gridHeight; ++y) {
      const int y0 = (y * windowHeight) / gridHeight;
      const int y1 = ((y + 1) * windowHeight) / gridHeight;
      const int cellHeight = std::max(1, y1 - y0);

      for (int x = 0; x < gridWidth; ++x) {
        const int x0 = (x * windowWidth) / gridWidth;
        const int x1 = ((x + 1) * windowWidth) / gridWidth;
        const int cellWidth = std::max(1, x1 - x0);

        if (cellWidth != lastCellW_ || cellHeight != lastCellH_) {
          rebuildGlyphAtlas(cellWidth, cellHeight);
          lastCellW_ = cellWidth;
          lastCellH_ = cellHeight;
        }

        const AsciiCell& cell = framebuffer_.at(static_cast<std::size_t>(x), static_cast<std::size_t>(y));
        const Vec3 fg = cell.fg;
        const bool filled = renderSettings_.glyphRampMode == GlyphRampMode::Filled;
        renderCellToPixels(
            pixelData, pitch, windowWidth, windowHeight,
            x0, y0, cellWidth, cellHeight,
            cell.glyph, fg, cell.bg, filled);
      }
    }

    window_.unlockFramePixels();
  }
  if (profilingEnabled) {
    const auto outputEnd = std::chrono::high_resolution_clock::now();
    renderer_.setPresentationProfiling(
        elapsedMs(outputStart, outputEnd),
        elapsedMs(frameStart, outputEnd));
  }
}

void Application::handleRuntimeSettingsInput()
{
  if (input_.wasKeyPressed(Key::F1)) {
    panelVisible_ = !panelVisible_;
  }
  if (input_.wasKeyPressed(Key::F)) {
    window_.setFullscreen(!window_.isFullscreen());
  }
  if (input_.wasKeyPressed(Key::F3)) {
    renderSettings_.toggleRenderProfiling();
  }
  if (input_.wasKeyPressed(Key::F4)) {
    showGlyphAtlas_ = !showGlyphAtlas_;
  }
  if (input_.wasKeyPressed(Key::Digit1)) {
    renderSettings_.toggleShadows();
  }
  if (input_.wasKeyPressed(Key::Digit2)) {
    renderSettings_.toggleSoftShadows();
  }
  if (input_.wasKeyPressed(Key::Digit3)) {
    renderSettings_.toggleReflections();
  }
  if (input_.wasKeyPressed(Key::Digit4)) {
    renderSettings_.toggleAdaptiveSampling();
  }
  if (input_.wasKeyPressed(Key::Digit5)) {
    renderSettings_.toggleBvh();
  }
  if (input_.wasKeyPressed(Key::Digit6)) {
    renderSettings_.cycleGlyphRampMode();
  }
  if (input_.wasKeyPressed(Key::T)) {
    renderSettings_.toggleTemporalAccumulation();
  }
  if (input_.wasKeyPressed(Key::Space)) {
    renderSettings_.cycleDebugViewMode();
  }
  if (input_.wasKeyPressed(Key::LeftBracket)) {
    renderSettings_.adjustSamplesPerCell(-1);
  }
  if (input_.wasKeyPressed(Key::RightBracket)) {
    renderSettings_.adjustSamplesPerCell(1);
  }
  if (input_.wasKeyPressed(Key::Minus)) {
    renderSettings_.adjustSecondaryQuality(-1);
  }
  if (input_.wasKeyPressed(Key::Equals)) {
    renderSettings_.adjustSecondaryQuality(1);
  }
}

void Application::renderGlyphAtlasDebug()
{
  window_.beginFrame(Vec3{0.0F, 0.0F, 0.0F});

  void* pixels = nullptr;
  int pitch = 0;
  const int w = window_.width();
  const int h = window_.height();
  if (!window_.lockFramePixels(w, h, &pixels, &pitch)) {
    return;
  }

  auto* pixelData = static_cast<std::uint8_t*>(pixels);

  // Fill black
  for (int row = 0; row < h; ++row) {
    for (int col = 0; col < w; ++col) {
      std::uint8_t* p = pixelData + row * pitch + col * 4;
      p[0] = 0;
      p[1] = 0;
      p[2] = 0;
      p[3] = 255;
    }
  }

  const std::string chars =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:.,-/[]=# @#$%?&*iI:-.|\\/=";

  const int cellW = lastCellW_;
  const int cellH = lastCellH_;
  if (cellW <= 0 || cellH <= 0) {
    window_.unlockFramePixels();
    return;
  }

  const int cols = std::max(1, w / cellW);
  const Vec3 white{1.0F, 1.0F, 1.0F};
  const Vec3 black{0.0F, 0.0F, 0.0F};

  for (std::size_t i = 0; i < chars.size(); ++i) {
    const int gx = static_cast<int>(i) % cols;
    const int gy = static_cast<int>(i) / cols;
    const int x = gx * cellW;
    const int y = gy * cellH;
    if (x + cellW > w || y + cellH > h) {
      break;
    }
    renderCellToPixels(
        pixelData, pitch, w, h,
        x, y, cellW, cellH,
        chars[i], white, black, false);
  }

  window_.unlockFramePixels();
}

void Application::renderGlyph(
    char glyph,
    int x,
    int y,
    int cellWidth,
    int cellHeight,
    const Vec3& fgColor,
    const Vec3& bgColor)
{
  if (bgColor.x > 0.0F || bgColor.y > 0.0F || bgColor.z > 0.0F) {
    window_.drawFilledRect(x, y, cellWidth, cellHeight, bgColor);
  }

  if (glyph == ' ') {
    return;
  }

  const int scale = std::min(cellWidth / kGlyphPixelWidth, cellHeight / kGlyphPixelHeight);
  if (scale <= 0) {
    window_.drawFilledRect(x, y, cellWidth, cellHeight, fgColor);
    return;
  }

  const char character = static_cast<char>(std::toupper(static_cast<unsigned char>(glyph)));
  const auto pattern = glyphPattern(character);
  const int drawWidth = kGlyphPixelWidth * scale;
  const int drawHeight = kGlyphPixelHeight * scale;
  const int offsetX = x + (cellWidth - drawWidth) / 2;
  const int offsetY = y + (cellHeight - drawHeight) / 2;

  for (int row = 0; row < kGlyphPixelHeight; ++row) {
    for (int column = 0; column < kGlyphPixelWidth; ++column) {
      if (pattern[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)] != '1') {
        continue;
      }
      window_.drawFilledRect(
          offsetX + column * scale,
          offsetY + row * scale,
          scale,
          scale,
          fgColor);
    }
  }
}

void Application::renderCellToPixels(
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
    bool filled) const
{
  (void)windowWidth;
  (void)windowHeight;
  const auto setPixel = [&](int px, int py, const Vec3& color) {
    std::uint8_t* p = pixels + py * pitch + px * 4;
    p[0] = static_cast<std::uint8_t>(std::clamp(color.x, 0.0F, 1.0F) * 255.0F);  // R
    p[1] = static_cast<std::uint8_t>(std::clamp(color.y, 0.0F, 1.0F) * 255.0F);  // G
    p[2] = static_cast<std::uint8_t>(std::clamp(color.z, 0.0F, 1.0F) * 255.0F);  // B
    p[3] = 255;  // A
  };

  if (filled) {
    for (int row = y; row < y + cellHeight; ++row) {
      for (int col = x; col < x + cellWidth; ++col) {
        setPixel(col, row, fgColor);
      }
    }
    return;
  }

  const auto& mask = glyphAtlas_.getMask(glyph);
  const bool hasMask = !mask.empty();

  for (int row = y; row < y + cellHeight; ++row) {
    for (int col = x; col < x + cellWidth; ++col) {
      if (!hasMask) {
        setPixel(col, row, fgColor);
        continue;
      }
      const int mx = col - x;
      const int my = row - y;
      const std::size_t idx = static_cast<std::size_t>(my * cellWidth + mx);
      const uint8_t bit = (idx < mask.size()) ? mask[idx] : 0;
      setPixel(col, row, bit ? fgColor : bgColor);
    }
  }
}

void Application::updateWindowTitle()
{
  const Vec3 position = camera_.position();
  const RenderMetrics& metrics = renderer_.metrics();
  std::ostringstream title;
  title << baseTitle_;
  if (renderSettings_.showFps) {
    title << " | FPS: " << std::fixed << std::setprecision(1) << time_.fps();
  }
  title << " | Grid: " << renderSettings_.gridWidth << "x" << renderSettings_.gridHeight;
  title << " | Samples: " << renderSettings_.samplesPerCell;
  title << " | Avg Samples: " << std::setprecision(2) << metrics.averageSamplesPerCell;
  title << " | BVH: " << (renderSettings_.enableBvh ? "on" : "off");
  title << " | Triangles: " << renderer_.sceneTriangleCount();
  title << " | Camera: " << std::setprecision(2) << position.x << ", " << position.y << ", " << position.z;
  window_.setTitle(title.str());
}

void Application::renderOverlayText(
    const std::vector<std::string>& lines,
    int originX,
    int originY,
    const Vec3& textColor,
    const Vec3& backgroundColor,
    int fixedWidth)
{
  if (lines.empty()) {
    return;
  }

  const int scale = std::max(2, std::min(window_.width() / 420, window_.height() / 240));
  const int glyphAdvance = (kGlyphPixelWidth + kGlyphSpacing) * scale;
  const int lineAdvance = (kGlyphPixelHeight + kLineSpacing) * scale;
  int maxWidth = 0;
  for (const std::string& line : lines) {
    maxWidth = std::max(maxWidth, linePixelWidth(line, scale));
  }

  const int panelPadding = 8 * scale;
  const int panelWidth = fixedWidth > 0 ? fixedWidth : maxWidth + panelPadding * 2;
  const int panelHeight = static_cast<int>(lines.size()) * lineAdvance - (kLineSpacing * scale) + panelPadding * 2;
  window_.drawFilledRect(originX, originY, panelWidth, panelHeight, backgroundColor);

  int y = originY + panelPadding;
  for (const std::string& line : lines) {
    int x = originX + panelPadding;
    for (char rawCharacter : line) {
      const char character = static_cast<char>(std::toupper(static_cast<unsigned char>(rawCharacter)));
      const auto pattern = glyphPattern(character);
      for (int row = 0; row < kGlyphPixelHeight; ++row) {
        for (int column = 0; column < kGlyphPixelWidth; ++column) {
          if (pattern[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)] != '1') {
            continue;
          }

          window_.drawFilledRect(
              x + column * scale,
              y + row * scale,
              scale,
              scale,
              textColor);
        }
      }
      x += glyphAdvance;
    }
    y += lineAdvance;
  }
}

void Application::rebuildGlyphAtlas(int cellW, int cellH)
{
  std::string charset;
  charset.reserve(128);
  // UI / overlay characters
  for (char c = 'A'; c <= 'Z'; ++c) {
    charset.push_back(c);
  }
  for (char c = '0'; c <= '9'; ++c) {
    charset.push_back(c);
  }
  const char* specials = ":.,-/[]=# ";
  charset += specials;
  // ASCII ramp characters (must match AsciiMapper ramps)
  const char* classicRamp = "@#$%?&*iI:-. ";
  const char* horizontalRamp = "=- ";
  const char* verticalRamp = "| ";
  const char* diagonalRamp = "/\\ ";
  charset += classicRamp;
  charset += horizontalRamp;
  charset += verticalRamp;
  charset += diagonalRamp;

  std::sort(charset.begin(), charset.end());
  charset.erase(std::unique(charset.begin(), charset.end()), charset.end());

  glyphAtlas_.build(cellW, cellH, charset);
}

int Application::viewportWidth() const noexcept
{
  return std::max(1, window_.width() - (panelVisible_ ? currentPanelWidth_ : 0));
}

} // namespace astraglyph
