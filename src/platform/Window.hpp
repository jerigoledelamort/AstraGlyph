#pragma once

#include "math/Vec3.hpp"

#include <cstdint>
#include <string>
#include <string_view>

struct SDL_Window;
struct SDL_Renderer;

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
  void drawFilledRect(int x, int y, int width, int height, const Vec3& color) noexcept;
  void present() noexcept;

  [[nodiscard]] int width() const noexcept;
  [[nodiscard]] int height() const noexcept;
  [[nodiscard]] std::string_view title() const noexcept;
  [[nodiscard]] SDL_Window* nativeHandle() const noexcept;
  [[nodiscard]] SDL_Renderer* nativeRenderer() const noexcept;
  [[nodiscard]] std::uint32_t windowId() const noexcept;

private:
  static void acquireSdl();
  static void releaseSdl() noexcept;
  void destroy() noexcept;

  SDL_Window* window_{nullptr};
  SDL_Renderer* renderer_{nullptr};
  int width_{0};
  int height_{0};
  std::string title_{};
  bool closeRequested_{false};
  bool sdlOwned_{false};
};

} // namespace astraglyph
