#pragma once

#include "math/Vec2.hpp"

#include <array>
#include <cstddef>

union SDL_Event;

namespace astraglyph {

class Window;

struct MouseState {
  int x{0};
  int y{0};
  int deltaX{0};
  int deltaY{0};
  bool leftButton{false};
  bool rightButton{false};
  bool middleButton{false};
  int wheelDelta{0};
};

enum class Key : std::size_t {
  W = 0,
  A,
  S,
  D,
  T,
  Digit1,
  Digit2,
  Digit3,
  Digit4,
  Digit5,
  Digit6,
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
  F3,
  F,
  Count
};

enum class MouseButton : std::size_t {
  Left = 0,
  Right,
  Middle,
  Count
};

class Input {
public:
  virtual ~Input() = default;

  virtual void beginFrame() noexcept;
  virtual void pollEvents(Window& window);
  void processEvent(const ::SDL_Event& event, Window& window);

  [[nodiscard]] virtual bool isKeyDown(Key key) const noexcept;
  [[nodiscard]] virtual bool wasKeyPressed(Key key) const noexcept;

  [[nodiscard]] virtual bool isMouseButtonDown(MouseButton button) const noexcept;
  [[nodiscard]] virtual bool wasMouseButtonPressed(MouseButton button) const noexcept;
  [[nodiscard]] virtual bool wasMouseButtonReleased(MouseButton button) const noexcept;

  [[nodiscard]] virtual Vec2 mouseDelta() const noexcept;
  [[nodiscard]] virtual bool isMouseCaptured() const noexcept;

  [[nodiscard]] MouseState mouseState() const noexcept;
  void setMouseInPanel(bool inPanel) noexcept;
  void applyMouseCapture(Window& window) noexcept;

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

  MouseState mouseState_{};
  int prevMouseX_{0};
  int prevMouseY_{0};
  bool mouseInPanel_{false};
};

} // namespace astraglyph
