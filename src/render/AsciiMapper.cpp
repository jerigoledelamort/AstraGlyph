#include "render/AsciiMapper.hpp"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <limits>
#include <string_view>

namespace astraglyph {
namespace {

constexpr std::string_view kClassicRamp = "@#$S%?&*iI:-. ";

constexpr std::string_view kHorizontalRamp = "#=-:. ";
constexpr std::string_view kVerticalRamp   = "#|!:. ";
constexpr std::string_view kDiagonalRamp   = "#/\\:. ";

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

[[nodiscard]] float sampleLuminance(const AsciiFramebuffer& fb, int x, int y) noexcept
{
  if (x < 0 || y < 0 || x >= static_cast<int>(fb.width()) || y >= static_cast<int>(fb.height())) {
    return 0.0F;
  }
  return fb.at(static_cast<std::size_t>(x), static_cast<std::size_t>(y)).luminance;
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

void AsciiMapper::computeGradient(
    const AsciiFramebuffer& framebuffer,
    int x,
    int y,
    float& outStrength,
    float& outDirection) noexcept
{
  // Sobel 3x3 on luminance
  const float l00 = sampleLuminance(framebuffer, x - 1, y - 1);
  const float l10 = sampleLuminance(framebuffer, x,     y - 1);
  const float l20 = sampleLuminance(framebuffer, x + 1, y - 1);
  const float l01 = sampleLuminance(framebuffer, x - 1, y);
  const float l21 = sampleLuminance(framebuffer, x + 1, y);
  const float l02 = sampleLuminance(framebuffer, x - 1, y + 1);
  const float l12 = sampleLuminance(framebuffer, x,     y + 1);
  const float l22 = sampleLuminance(framebuffer, x + 1, y + 1);

  const float gx = (-1.0F * l00 + 1.0F * l20) +
                   (-2.0F * l01 + 2.0F * l21) +
                   (-1.0F * l02 + 1.0F * l22);

  const float gy = (-1.0F * l00 + -2.0F * l10 + -1.0F * l20) +
                   ( 1.0F * l02 +  2.0F * l12 +  1.0F * l22);

  outStrength = std::sqrt(gx * gx + gy * gy);
  outDirection = std::atan2(gy, gx);
}

char AsciiMapper::mapShapeAwareGlyph(
    float luminance,
    float gradientStrength,
    float gradientDirection,
    GlyphRampMode mode) const noexcept
{
  constexpr float kThreshold = 0.05F;
  if (gradientStrength < kThreshold || mode == GlyphRampMode::Filled) {
    return mapLuminanceToGlyph(luminance, mode);
  }

  const float blend = clamp01((gradientStrength - kThreshold) / 0.25F);
  if (blend < 0.5F) {
    return mapLuminanceToGlyph(luminance, mode);
  }

  const float gx = std::cos(gradientDirection) * gradientStrength;
  const float gy = std::sin(gradientDirection) * gradientStrength;

  std::string_view ramp;
  const float absGx = std::abs(gx);
  const float absGy = std::abs(gy);
  if (absGx > absGy * 1.5F) {
    ramp = kHorizontalRamp;
  } else if (absGy > absGx * 1.5F) {
    ramp = kVerticalRamp;
  } else {
    ramp = kDiagonalRamp;
  }

  const float clamped = clamp01(luminance);
  const float inverted = 1.0F - clamped;
  const auto index = static_cast<std::size_t>(inverted * static_cast<float>(ramp.size() - 1U));
  return ramp[index];
}

void AsciiMapper::enhanceEdges(AsciiFramebuffer& framebuffer) noexcept
{
  const int w = static_cast<int>(framebuffer.width());
  const int h = static_cast<int>(framebuffer.height());
  if (w <= 2 || h <= 2) {
    return;
  }

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      AsciiCell& cell = framebuffer.at(static_cast<std::size_t>(x), static_cast<std::size_t>(y));

      float strength = 0.0F;
      float direction = 0.0F;
      computeGradient(framebuffer, x, y, strength, direction);

      constexpr float kEdgeThreshold = 0.08F;
      if (strength < kEdgeThreshold) {
        continue;
      }

      // Unsharp mask: sharpen luminance against neighbour average
      float neighbourLuma = 0.0F;
      int count = 0;
      for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
          if (dx == 0 && dy == 0) {
            continue;
          }
          const int nx = x + dx;
          const int ny = y + dy;
          if (nx >= 0 && ny >= 0 && nx < w && ny < h) {
            neighbourLuma += framebuffer.at(static_cast<std::size_t>(nx), static_cast<std::size_t>(ny)).luminance;
            ++count;
          }
        }
      }
      if (count == 0) {
        continue;
      }

      const float avgLuma = neighbourLuma / static_cast<float>(count);
      const float diff = cell.luminance - avgLuma;
      const float sharpenAmount = strength * 2.0F;
      cell.luminance = clamp01(cell.luminance + diff * sharpenAmount);

      // Boost foreground colour slightly on edges
      const float colourBoost = 1.0F + strength * 0.5F;
      cell.fg = Vec3{
          clamp01(cell.fg.x * colourBoost),
          clamp01(cell.fg.y * colourBoost),
          clamp01(cell.fg.z * colourBoost)};
    }
  }
}

} // namespace astraglyph
