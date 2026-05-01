#pragma once

#include "math/Ray.hpp"
#include "math/Vec2.hpp"
#include "math/Vec3.hpp"
#include "platform/Input.hpp"

namespace astraglyph {

struct CameraState {
  Vec3 position{0.0F, 0.0F, 3.0F};
  float yaw{-1.5707963F};
  float pitch{0.0F};
  float fovY{1.0471976F};
  float aspect{16.0F / 9.0F};

  [[nodiscard]] bool operator==(const CameraState& other) const noexcept = default;
  [[nodiscard]] bool operator!=(const CameraState& other) const noexcept = default;
};

class Camera {
public:
  [[nodiscard]] const Vec3& position() const noexcept;
  [[nodiscard]] float yaw() const noexcept;
  [[nodiscard]] float pitch() const noexcept;
  [[nodiscard]] float fovY() const noexcept;
  [[nodiscard]] float aspect() const noexcept;

  void setAspect(float value) noexcept;
  void setPosition(Vec3 value) noexcept;
  void setYaw(float value) noexcept;
  void setPitch(float value) noexcept;
  void setFovY(float value) noexcept;

  [[nodiscard]] Vec3 getForward() const noexcept;
  [[nodiscard]] Vec3 getRight() const noexcept;
  [[nodiscard]] Vec3 getUp() const noexcept;

  void updateFromInput(const Input& input, float dt) noexcept;
  void recomputeBasis() noexcept;
  [[nodiscard]] Ray generateRay(float u, float v) const noexcept;
  [[nodiscard]] Ray makeRay(Vec2 uv) const noexcept;

  [[nodiscard]] CameraState state() const noexcept;

private:
  Vec3 position_{0.0F, 0.0F, 3.0F};
  Vec3 velocity_{};
  float yaw_{-1.5707963F};
  float pitch_{0.0F};
  float fovY_{1.0471976F};
  float aspect_{16.0F / 9.0F};
  float movementSpeed_{3.5F};
  float mouseSensitivity_{0.0025F};
  float acceleration_{8.0F};
  float friction_{10.0F};

  Vec3 forward_{};
  Vec3 right_{};
  Vec3 up_{};
  float tanHalfFov_{};
};

} // namespace astraglyph
