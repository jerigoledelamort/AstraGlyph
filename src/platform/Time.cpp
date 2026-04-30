#include "platform/Time.hpp"

namespace astraglyph {

void Time::update()
{
  const Clock::time_point now = Clock::now();
  deltaTime_ = std::chrono::duration<double>(now - lastFrameTime_).count();
  lastFrameTime_ = now;

  elapsedTime_ = std::chrono::duration<double>(now - startTime_).count();
  ++frameCount_;

  fpsAccumulatedTime_ += deltaTime_;
  ++fpsAccumulatedFrames_;

  if (fpsAccumulatedTime_ >= 0.5) {
    fps_ = static_cast<double>(fpsAccumulatedFrames_) / fpsAccumulatedTime_;
    fpsAccumulatedTime_ = 0.0;
    fpsAccumulatedFrames_ = 0U;
  }
}

double Time::deltaTime() const noexcept
{
  return deltaTime_;
}

double Time::elapsedTime() const noexcept
{
  return elapsedTime_;
}

std::uint64_t Time::frameCount() const noexcept
{
  return frameCount_;
}

double Time::fps() const noexcept
{
  return fps_;
}

} // namespace astraglyph
