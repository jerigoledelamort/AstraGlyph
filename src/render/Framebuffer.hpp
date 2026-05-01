#pragma once

#include <cstddef>

namespace astraglyph {

class Framebuffer {
public:
  constexpr Framebuffer() noexcept = default;
  constexpr Framebuffer(std::size_t columns, std::size_t rows) noexcept
      : columns_{columns}, rows_{rows}
  {
  }

  [[nodiscard]] constexpr std::size_t columns() const noexcept
  {
    return columns_;
  }

  [[nodiscard]] constexpr std::size_t rows() const noexcept
  {
    return rows_;
  }

private:
  std::size_t columns_{0};
  std::size_t rows_{0};
};

} // namespace astraglyph
