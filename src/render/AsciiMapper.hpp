#pragma once

#include "math/Vec3.hpp"
#include "render/AsciiFramebuffer.hpp"
#include "render/RenderSettings.hpp"

namespace astraglyph {

class AsciiMapper {
public:
  [[nodiscard]] char mapLuminanceToGlyph(float luminance, GlyphRampMode mode = GlyphRampMode::Classic) const noexcept;
  [[nodiscard]] float radianceToLuminance(const Vec3& radiance) const noexcept;
  [[nodiscard]] Vec3 applyExposureGamma(const Vec3& radiance, float exposure, float gamma) const noexcept;

  [[nodiscard]] char map(float luminance) const noexcept;

  // Phase 3: shape-aware glyph selection
  [[nodiscard]] char mapShapeAwareGlyph(
      float luminance,
      float gradientStrength,
      float gradientDirection,
      GlyphRampMode mode = GlyphRampMode::Classic) const noexcept;

  // Sobel gradient computation (strength in [0,∞), direction in radians)
  static void computeGradient(
      const AsciiFramebuffer& framebuffer,
      int x,
      int y,
      float& outStrength,
      float& outDirection) noexcept;

  // Edge enhancement pass
  static void enhanceEdges(AsciiFramebuffer& framebuffer) noexcept;
};

} // namespace astraglyph
