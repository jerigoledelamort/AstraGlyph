#pragma once

#include "math/Vec2.hpp"

#include <array>
#include <cstddef>

union SDL_Event;

namespace astraglyph {

class Window;

enum class Key : std::size_t {
  W = 0,
  A,
  S,
  D,
  Digit1,
  Digit2,
  Digit3,
  Digit4,
  Digit5,
  LeftBracket,
  RightBracket,
  Minus,
  Equals,
  LeftShift,
  Space,
  LeftCtrl,
  Escape,
  F1,
  F2,
  Count
};

enum class MouseButton : std::size_t {
  Left = 0,
  Count
};

class Input {
public:
  void beginFrame() noexcept;
  void pollEvents(Window& window);
  void processEvent(const ::SDL_Event& event, Window& window);

  [[nodiscard]] bool isKeyDown(Key key) const noexcept;
  [[nodiscard]] bool wasKeyPressed(Key key) const noexcept;

  [[nodiscard]] bool isMouseButtonDown(MouseButton button) const noexcept;
  [[nodiscard]] bool wasMouseButtonPressed(MouseButton button) const noexcept;
  [[nodiscard]] bool wasMouseButtonReleased(MouseButton button) const noexcept;

  [[nodiscard]] Vec2 mouseDelta() const noexcept;
  [[nodiscard]] bool isMouseCaptured() const noexcept;

private:
  static constexpr std::size_t KeyCount = static_cast<std::size_t>(Key::Count);
  static constexpr std::size_t MouseButtonCount = static_cast<std::size_t>(MouseButton::Count);

  static constexpr std::size_t keyIndex(Key key) noexcept
  {
    return static_cast<std::size_t>(key);
  }

  static constexpr std::size_t mouseButtonIndex(MouseButton button) noexcept
  {
    return static_cast<std::size_t>(button);
  }

  void setKeyState(Key key, bool down) noexcept;
  void setMouseButtonState(MouseButton button, bool down) noexcept;
  void setMouseCaptured(Window& window, bool captured) noexcept;

  std::array<bool, KeyCount> keyDown_{};
  std::array<bool, KeyCount> keyPressed_{};
  std::array<bool, KeyCount> keyReleased_{};

  std::array<bool, MouseButtonCount> mouseButtonDown_{};
  std::array<bool, MouseButtonCount> mouseButtonPressed_{};
  std::array<bool, MouseButtonCount> mouseButtonReleased_{};

  Vec2 mouseDelta_{};
  bool mouseCaptured_{false};
};

} // namespace astraglyph
