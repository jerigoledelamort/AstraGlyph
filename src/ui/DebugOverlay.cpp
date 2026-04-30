#include "ui/DebugOverlay.hpp"

#include <iomanip>
#include <sstream>

namespace astraglyph {
namespace {

[[nodiscard]] const char* enabledLabel(bool value) noexcept
{
  return value ? "ON" : "OFF";
}

} // namespace

void DebugOverlay::setVisible(bool visible) noexcept
{
  visible_ = visible;
}

bool DebugOverlay::isVisible() const noexcept
{
  return visible_;
}

std::vector<std::string> DebugOverlay::buildLines(
    const RenderSettings& settings,
    const RenderMetrics& metrics,
    double fps,
    double frameTimeMs,
    std::size_t triangleCount,
    const Vec3& cameraPosition) const
{
  std::vector<std::string> lines;
  if (!visible_) {
    return lines;
  }

  std::ostringstream stream;
  stream << std::fixed << std::setprecision(1);
  stream << "FPS: " << fps << "  FRAME MS: " << frameTimeMs;
  lines.push_back(stream.str());

  stream.str({});
  stream.clear();
  stream << "GRID: " << settings.gridWidth << "X" << settings.gridHeight
         << "  SAMPLES: " << settings.samplesPerCell
         << "  AVG: " << std::setprecision(2) << metrics.averageSamplesPerCell;
  lines.push_back(stream.str());

  stream.str({});
  stream.clear();
  stream << "PRIMARY: " << metrics.primaryRays
         << "  SHADOW: " << metrics.shadowRays
         << "  REFLECT: " << metrics.reflectionRays;
  lines.push_back(stream.str());

  stream.str({});
  stream.clear();
  stream << "TRIANGLES: " << triangleCount
         << "  BVH NODES: " << metrics.bvhNodes
         << "  LEAVES: " << metrics.bvhLeaves;
  lines.push_back(stream.str());

  stream.str({});
  stream.clear();
  stream << std::setprecision(2)
         << "CAMERA: " << cameraPosition.x << ", " << cameraPosition.y << ", " << cameraPosition.z;
  lines.push_back(stream.str());

  stream.str({});
  stream.clear();
  stream << "FEATURES: SHADOWS " << enabledLabel(settings.enableShadows)
         << "  SOFT " << enabledLabel(settings.enableSoftShadows)
         << "  REFLECT " << enabledLabel(settings.enableReflections)
         << "  ADAPTIVE " << enabledLabel(settings.adaptiveSampling)
         << "  BVH " << enabledLabel(settings.enableBvh)
         << "  COLOR " << enabledLabel(settings.colorOutput);
  lines.push_back(stream.str());

  return lines;
}

} // namespace astraglyph
