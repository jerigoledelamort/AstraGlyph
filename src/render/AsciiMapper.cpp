#include "render/AsciiMapper.hpp"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <string_view>

namespace astraglyph {
namespace {

constexpr std::string_view kClassicRamp = "@#$S%?&*iI:-. ";

[[nodiscard]] std::string_view rampForMode(GlyphRampMode mode) noexcept
{
  switch (mode) {
    case GlyphRampMode::Filled:
      return " ";
    default:
      return kClassicRamp;
  }
}

float clamp01(float value) noexcept
{
  return std::clamp(value, 0.0F, 1.0F);
}

} // namespace

char AsciiMapper::mapLuminanceToGlyph(float luminance, GlyphRampMode mode) const noexcept
{
  const float clamped = clamp01(luminance);
  const float inverted = 1.0F - clamped;
  const std::string_view ramp = rampForMode(mode);
  const auto index = static_cast<std::size_t>(inverted * static_cast<float>(ramp.size() - 1U));
  return ramp[index];
}

float AsciiMapper::radianceToLuminance(const Vec3& radiance) const noexcept
{
  // Rec.709 luma coefficients.
  return 0.2126F * radiance.x + 0.7152F * radiance.y + 0.0722F * radiance.z;
}

Vec3 AsciiMapper::applyExposureGamma(const Vec3& radiance, float exposure, float gamma) const noexcept
{
  const float safeExposure = exposure > 0.0F ? exposure : 1.0F;
  const float safeGamma = gamma > 0.0F ? gamma : 2.2F;
  const float invGamma = 1.0F / safeGamma;

  // Simple exposure and gamma (no tone mapping for direct sRGB mapping)
  const auto apply = [&](float channel) noexcept {
    const float withExposure = std::max(channel * safeExposure, 0.0F);
    const float clamped = std::clamp(withExposure, 0.0F, 1.0F);
    return std::pow(clamped, invGamma);
  };

  return {apply(radiance.x), apply(radiance.y), apply(radiance.z)};
}

char AsciiMapper::map(float luminance) const noexcept
{
  return mapLuminanceToGlyph(luminance);
}

} // namespace astraglyph
