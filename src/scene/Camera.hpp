#pragma once

#include "math/Ray.hpp"
#include "math/Vec2.hpp"
#include "math/Vec3.hpp"
#include "platform/Input.hpp"

namespace astraglyph {

class Camera {
public:
  [[nodiscard]] const Vec3& position() const noexcept;
  [[nodiscard]] float yaw() const noexcept;
  [[nodiscard]] float pitch() const noexcept;
  [[nodiscard]] float fovY() const noexcept;
  [[nodiscard]] float aspect() const noexcept;

  void setAspect(float value) noexcept;

  [[nodiscard]] Vec3 getForward() const noexcept;
  [[nodiscard]] Vec3 getRight() const noexcept;
  [[nodiscard]] Vec3 getUp() const noexcept;

  void updateFromInput(const Input& input, float dt) noexcept;
  [[nodiscard]] Ray generateRay(float u, float v) const noexcept;
  [[nodiscard]] Ray makeRay(Vec2 uv) const noexcept;

private:
  Vec3 position_{0.0F, 0.0F, 3.0F};
  float yaw_{-1.5707963F};
  float pitch_{0.0F};
  float fovY_{1.0471976F};
  float aspect_{16.0F / 9.0F};
  float movementSpeed_{4.0F};
  float mouseSensitivity_{0.0025F};
};

} // namespace astraglyph
