#pragma once

#include "math/Vec3.hpp"

#include <SDL3/SDL.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace astraglyph {

class Window {
public:
  Window(int width, int height, std::string title);
  ~Window();

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  Window(Window&& other) noexcept;
  Window& operator=(Window&& other) noexcept;

  [[nodiscard]] bool shouldClose() const noexcept;
  void requestClose() noexcept;

  void setTitle(std::string_view title);

  void beginFrame(const Vec3& clearColor) noexcept;

  [[nodiscard]] bool lockFramePixels(int width, int height, void** pixels, int* pitch) noexcept;
  void unlockFramePixels() noexcept;

  void drawFilledRect(int x, int y, int width, int height, const Vec3& color) noexcept;

  void present() noexcept;

  [[nodiscard]] int width() const noexcept;
  [[nodiscard]] int height() const noexcept;
  [[nodiscard]] std::string_view title() const noexcept;

  [[nodiscard]] SDL_Window* nativeHandle() const noexcept;
  [[nodiscard]] SDL_Renderer* nativeRenderer() const noexcept;

  [[nodiscard]] std::uint32_t windowId() const noexcept;

  void setSize(int width, int height);
  void setFullscreen(bool fullscreen);
  [[nodiscard]] bool isFullscreen() const noexcept;
  void updateSize() noexcept;

private:
  void destroy() noexcept;

  SDL_Window* window_{nullptr};
  SDL_Renderer* renderer_{nullptr};
  SDL_Texture* frameTexture_{nullptr};
  int frameTextureWidth_{0};
  int frameTextureHeight_{0};
  int width_{0};
  int height_{0};
  std::string title_{};
  bool closeRequested_{false};
  bool sdlOwned_{false};

  bool fullscreen_{false};
  int windowedWidth_{0};
  int windowedHeight_{0};
  int windowedPosX_{0};
  int windowedPosY_{0};
};

} // namespace astraglyph
