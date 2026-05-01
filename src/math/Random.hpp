#pragma once

#include "math/Vec2.hpp"

#include <cstdint>

namespace astraglyph {

class Random {
public:
  explicit constexpr Random(std::uint32_t seed = 1U) noexcept
      : state_{seed}
  {
  }

  [[nodiscard]] std::uint32_t nextU32() noexcept
  {
    // xorshift32: deterministic and lightweight for sampling utilities.
    std::uint32_t value = state_;
    value ^= value << 13U;
    value ^= value >> 17U;
    value ^= value << 5U;
    state_ = value;
    return state_;
  }

  [[nodiscard]] float nextFloat01() noexcept
  {
    constexpr float scale = 1.0F / 16777216.0F;
    return static_cast<float>(nextU32() >> 8U) * scale;
  }

  [[nodiscard]] Vec2 nextVec2() noexcept
  {
    return {nextFloat01(), nextFloat01()};
  }

  [[nodiscard]] float next01() noexcept
  {
    return nextFloat01();
  }

private:
  std::uint32_t state_{1U};
};

} // namespace astraglyph
