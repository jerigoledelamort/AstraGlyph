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
    if (event.button.button == SDL_BUTTON_LEFT) {
      const bool down = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
      setMouseButtonState(MouseButton::Left, down);
      setMouseCaptured(window, down);
    }
    return;
  }

  if (event.type == SDL_EVENT_MOUSE_MOTION && mouseCaptured_) {
    mouseDelta_.x += event.motion.xrel;
    mouseDelta_.y += event.motion.yrel;
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

} // namespace astraglyph
