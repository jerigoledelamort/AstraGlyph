#include "render/Sampler.hpp"

#include "render/BlueNoiseData.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace astraglyph {
namespace {

struct Stratum {
  int x;
  int y;
  int columns;
  int rows;
};

constexpr std::array<Stratum, 2> kDiagonalPair{{
    {0, 0, 2, 2},
    {1, 1, 2, 2},
}};

constexpr std::array<Stratum, 4> kGrid2x2{{
    {0, 0, 2, 2},
    {1, 0, 2, 2},
    {0, 1, 2, 2},
    {1, 1, 2, 2},
}};

constexpr std::array<Stratum, 8> kStratified8{{
    {0, 0, 4, 4},
    {2, 0, 4, 4},
    {1, 1, 4, 4},
    {3, 1, 4, 4},
    {0, 2, 4, 4},
    {2, 2, 4, 4},
    {1, 3, 4, 4},
    {3, 3, 4, 4},
}};

constexpr std::array<Stratum, 16> kStratified16{{
    {0, 0, 4, 4},
    {2, 0, 4, 4},
    {1, 1, 4, 4},
    {3, 1, 4, 4},
    {0, 2, 4, 4},
    {2, 2, 4, 4},
    {1, 3, 4, 4},
    {3, 3, 4, 4},
    {1, 0, 4, 4},
    {3, 0, 4, 4},
    {0, 1, 4, 4},
    {2, 1, 4, 4},
    {1, 2, 4, 4},
    {3, 2, 4, 4},
    {0, 3, 4, 4},
    {2, 3, 4, 4},
}};

constexpr float kMaxOffset = 0.99999994F;

[[nodiscard]] std::uint32_t mixBits(std::uint32_t value) noexcept
{
  value ^= value >> 16U;
  value *= 0x7FEB352DU;
  value ^= value >> 15U;
  value *= 0x846CA68BU;
  value ^= value >> 16U;
  return value;
}

[[nodiscard]] int clampSampleCount(int sampleCount) noexcept
{
  return std::clamp(sampleCount, 1, 16);
}

[[nodiscard]] int resolvePatternSampleCount(int sampleCount) noexcept
{
  const int clamped = std::max(sampleCount, 1);
  if (clamped <= 1) {
    return 1;
  }
  if (clamped <= 2) {
    return 2;
  }
  if (clamped <= 4) {
    return 4;
  }
  if (clamped <= 8) {
    return 8;
  }
  return 16;
}

[[nodiscard]] std::uint32_t buildSeed(
    std::uint32_t frameIndex,
    std::uint32_t value0,
    std::uint32_t value1,
    std::uint32_t value2) noexcept
{
  std::uint32_t seed = 0x9E3779B9U;
  seed ^= mixBits(frameIndex + 0x85EBCA6BU);
  seed ^= mixBits(value0 + 0xC2B2AE35U);
  seed ^= mixBits(value1 + 0x27D4EB2FU);
  seed ^= mixBits(value2 + 0x165667B1U);
  return mixBits(seed);
}

[[nodiscard]] float randomFloat01(std::uint32_t state) noexcept
{
  constexpr float kInv24BitRange = 1.0F / 16777216.0F;
  const std::uint32_t bits = mixBits(state) >> 8U;
  return (static_cast<float>(bits) + 0.5F) * kInv24BitRange;
}

[[nodiscard]] Vec2 sampleStratum(
    const Stratum& stratum,
    std::uint32_t seed,
    bool jitteredSampling) noexcept
{
  const float jitterX = jitteredSampling ? randomFloat01(seed ^ 0xA511E9B3U) : 0.5F;
  const float jitterY = jitteredSampling ? randomFloat01(seed ^ 0x63D83595U) : 0.5F;

  const float offsetX =
      (static_cast<float>(stratum.x) + jitterX) / static_cast<float>(stratum.columns);
  const float offsetY =
      (static_cast<float>(stratum.y) + jitterY) / static_cast<float>(stratum.rows);

  return {
      std::clamp(offsetX, 0.0F, kMaxOffset),
      std::clamp(offsetY, 0.0F, kMaxOffset),
  };
}

[[nodiscard]] Vec2 sampleGenericGrid(
    int sampleIndex,
    int sampleCount,
    std::uint32_t seed,
    bool jitteredSampling) noexcept
{
  const int clampedSamples = std::max(sampleCount, 1);
  const int clampedIndex = std::clamp(sampleIndex, 0, clampedSamples - 1);
  const int columns = std::max(1, static_cast<int>(std::ceil(std::sqrt(static_cast<float>(clampedSamples)))));
  const int rows = std::max(1, (clampedSamples + columns - 1) / columns);
  const Stratum stratum{
      clampedIndex % columns,
      clampedIndex / columns,
      columns,
      rows,
  };
  return sampleStratum(stratum, seed, jitteredSampling);
}

template <std::size_t Size>
[[nodiscard]] Sample sampleFromPattern(
    const std::array<Stratum, Size>& pattern,
    int sampleIndex,
    std::uint32_t seed,
    bool jitteredSampling) noexcept
{
  const std::size_t index = static_cast<std::size_t>(
      std::clamp(sampleIndex, 0, static_cast<int>(pattern.size()) - 1));
  return {sampleStratum(pattern[index], seed, jitteredSampling)};
}

} // namespace

Sample Sampler::next() const noexcept
{
  return {};
}

std::uint32_t Sampler::makeSeed(
    std::uint32_t frameIndex,
    std::uint32_t value0,
    std::uint32_t value1,
    std::uint32_t value2) const noexcept
{
  return buildSeed(frameIndex, value0, value1, value2);
}

Vec2 Sampler::sample2D(
    std::uint32_t seed,
    int sampleIndex,
    int sampleCount,
    bool jitteredSampling) const noexcept
{
  const std::uint32_t sampleSeed = buildSeed(
      seed,
      static_cast<std::uint32_t>(std::max(sampleIndex, 0)),
      static_cast<std::uint32_t>(std::max(sampleCount, 1)),
      0U);

  if (sampleCount > 16) {
    return sampleGenericGrid(sampleIndex, sampleCount, sampleSeed, jitteredSampling);
  }

  switch (resolvePatternSampleCount(sampleCount)) {
  case 2:
    return sampleFromPattern(kDiagonalPair, sampleIndex, sampleSeed, jitteredSampling).uv;
  case 4:
    return sampleFromPattern(kGrid2x2, sampleIndex, sampleSeed, jitteredSampling).uv;
  case 8:
    return sampleFromPattern(kStratified8, sampleIndex, sampleSeed, jitteredSampling).uv;
  case 16:
    return sampleFromPattern(kStratified16, sampleIndex, sampleSeed, jitteredSampling).uv;
  case 1:
  default:
    return {0.5F, 0.5F};
  }
}

Sample Sampler::generateBlueNoiseSample(
    int cellX,
    int cellY,
    int sampleIndex,
    std::uint32_t frameIndex,
    int gridWidth,
    int gridHeight) const noexcept
{
  const int scaleX = std::max(gridWidth, 1);
  const int scaleY = std::max(gridHeight, 1);
  const int bx = ((cellX * 64) / scaleX + sampleIndex + static_cast<int>(frameIndex & 63U)) & 63;
  const int by = ((cellY * 64) / scaleY + (sampleIndex >> 6) + static_cast<int>((frameIndex >> 6) & 63U)) & 63;
  return {kBlueNoise[by][bx]};
}

Sample Sampler::generateSubCellSample(
    int cellX,
    int cellY,
    int sampleIndex,
    const RenderSettings& settings) const noexcept
{
  if (sampleIndex <= 0 && settings.samplesPerCell <= 1) {
    return {};
  }

  if (settings.jitteredSampling) {
    return generateBlueNoiseSample(
        cellX, cellY, sampleIndex, settings.frameIndex, settings.gridWidth, settings.gridHeight);
  }

  const int maxSamples = clampSampleCount(std::max(settings.samplesPerCell, settings.maxSamplesPerCell));
  const int requestedSamples = std::clamp(
      std::max(settings.samplesPerCell, sampleIndex + 1),
      1,
      maxSamples);
  const std::uint32_t seed = makeSeed(
      settings.frameIndex,
      static_cast<std::uint32_t>(cellX),
      static_cast<std::uint32_t>(cellY),
      0U);
  return {sample2D(seed, sampleIndex, requestedSamples, false)};
}

} // namespace astraglyph
