#pragma once

#include <chrono>
#include <cstdint>

namespace astraglyph {

class Time {
public:
  void update();

  [[nodiscard]] double deltaTime() const noexcept;
  [[nodiscard]] double elapsedTime() const noexcept;
  [[nodiscard]] std::uint64_t frameCount() const noexcept;
  [[nodiscard]] double fps() const noexcept;

private:
  using Clock = std::chrono::steady_clock;

  Clock::time_point startTime_{Clock::now()};
  Clock::time_point lastFrameTime_{startTime_};
  double deltaTime_{0.0};
  double elapsedTime_{0.0};
  std::uint64_t frameCount_{0U};

  double fps_{0.0};
  double fpsAccumulatedTime_{0.0};
  std::uint32_t fpsAccumulatedFrames_{0U};
};

} // namespace astraglyph
