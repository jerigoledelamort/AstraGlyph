#include "platform/Input.hpp"

#include "platform/Window.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace astraglyph {
namespace {

bool tryMapKey(SDL_Keycode keycode, Key& outKey) noexcept
{
  switch (keycode) {
  case SDLK_W:
  case 'W':
    outKey = Key::W;
    return true;
  case SDLK_A:
  case 'A':
    outKey = Key::A;
    return true;
  case SDLK_S:
  case 'S':
    outKey = Key::S;
    return true;
  case SDLK_D:
  case 'D':
    outKey = Key::D;
    return true;
  case SDLK_T:
  case 'T':
    outKey = Key::T;
    return true;
  case SDLK_1:
    outKey = Key::Digit1;
    return true;
  case SDLK_2:
    outKey = Key::Digit2;
    return true;
  case SDLK_3:
    outKey = Key::Digit3;
    return true;
  case SDLK_4:
    outKey = Key::Digit4;
    return true;
  case SDLK_5:
    outKey = Key::Digit5;
    return true;
  case SDLK_6:
    outKey = Key::Digit6;
    return true;
  case SDLK_LEFTBRACKET:
    outKey = Key::LeftBracket;
    return true;
  case SDLK_RIGHTBRACKET:
    outKey = Key::RightBracket;
    return true;
  case SDLK_MINUS:
    outKey = Key::Minus;
    return true;
  case SDLK_EQUALS:
    outKey = Key::Equals;
    return true;
  case SDLK_LSHIFT:
    outKey = Key::LeftShift;
    return true;
  case SDLK_SPACE:
    outKey = Key::Space;
    return true;
  case SDLK_LCTRL:
    outKey = Key::LeftCtrl;
    return true;
  case SDLK_ESCAPE:
    outKey = Key::Escape;
    return true;
  case SDLK_F1:
    outKey = Key::F1;
    return true;
  case SDLK_F2:
    outKey = Key::F2;
    return true;
  case SDLK_F3:
    outKey = Key::F3;
    return true;
  case SDLK_F:
    outKey = Key::F;
    return true;
  default:
    return false;
  }
}

} // namespace

void Input::beginFrame() noexcept
{
  std::fill(keyPressed_.begin(), keyPressed_.end(), false);
  std::fill(keyReleased_.begin(), keyReleased_.end(), false);
  std::fill(mouseButtonPressed_.begin(), mouseButtonPressed_.end(), false);
  std::fill(mouseButtonReleased_.begin(), mouseButtonReleased_.end(), false);
  mouseDelta_ = Vec2{};
  mouseState_.wheelDelta = 0;
  mouseState_.deltaX = 0;
  mouseState_.deltaY = 0;
}

void Input::pollEvents(Window& window)
{
  SDL_Event event{};
  while (SDL_PollEvent(&event)) {
    processEvent(event, window);
  }
}

void Input::processEvent(const ::SDL_Event& event, Window& window)
{
  const std::uint32_t currentWindowId = window.windowId();

  if (event.type == SDL_EVENT_QUIT) {
    window.requestClose();
    return;
  }

  if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
    if (currentWindowId == 0U || event.window.windowID == currentWindowId) {
      window.requestClose();
    }
    return;
  }

  if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
    Key key{};
    if (tryMapKey(event.key.key, key)) {
      const bool down = (event.type == SDL_EVENT_KEY_DOWN);
      setKeyState(key, down);
      if (down && key == Key::Escape) {
        window.requestClose();
      }
    }
    return;
  }

  if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
    const bool down = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
    if (event.button.button == SDL_BUTTON_LEFT) {
      setMouseButtonState(MouseButton::Left, down);
      mouseState_.leftButton = down;
    } else if (event.button.button == SDL_BUTTON_RIGHT) {
      setMouseButtonState(MouseButton::Right, down);
      mouseState_.rightButton = down;
    } else if (event.button.button == SDL_BUTTON_MIDDLE) {
      mouseState_.middleButton = down;
    }
    return;
  }

  if (event.type == SDL_EVENT_MOUSE_MOTION) {
    mouseState_.x = event.motion.x;
    mouseState_.y = event.motion.y;
    mouseState_.deltaX = event.motion.xrel;
    mouseState_.deltaY = event.motion.yrel;
    
    if (mouseCaptured_) {
      mouseDelta_.x += event.motion.xrel;
      mouseDelta_.y += event.motion.yrel;
    }
    
    prevMouseX_ = mouseState_.x;
    prevMouseY_ = mouseState_.y;
    return;
  }

  if (event.type == SDL_EVENT_MOUSE_WHEEL) {
    mouseState_.wheelDelta = event.wheel.y;
    return;
  }
}

bool Input::isKeyDown(Key key) const noexcept
{
  return keyDown_[keyIndex(key)];
}

bool Input::wasKeyPressed(Key key) const noexcept
{
  return keyPressed_[keyIndex(key)];
}

bool Input::isMouseButtonDown(MouseButton button) const noexcept
{
  return mouseButtonDown_[mouseButtonIndex(button)];
}

bool Input::wasMouseButtonPressed(MouseButton button) const noexcept
{
  return mouseButtonPressed_[mouseButtonIndex(button)];
}

bool Input::wasMouseButtonReleased(MouseButton button) const noexcept
{
  return mouseButtonReleased_[mouseButtonIndex(button)];
}

Vec2 Input::mouseDelta() const noexcept
{
  if (mouseInPanel_) {
    return Vec2{};
  }
  return mouseDelta_;
}

bool Input::isMouseCaptured() const noexcept
{
  return mouseCaptured_;
}

void Input::setKeyState(Key key, bool down) noexcept
{
  const std::size_t index = keyIndex(key);
  if (keyDown_[index] == down) {
    return;
  }

  keyDown_[index] = down;
  if (down) {
    keyPressed_[index] = true;
  } else {
    keyReleased_[index] = true;
  }
}

void Input::setMouseButtonState(MouseButton button, bool down) noexcept
{
  const std::size_t index = mouseButtonIndex(button);
  if (mouseButtonDown_[index] == down) {
    return;
  }

  mouseButtonDown_[index] = down;
  if (down) {
    mouseButtonPressed_[index] = true;
  } else {
    mouseButtonReleased_[index] = true;
  }
}

void Input::applyMouseCapture(Window& window) noexcept
{
  if (mouseInPanel_) {
    if (mouseCaptured_) {
      setMouseCaptured(window, false);
    }
    return;
  }

  const bool leftDown = mouseButtonDown_[mouseButtonIndex(MouseButton::Left)];
  if (leftDown && !mouseCaptured_) {
    setMouseCaptured(window, true);
  } else if (!leftDown && mouseCaptured_) {
    setMouseCaptured(window, false);
  }
}

void Input::setMouseCaptured(Window& window, bool captured) noexcept
{
  if (mouseCaptured_ == captured) {
    return;
  }

  if (window.nativeHandle() != nullptr) {
    SDL_SetWindowRelativeMouseMode(window.nativeHandle(), captured);
  }

  mouseCaptured_ = captured;
  mouseDelta_ = Vec2{};
}

MouseState Input::mouseState() const noexcept
{
  MouseState ms = mouseState_;
  if (mouseInPanel_) {
    ms.deltaX = 0;
    ms.deltaY = 0;
  }
  return ms;
}

void Input::setMouseInPanel(bool inPanel) noexcept
{
  mouseInPanel_ = inPanel;
}

} // namespace astraglyph
