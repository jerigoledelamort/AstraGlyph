#pragma once

#include "render/RenderSettings.hpp"

#include <string>
#include <vector>

namespace astraglyph {

class SettingsPanel {
public:
  void setVisible(bool visible) noexcept;
  [[nodiscard]] bool isVisible() const noexcept;
  [[nodiscard]] std::vector<std::string> buildLines(const RenderSettings& settings) const;

private:
  bool visible_{false};
};

} // namespace astraglyph
