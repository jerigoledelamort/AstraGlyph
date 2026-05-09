#include "platform/Window.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstddef>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

namespace astraglyph {
namespace {

std::mutex gSdlMutex;
std::size_t gSdlRefCount = 0;

void acquireSdl()
{
  const std::scoped_lock lock{gSdlMutex};
  if (gSdlRefCount == 0U) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
      throw std::runtime_error("SDL_Init failed: " + std::string{SDL_GetError()});
    }
  }
  ++gSdlRefCount;
}

void releaseSdl() noexcept
{
  const std::scoped_lock lock{gSdlMutex};
  if (gSdlRefCount == 0U) {
    return;
  }

  --gSdlRefCount;
  if (gSdlRefCount == 0U) {
    SDL_Quit();
  }
}

} // namespace

Window::Window(int width, int height, std::string title)
    : width_{width}, height_{height}, title_{std::move(title)}
{
  if (width_ <= 0 || height_ <= 0) {
    throw std::invalid_argument("Window size must be positive");
  }

  acquireSdl();
  sdlOwned_ = true;
  window_ = SDL_CreateWindow(title_.c_str(), width_, height_, SDL_WINDOW_RESIZABLE);
  if (window_ == nullptr) {
    releaseSdl();
    sdlOwned_ = false;
    throw std::runtime_error("SDL_CreateWindow failed: " + std::string{SDL_GetError()});
  }

  renderer_ = SDL_CreateRenderer(window_, nullptr);
  if (renderer_ == nullptr) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
    releaseSdl();
    sdlOwned_ = false;
    throw std::runtime_error("SDL_CreateRenderer failed: " + std::string{SDL_GetError()});
  }

  // Сохраняем начальные размеры и позицию окна
  windowedWidth_ = width_;
  windowedHeight_ = height_;
  SDL_GetWindowPosition(window_, &windowedPosX_, &windowedPosY_);
}

Window::~Window()
{
  destroy();
}

Window::Window(Window&& other) noexcept
    : window_{other.window_},
      renderer_{other.renderer_},
      frameTexture_{other.frameTexture_},
      frameTextureWidth_{other.frameTextureWidth_},
      frameTextureHeight_{other.frameTextureHeight_},
      width_{other.width_},
      height_{other.height_},
      title_{std::move(other.title_)},
      closeRequested_{other.closeRequested_},
      sdlOwned_{other.sdlOwned_}
{
  other.window_ = nullptr;
  other.renderer_ = nullptr;
  other.frameTexture_ = nullptr;
  other.frameTextureWidth_ = 0;
  other.frameTextureHeight_ = 0;
  other.width_ = 0;
  other.height_ = 0;
  other.closeRequested_ = true;
  other.sdlOwned_ = false;
}

Window& Window::operator=(Window&& other) noexcept
{
  if (this == &other) {
    return *this;
  }

  destroy();

  window_ = other.window_;
  renderer_ = other.renderer_;
  frameTexture_ = other.frameTexture_;
  frameTextureWidth_ = other.frameTextureWidth_;
  frameTextureHeight_ = other.frameTextureHeight_;
  width_ = other.width_;
  height_ = other.height_;
  title_ = std::move(other.title_);
  closeRequested_ = other.closeRequested_;
  sdlOwned_ = other.sdlOwned_;

  other.window_ = nullptr;
  other.renderer_ = nullptr;
  other.frameTexture_ = nullptr;
  other.frameTextureWidth_ = 0;
  other.frameTextureHeight_ = 0;
  other.width_ = 0;
  other.height_ = 0;
  other.closeRequested_ = true;
  other.sdlOwned_ = false;
  return *this;
}

bool Window::shouldClose() const noexcept
{
  return closeRequested_;
}

void Window::requestClose() noexcept
{
  closeRequested_ = true;
}

void Window::setTitle(std::string_view title)
{
  title_ = std::string{title};
  if (window_ != nullptr) {
    SDL_SetWindowTitle(window_, title_.c_str());
  }
}

void Window::beginFrame(const Vec3& clearColor) noexcept
{
  if (renderer_ == nullptr) {
    return;
  }

  const auto toColor = [](float value) noexcept {
    const float clamped = std::clamp(value, 0.0F, 1.0F);
    return static_cast<std::uint8_t>(clamped * 255.0F);
  };

  SDL_SetRenderDrawColor(renderer_, toColor(clearColor.x), toColor(clearColor.y), toColor(clearColor.z), 255U);
  SDL_RenderClear(renderer_);
}

bool Window::lockFramePixels(int width, int height, void** pixels, int* pitch) noexcept
{
  if (renderer_ == nullptr || width <= 0 || height <= 0) {
    return false;
  }

  if (frameTexture_ == nullptr || frameTextureWidth_ != width || frameTextureHeight_ != height) {
    if (frameTexture_ != nullptr) {
      SDL_DestroyTexture(frameTexture_);
      frameTexture_ = nullptr;
    }
    frameTexture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, width, height);
    frameTextureWidth_ = width;
    frameTextureHeight_ = height;
  }

  if (frameTexture_ == nullptr) {
    return false;
  }

  return SDL_LockTexture(frameTexture_, nullptr, pixels, pitch);
}

void Window::unlockFramePixels() noexcept
{
  if (frameTexture_ == nullptr || renderer_ == nullptr) {
    return;
  }

  SDL_UnlockTexture(frameTexture_);
  SDL_RenderTexture(renderer_, frameTexture_, nullptr, nullptr);
}

void Window::drawFilledRect(int x, int y, int width, int height, const Vec3& color) noexcept
{
  if (renderer_ == nullptr || width <= 0 || height <= 0) {
    return;
  }

  const auto toColor = [](float value) noexcept {
    const float clamped = std::clamp(value, 0.0F, 1.0F);
    return static_cast<std::uint8_t>(clamped * 255.0F);
  };

  SDL_SetRenderDrawColor(renderer_, toColor(color.x), toColor(color.y), toColor(color.z), 255U);
  const SDL_FRect rect{
      static_cast<float>(x),
      static_cast<float>(y),
      static_cast<float>(width),
      static_cast<float>(height),
  };
  SDL_RenderFillRect(renderer_, &rect);
}

void Window::present() noexcept
{
  if (renderer_ != nullptr) {
    SDL_RenderPresent(renderer_);
  }
}

int Window::width() const noexcept
{
  return width_;
}

int Window::height() const noexcept
{
  return height_;
}

std::string_view Window::title() const noexcept
{
  return title_;
}

SDL_Window* Window::nativeHandle() const noexcept
{
  return window_;
}

SDL_Renderer* Window::nativeRenderer() const noexcept
{
  return renderer_;
}

std::uint32_t Window::windowId() const noexcept
{
  if (window_ == nullptr) {
    return 0U;
  }
  return SDL_GetWindowID(window_);
}

void Window::setFullscreen(bool fullscreen)
{
  if (fullscreen_ == fullscreen) {
    return;
  }

  if (fullscreen) {
    // Запоминаем текущие размеры и позицию перед переходом в fullscreen
    SDL_GetWindowSize(window_, &windowedWidth_, &windowedHeight_);
    SDL_GetWindowPosition(window_, &windowedPosX_, &windowedPosY_);
    
    if (!SDL_SetWindowFullscreen(window_, true)) {
      // Ошибка переключения - оставляем флаг как был
      return;
    }
  } else {
    // Восстанавливаем размеры и позицию из оконного режима
    if (!SDL_SetWindowFullscreen(window_, false)) {
      // Ошибка переключения - оставляем флаг как был
      return;
    }
    SDL_SetWindowPosition(window_, windowedPosX_, windowedPosY_);
    SDL_SetWindowSize(window_, windowedWidth_, windowedHeight_);
  }

  fullscreen_ = fullscreen;
  updateSize();
}

bool Window::isFullscreen() const noexcept
{
  return fullscreen_;
}

void Window::updateSize() noexcept
{
  if (window_ != nullptr) {
    SDL_GetWindowSize(window_, &width_, &height_);
  }
}

void Window::destroy() noexcept
{
  if (frameTexture_ != nullptr) {
    SDL_DestroyTexture(frameTexture_);
    frameTexture_ = nullptr;
    frameTextureWidth_ = 0;
    frameTextureHeight_ = 0;
  }
  if (renderer_ != nullptr) {
    SDL_DestroyRenderer(renderer_);
    renderer_ = nullptr;
  }
  if (window_ != nullptr) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }
  if (sdlOwned_) {
    releaseSdl();
    sdlOwned_ = false;
  }
}

} // namespace astraglyph
