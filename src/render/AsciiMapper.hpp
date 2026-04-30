#pragma once

#include "math/Vec3.hpp"
#include "render/RenderSettings.hpp"

namespace astraglyph {

class AsciiMapper {
public:
  [[nodiscard]] char mapLuminanceToGlyph(float luminance, GlyphRampMode mode = GlyphRampMode::Classic) const noexcept;
  [[nodiscard]] float radianceToLuminance(const Vec3& radiance) const noexcept;
  [[nodiscard]] Vec3 applyExposureGamma(const Vec3& radiance, float exposure, float gamma) const noexcept;

  [[nodiscard]] char map(float luminance) const noexcept;
};

} // namespace astraglyph
