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

void Camera::setPosition(Vec3 value) noexcept
{
  position_ = value;
}

void Camera::setYaw(float value) noexcept
{
  yaw_ = value;
}

void Camera::setPitch(float value) noexcept
{
  pitch_ = std::clamp(value, -kMaxPitch, kMaxPitch);
}

void Camera::setFovY(float value) noexcept
{
  if (value > 0.0F && value < 180.0F) {
    fovY_ = value * (3.14159265358979323846F / 180.0F);
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
  const Vec3 forward = getForward();
  const Vec3 right = getRight();
  const Vec3 worldUp{0.0F, 1.0F, 0.0F};

  Vec3 targetVelocity{};
  if (input.isKeyDown(Key::W)) {
    targetVelocity += forward;
  }
  if (input.isKeyDown(Key::S)) {
    targetVelocity -= forward;
  }
  if (input.isKeyDown(Key::D)) {
    targetVelocity += right;
  }
  if (input.isKeyDown(Key::A)) {
    targetVelocity -= right;
  }
  if (input.isKeyDown(Key::LeftShift)) {
    targetVelocity += worldUp;
  }
  if (input.isKeyDown(Key::LeftCtrl)) {
    targetVelocity -= worldUp;
  }

  if (targetVelocity.x != 0.0F || targetVelocity.y != 0.0F || targetVelocity.z != 0.0F) {
    targetVelocity = normalize(targetVelocity) * movementSpeed_;
  }

  const float expAccel = 1.0F - std::exp(-acceleration_ * dt);
  const float expFriction = 1.0F - std::exp(-friction_ * dt);

  if (targetVelocity.x != 0.0F || targetVelocity.y != 0.0F || targetVelocity.z != 0.0F) {
    velocity_.x += (targetVelocity.x - velocity_.x) * expAccel;
    velocity_.y += (targetVelocity.y - velocity_.y) * expAccel;
    velocity_.z += (targetVelocity.z - velocity_.z) * expAccel;
  } else {
    velocity_.x -= velocity_.x * expFriction;
    velocity_.y -= velocity_.y * expFriction;
    velocity_.z -= velocity_.z * expFriction;
    if (std::fabs(velocity_.x) < 0.001F) { velocity_.x = 0.0F; }
    if (std::fabs(velocity_.y) < 0.001F) { velocity_.y = 0.0F; }
    if (std::fabs(velocity_.z) < 0.001F) { velocity_.z = 0.0F; }
  }

  position_ += velocity_ * dt;

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

CameraState Camera::state() const noexcept
{
  return CameraState{position_, yaw_, pitch_, fovY_, aspect_};
}

} // namespace astraglyph
