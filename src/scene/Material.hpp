#pragma once

#include "math/Vec2.hpp"
#include "math/Vec3.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace astraglyph {

struct Material {
  Vec3 albedo{1.0F, 1.0F, 1.0F};
  float roughness{0.5F};
  float metallic{0.0F};
  float reflectivity{0.0F};
  Vec3 emissionColor{0.0F, 0.0F, 0.0F};
  float emissionStrength{0.0F};

  // Texture paths (loaded from MTL files)
  std::string albedoTexturePath;
  std::string metallicTexturePath;
  std::string roughnessTexturePath;

  // Albedo texture data (loaded into memory)
  std::vector<unsigned char> albedoTextureData;
  int albedoTextureWidth{0};
  int albedoTextureHeight{0};
  int albedoTextureChannels{0};
  bool hasAlbedoTexture{false};

  // Metallic texture data (loaded into memory)
  std::vector<unsigned char> metallicTextureData;
  int metallicTextureWidth{0};
  int metallicTextureHeight{0};
  int metallicTextureChannels{0};
  bool hasMetallicTexture{false};

  // Roughness texture data (loaded into memory)
  std::vector<unsigned char> roughnessTextureData;
  int roughnessTextureWidth{0};
  int roughnessTextureHeight{0};
  int roughnessTextureChannels{0};
  bool hasRoughnessTexture{false};
};

// Sample albedo texture at given UV coordinates
// Returns texture color if texture is available, otherwise returns material.albedo
// Note: Returns sRGB color (not converted to linear space)
[[nodiscard]] inline Vec3 sampleAlbedoTexture(const Material& mat, const Vec2& uv)
{
  if (!mat.hasAlbedoTexture || mat.albedoTextureData.empty() || mat.albedoTextureWidth <= 0 || mat.albedoTextureHeight <= 0) {
    // DEBUG: albedo fallback
    return Vec3{mat.albedo.x, mat.albedo.y, mat.albedo.z};
  }

  // Clamp UV to [0, 1] range
  const float clampedU = std::clamp(uv.x, 0.0F, 1.0F);
  const float clampedV = std::clamp(uv.y, 0.0F, 1.0F);

  // Calculate pixel coordinates - flip V for texture coordinate system
  const int x = std::clamp(static_cast<int>(clampedU * static_cast<float>(mat.albedoTextureWidth)), 0, mat.albedoTextureWidth - 1);
  const int y = std::clamp(static_cast<int>((1.0F - clampedV) * static_cast<float>(mat.albedoTextureHeight)), 0, mat.albedoTextureHeight - 1);

  // Handle different channel counts (3 = RGB, 4 = RGBA)
  const int channels = std::max(mat.albedoTextureChannels, 3);
  const int idx = (y * mat.albedoTextureWidth + x) * channels;

  // Ensure we don't read out of bounds
  if (idx + 2 >= static_cast<int>(mat.albedoTextureData.size())) {
    return mat.albedo;
  }

  // Convert from [0, 255] to [0, 1] - return sRGB value directly
  const float r = static_cast<float>(mat.albedoTextureData[idx]) / 255.0F;
  const float g = static_cast<float>(mat.albedoTextureData[idx + 1]) / 255.0F;
  const float b = static_cast<float>(mat.albedoTextureData[idx + 2]) / 255.0F;

  return Vec3{r, g, b};
}

// Sample albedo texture for shading (returns linear color)
// Converts sRGB texture to linear space for correct lighting calculations
[[nodiscard]] inline Vec3 sampleAlbedoTextureLinear(const Material& mat, const Vec2& uv)
{
  const Vec3 srgb = sampleAlbedoTexture(mat, uv);
  
  // Convert from sRGB to linear space (standard sRGB formula)
  const auto srgbToLinear = [](float value) noexcept {
    const float v = std::clamp(value, 0.0F, 1.0F);
    return (v <= 0.04045F) ? (v / 12.92F) : (std::pow((v + 0.055F) / 1.055F, 2.4F));
  };

  return Vec3{srgbToLinear(srgb.x), srgbToLinear(srgb.y), srgbToLinear(srgb.z)};
}

// Sample metallic texture at given UV coordinates
// Returns texture value if texture is available, otherwise returns material.metallic
[[nodiscard]] inline float sampleMetallicTexture(const Material& mat, const Vec2& uv)
{
  if (!mat.hasMetallicTexture || mat.metallicTextureData.empty() ||
      mat.metallicTextureWidth <= 0 || mat.metallicTextureHeight <= 0) {
    return mat.metallic;
  }

  // Clamp UV to [0, 1] range
  const float clampedU = std::clamp(uv.x, 0.0F, 1.0F);
  const float clampedV = std::clamp(uv.y, 0.0F, 1.0F);

  // Calculate pixel coordinates - flip V for texture coordinate system
  const int x = std::clamp(static_cast<int>(clampedU * static_cast<float>(mat.metallicTextureWidth)), 0, mat.metallicTextureWidth - 1);
  const int y = std::clamp(static_cast<int>((1.0F - clampedV) * static_cast<float>(mat.metallicTextureHeight)), 0, mat.metallicTextureHeight - 1);

  // Use first channel (grayscale)
  const int channels = std::max(mat.metallicTextureChannels, 1);
  const int idx = (y * mat.metallicTextureWidth + x) * channels;

  // Ensure we don't read out of bounds
  if (idx >= static_cast<int>(mat.metallicTextureData.size())) {
    return mat.metallic;
  }

  // Convert from [0, 255] to [0, 1]
  return static_cast<float>(mat.metallicTextureData[idx]) / 255.0F;
}

// Sample roughness texture at given UV coordinates
// Returns texture value if texture is available, otherwise returns material.roughness
[[nodiscard]] inline float sampleRoughnessTexture(const Material& mat, const Vec2& uv)
{
  if (!mat.hasRoughnessTexture || mat.roughnessTextureData.empty() ||
      mat.roughnessTextureWidth <= 0 || mat.roughnessTextureHeight <= 0) {
    return mat.roughness;
  }

  // Clamp UV to [0, 1] range
  const float clampedU = std::clamp(uv.x, 0.0F, 1.0F);
  const float clampedV = std::clamp(uv.y, 0.0F, 1.0F);

  // Calculate pixel coordinates - flip V for texture coordinate system
  const int x = std::clamp(static_cast<int>(clampedU * static_cast<float>(mat.roughnessTextureWidth)), 0, mat.roughnessTextureWidth - 1);
  const int y = std::clamp(static_cast<int>((1.0F - clampedV) * static_cast<float>(mat.roughnessTextureHeight)), 0, mat.roughnessTextureHeight - 1);

  // Use first channel (grayscale)
  const int channels = std::max(mat.roughnessTextureChannels, 1);
  const int idx = (y * mat.roughnessTextureWidth + x) * channels;

  // Ensure we don't read out of bounds
  if (idx >= static_cast<int>(mat.roughnessTextureData.size())) {
    return mat.roughness;
  }

  // Convert from [0, 255] to [0, 1]
  return static_cast<float>(mat.roughnessTextureData[idx]) / 255.0F;
}

} // namespace astraglyph
