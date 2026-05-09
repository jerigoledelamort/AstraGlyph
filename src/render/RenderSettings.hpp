#pragma once

#include "math/Vec3.hpp"

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace astraglyph {

enum class GlyphRampMode : std::uint8_t {
  Classic = 0,
  Filled,
};

enum class DebugViewMode : std::uint8_t {
  Off = 0,
  Albedo,
  Normals,
  Depth,
  Lighting,
};

struct RenderSettings {
  static constexpr std::uint32_t DirtyNone = 0U;
  static constexpr std::uint32_t DirtyPresentation = 1U << 0U;
  static constexpr std::uint32_t DirtyAccumulation = 1U << 1U;
  static constexpr std::uint32_t DirtyAcceleration = 1U << 2U;

  int gridWidth{160};
  int gridHeight{90};
  int samplesPerCell{4};
  int maxSamplesPerCell{16};
  bool jitteredSampling{true};
  bool adaptiveSampling{true};
  float adaptiveVarianceThreshold{0.02F};
  bool temporalAccumulation{true};
  bool enableShadows{true};
  bool enableSoftShadows{true};
  int shadowSamples{4};
  bool enableReflections{true};
  int maxBounces{2};
  bool enableBvh{true};
  bool enableMultithreading{true};
  int threadCount{0};
  float exposure{1.0F};
  float gamma{2.2F};
  bool colorOutput{true};
  GlyphRampMode glyphRampMode{GlyphRampMode::Classic};
  bool showFps{true};
  bool showDebugInfo{true};
  bool enableRenderProfiling{false};

  bool backfaceCulling{false};
  int bvhLeafSize{4};
  float ambientStrength{0.05F};
  Vec3 backgroundColor{0.01F, 0.01F, 0.015F};
  float shadowBias{0.001F};
  float reflectionBias{0.001F};
  std::uint32_t frameIndex{0};

  // Debug: output raw albedo texture (bypass lighting)
  bool debugAlbedoOnly{false};
  DebugViewMode debugViewMode{DebugViewMode::Off};

  std::uint64_t settingsVersion{0};
  std::uint32_t dirtyFlags{DirtyAccumulation | DirtyPresentation | DirtyAcceleration};
  bool accumulationDirty{true};

  void validate() noexcept
  {
    gridWidth = std::clamp(gridWidth, 1, 320);
    gridHeight = std::clamp(gridHeight, 1, 180);
    samplesPerCell = std::clamp(samplesPerCell, 1, 16);
    maxSamplesPerCell = std::clamp(maxSamplesPerCell, samplesPerCell, 16);
    shadowSamples = std::clamp(shadowSamples, 1, 16);
    maxBounces = std::clamp(maxBounces, 0, 8);
    bvhLeafSize = std::clamp(bvhLeafSize, 1, 16);
    threadCount = std::clamp(threadCount, 0, 64);
    adaptiveVarianceThreshold = std::clamp(adaptiveVarianceThreshold, -1.0F, 1.0F);
    exposure = std::clamp(exposure, 0.01F, 8.0F);
    gamma = std::clamp(gamma, 0.1F, 4.0F);
    ambientStrength = std::clamp(ambientStrength, 0.0F, 1.0F);
    shadowBias = std::clamp(shadowBias, 1.0e-5F, 0.1F);
    reflectionBias = std::clamp(reflectionBias, 1.0e-5F, 0.1F);
  }

  void clearDirtyState() noexcept
  {
    dirtyFlags = DirtyNone;
    accumulationDirty = false;
  }

  [[nodiscard]] bool hasDirtyFlag(std::uint32_t flag) const noexcept
  {
    return (dirtyFlags & flag) != 0U;
  }

  void toggleShowDebugInfo() noexcept
  {
    showDebugInfo = !showDebugInfo;
    markChanged(DirtyPresentation);
  }

  void toggleShowFps() noexcept
  {
    showFps = !showFps;
    markChanged(DirtyPresentation);
  }

  void toggleRenderProfiling() noexcept
  {
    enableRenderProfiling = !enableRenderProfiling;
    markChanged(DirtyPresentation);
  }

  void toggleShadows() noexcept
  {
    enableShadows = !enableShadows;
    markChanged(DirtyAccumulation);
  }

  void toggleSoftShadows() noexcept
  {
    enableSoftShadows = !enableSoftShadows;
    markChanged(DirtyAccumulation);
  }

  void toggleReflections() noexcept
  {
    enableReflections = !enableReflections;
    markChanged(DirtyAccumulation);
  }

  void toggleAdaptiveSampling() noexcept
  {
    adaptiveSampling = !adaptiveSampling;
    markChanged(DirtyAccumulation);
  }

  void toggleTemporalAccumulation() noexcept
  {
    temporalAccumulation = !temporalAccumulation;
    markChanged(DirtyAccumulation);
  }

  void toggleBvh() noexcept
  {
    enableBvh = !enableBvh;
    markChanged(DirtyAccumulation | DirtyAcceleration);
  }

  void adjustSamplesPerCell(int delta) noexcept
  {
    const int previous = samplesPerCell;
    samplesPerCell += delta;
    validate();
    if (samplesPerCell != previous) {
      markChanged(DirtyAccumulation);
    }
  }

  void adjustShadowSamples(int delta) noexcept
  {
    const int previous = shadowSamples;
    shadowSamples += delta;
    validate();
    if (shadowSamples != previous) {
      markChanged(DirtyAccumulation);
    }
  }

  void adjustMaxBounces(int delta) noexcept
  {
    const int previous = maxBounces;
    maxBounces += delta;
    validate();
    if (maxBounces != previous) {
      markChanged(DirtyAccumulation);
    }
  }

  void adjustSecondaryQuality(int delta) noexcept
  {
    if (enableSoftShadows) {
      adjustShadowSamples(delta);
      return;
    }

    adjustMaxBounces(delta);
  }

  [[nodiscard]] static std::string_view glyphRampModeName(GlyphRampMode mode) noexcept
  {
    switch (mode) {
      case GlyphRampMode::Filled:
        return "Filled";
      default:
        return "Classic";
    }
  }

  [[nodiscard]] DebugViewMode activeDebugViewMode() const noexcept
  {
    return debugAlbedoOnly ? DebugViewMode::Albedo : debugViewMode;
  }

  [[nodiscard]] bool isDebugViewEnabled() const noexcept
  {
    return activeDebugViewMode() != DebugViewMode::Off;
  }

  [[nodiscard]] static std::string_view debugViewModeName(DebugViewMode mode) noexcept
  {
    switch (mode) {
      case DebugViewMode::Albedo:
        return "Albedo";
      case DebugViewMode::Normals:
        return "Normals";
      case DebugViewMode::Depth:
        return "Depth";
      case DebugViewMode::Lighting:
        return "Lighting";
      default:
        return "Off";
    }
  }

  void cycleGlyphRampMode() noexcept
  {
    glyphRampMode = (glyphRampMode == GlyphRampMode::Classic)
                        ? GlyphRampMode::Filled
                        : GlyphRampMode::Classic;
    markChanged(DirtyPresentation);
  }

  void toggleDebugAlbedoOnly() noexcept
  {
    debugAlbedoOnly = !debugAlbedoOnly;
    markChanged(DirtyAccumulation);
  }

  void cycleDebugViewMode() noexcept
  {
    debugAlbedoOnly = false;
    switch (debugViewMode) {
      case DebugViewMode::Off:
        debugViewMode = DebugViewMode::Albedo;
        break;
      case DebugViewMode::Albedo:
        debugViewMode = DebugViewMode::Normals;
        break;
      case DebugViewMode::Normals:
        debugViewMode = DebugViewMode::Depth;
        break;
      case DebugViewMode::Depth:
        debugViewMode = DebugViewMode::Lighting;
        break;
      case DebugViewMode::Lighting:
        debugViewMode = DebugViewMode::Off;
        break;
    }
    markChanged(DirtyAccumulation);
  }

  void resetToDefaults() noexcept
  {
    *this = RenderSettings{};
    markChanged(DirtyAccumulation | DirtyPresentation | DirtyAcceleration);
  }

private:
  void markChanged(std::uint32_t flags) noexcept
  {
    validate();
    ++settingsVersion;
    dirtyFlags |= flags;
    if ((flags & DirtyAccumulation) != 0U) {
      accumulationDirty = true;
    }
  }
};

} // namespace astraglyph
