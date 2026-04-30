#include "scene/Camera.hpp"

#include <algorithm>
#include <cmath>

namespace astraglyph {
namespace {

constexpr float kMaxPitch = 1.5533430F;

} // namespace

const Vec3& Camera::position() const noexcept
{
  return position_;
}

float Camera::yaw() const noexcept
{
  return yaw_;
}

float Camera::pitch() const noexcept
{
  return pitch_;
}

float Camera::fovY() const noexcept
{
  return fovY_;
}

float Camera::aspect() const noexcept
{
  return aspect_;
}

void Camera::setAspect(float value) noexcept
{
  if (value > 0.0F) {
    aspect_ = value;
  }
}

Vec3 Camera::getForward() const noexcept
{
  const float cosPitch = std::cos(pitch_);
  return normalize(Vec3{
      std::cos(yaw_) * cosPitch,
      std::sin(pitch_),
      std::sin(yaw_) * cosPitch,
  });
}

Vec3 Camera::getRight() const noexcept
{
  return normalize(cross(getForward(), Vec3{0.0F, 1.0F, 0.0F}));
}

Vec3 Camera::getUp() const noexcept
{
  return normalize(cross(getRight(), getForward()));
}

void Camera::updateFromInput(const Input& input, float dt) noexcept
{
  const float distance = movementSpeed_ * dt;
  const Vec3 forward = getForward();
  const Vec3 right = getRight();
  const Vec3 worldUp{0.0F, 1.0F, 0.0F};

  if (input.isKeyDown(Key::W)) {
    position_ += forward * distance;
  }
  if (input.isKeyDown(Key::S)) {
    position_ -= forward * distance;
  }
  if (input.isKeyDown(Key::D)) {
    position_ += right * distance;
  }
  if (input.isKeyDown(Key::A)) {
    position_ -= right * distance;
  }
  if (input.isKeyDown(Key::LeftShift)) {
    position_ += worldUp * distance;
  }
  if (input.isKeyDown(Key::LeftCtrl)) {
    position_ -= worldUp * distance;
  }

  if (input.isMouseCaptured()) {
    const Vec2 delta = input.mouseDelta();
    yaw_ += delta.x * mouseSensitivity_;
    pitch_ -= delta.y * mouseSensitivity_;
    pitch_ = std::clamp(pitch_, -kMaxPitch, kMaxPitch);
  }
}

Ray Camera::generateRay(float u, float v) const noexcept
{
  const float tanHalfFov = std::tan(fovY_ * 0.5F);
  const float screenX = (2.0F * u - 1.0F) * aspect_ * tanHalfFov;
  const float screenY = (1.0F - 2.0F * v) * tanHalfFov;

  const Vec3 forward = getForward();
  const Vec3 right = getRight();
  const Vec3 up = getUp();
  const Vec3 direction = normalize(forward + right * screenX + up * screenY);

  return Ray{position_, direction};
}

Ray Camera::makeRay(Vec2 uv) const noexcept
{
  return generateRay(uv.x, uv.y);
}

} // namespace astraglyph
