#pragma once

#include "math/Vec3.hpp"
#include "render/RenderMetrics.hpp"
#include "render/RenderSettings.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace astraglyph {

class DebugOverlay {
public:
  void setVisible(bool visible) noexcept;
  [[nodiscard]] bool isVisible() const noexcept;
  [[nodiscard]] std::vector<std::string> buildLines(
      const RenderSettings& settings,
      const RenderMetrics& metrics,
      double fps,
      double frameTimeMs,
      std::size_t triangleCount,
      const Vec3& cameraPosition) const;

private:
  bool visible_{true};
};

} // namespace astraglyph
