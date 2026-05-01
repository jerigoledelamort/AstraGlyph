#include "ui/SettingsPanel.hpp"

#include <iomanip>
#include <sstream>

namespace astraglyph {
namespace {

[[nodiscard]] const char* enabledLabel(bool value) noexcept
{
  return value ? "ON" : "OFF";
}

} // namespace

void SettingsPanel::setVisible(bool visible) noexcept
{
  visible_ = visible;
}

bool SettingsPanel::isVisible() const noexcept
{
  return visible_;
}

std::vector<std::string> SettingsPanel::buildLines(const RenderSettings& settings) const
{
  std::vector<std::string> lines;
  if (!visible_) {
    return lines;
  }

  std::ostringstream stream;
  lines.push_back("SETTINGS");

  stream << "GRID " << settings.gridWidth << "X" << settings.gridHeight
         << "  RAMP " << RenderSettings::glyphRampModeName(settings.glyphRampMode);
  lines.push_back(stream.str());

  stream.str({});
  stream.clear();
  stream << "SAMPLES " << settings.samplesPerCell
         << "  MAX " << settings.maxSamplesPerCell
         << "  JITTER " << enabledLabel(settings.jitteredSampling)
         << "  ADAPTIVE " << enabledLabel(settings.adaptiveSampling);
  lines.push_back(stream.str());

  stream.str({});
  stream.clear();
  stream << std::fixed << std::setprecision(2)
         << "EXPOSURE " << settings.exposure
         << "  GAMMA " << settings.gamma
         << "  TEMPORAL " << enabledLabel(settings.temporalAccumulation);
  lines.push_back(stream.str());

  stream.str({});
  stream.clear();
  stream << "SHADOWS " << enabledLabel(settings.enableShadows)
         << "  SOFT " << enabledLabel(settings.enableSoftShadows)
         << "  SHADOW SAMPLES " << settings.shadowSamples;
  lines.push_back(stream.str());

  stream.str({});
  stream.clear();
  stream << "REFLECTIONS " << enabledLabel(settings.enableReflections)
         << "  BOUNCES " << settings.maxBounces
         << "  BVH " << enabledLabel(settings.enableBvh)
         << "  MT " << enabledLabel(settings.enableMultithreading);
  lines.push_back(stream.str());

  stream.str({});
  stream.clear();
  stream << "THREADS " << (settings.threadCount == 0 ? "AUTO" : std::to_string(settings.threadCount));
  lines.push_back(stream.str());

  stream.str({});
  stream.clear();
  stream << "COLOR " << enabledLabel(settings.colorOutput)
         << "  SHOW FPS " << enabledLabel(settings.showFps)
         << "  DEBUG " << enabledLabel(settings.showDebugInfo);
  lines.push_back(stream.str());

  lines.push_back("HOTKEYS");
  lines.push_back("F1 DEBUG OVERLAY");
  lines.push_back("F2 SETTINGS PANEL");
  lines.push_back("1 SHADOWS  2 SOFT SHADOWS  3 REFLECTIONS");
  lines.push_back("4 ADAPTIVE  5 BVH  T TEMPORAL");
  lines.push_back("[ ] SAMPLES PER CELL");
  lines.push_back("- = SOFT SHADOW SAMPLES OR BOUNCES");

  return lines;
}

} // namespace astraglyph
