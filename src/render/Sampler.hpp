#pragma once

#include "math/Vec2.hpp"
#include "render/RenderSettings.hpp"

#include <cstdint>

namespace astraglyph {

struct Sample {
  Vec2 uv{0.5F, 0.5F};
};

class Sampler {
public:
  [[nodiscard]] Sample next() const noexcept;
  [[nodiscard]] std::uint32_t makeSeed(
      std::uint32_t frameIndex,
      std::uint32_t value0,
      std::uint32_t value1,
      std::uint32_t value2 = 0U) const noexcept;
  [[nodiscard]] Vec2 sample2D(
      std::uint32_t seed,
      int sampleIndex,
      int sampleCount,
      bool jitteredSampling = true) const noexcept;
  [[nodiscard]] Sample generateSubCellSample(
      int cellX,
      int cellY,
      int sampleIndex,
      const RenderSettings& settings) const noexcept;
};

} // namespace astraglyph
