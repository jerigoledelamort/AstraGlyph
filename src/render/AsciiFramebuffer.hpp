#pragma once

#include "math/Vec3.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

namespace astraglyph {

struct AsciiCell {
  char glyph{' '};
  Vec3 fg{1.0F, 1.0F, 1.0F};
  Vec3 bg{0.0F, 0.0F, 0.0F};
  float luminance{0.0F};
  float depth{std::numeric_limits<float>::infinity()};
  Vec3 radiance{};
  int sampleCount{0};

  // Temporal accumulation state (cell-level, not pixel-level)
  Vec3 accumulatedRadiance{};
  float accumulatedLuminance{0.0F};
  int accumulatedFrames{0};
};

class AsciiFramebuffer {
public:
  AsciiFramebuffer() = default;

  AsciiFramebuffer(std::size_t width, std::size_t height)
  {
    resize(width, height);
  }

  void resize(std::size_t width, std::size_t height)
  {
    width_ = width;
    height_ = height;
    cells_.assign(width_ * height_, AsciiCell{});
  }

  void clear()
  {
    std::fill(cells_.begin(), cells_.end(), AsciiCell{});
  }

  [[nodiscard]] std::size_t width() const noexcept
  {
    return width_;
  }

  [[nodiscard]] std::size_t height() const noexcept
  {
    return height_;
  }

  [[nodiscard]] AsciiCell& at(std::size_t x, std::size_t y) noexcept
  {
    return cells_[y * width_ + x];
  }

  [[nodiscard]] const AsciiCell& at(std::size_t x, std::size_t y) const noexcept
  {
    return cells_[y * width_ + x];
  }

  [[nodiscard]] std::vector<AsciiCell>& data() noexcept
  {
    return cells_;
  }

  [[nodiscard]] const std::vector<AsciiCell>& data() const noexcept
  {
    return cells_;
  }

  [[nodiscard]] std::string toString() const
  {
    std::string output;
    output.reserve(cells_.size() + height_);
    for (std::size_t y = 0; y < height_; ++y) {
      for (std::size_t x = 0; x < width_; ++x) {
        output.push_back(at(x, y).glyph);
      }
      output.push_back('\n');
    }
    return output;
  }

private:
  std::size_t width_{0};
  std::size_t height_{0};
  std::vector<AsciiCell> cells_{};
};

} // namespace astraglyph
