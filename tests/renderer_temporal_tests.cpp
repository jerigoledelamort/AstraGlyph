#include "render/AsciiFramebuffer.hpp"
#include "render/Renderer.hpp"
#include "render/RenderSettings.hpp"
#include "scene/Camera.hpp"
#include "scene/Scene.hpp"

#include <cassert>
#include <cstddef>
#include <iostream>

namespace {

void renderFrame(
    astraglyph::Renderer& renderer,
    astraglyph::AsciiFramebuffer& framebuffer,
    const astraglyph::Camera& camera,
    const astraglyph::Scene& scene,
    astraglyph::RenderSettings& settings)
{
  settings.validate();
  renderer.render(framebuffer, camera, scene, settings);
  settings.clearDirtyState();
  ++settings.frameIndex;
}

} // namespace

int main()
{
  using namespace astraglyph;

  RenderSettings settings;
  settings.gridWidth = 8;
  settings.gridHeight = 8;
  settings.temporalAccumulation = true;
  settings.samplesPerCell = 1;
  settings.maxSamplesPerCell = 1;
  settings.jitteredSampling = false;
  settings.adaptiveSampling = false;
  settings.enableShadows = false;
  settings.enableSoftShadows = false;
  settings.enableReflections = false;
  settings.maxBounces = 0;
  settings.enableMultithreading = false;

  Camera camera;
  camera.setAspect(1.0F);
  camera.setPosition(Vec3{0.0F, 0.0F, 0.0F});

  const Scene scene = Scene::createDefaultScene();
  AsciiFramebuffer framebuffer;
  Renderer renderer;

  // Кадр 1: первый рендер, accumulation должен сброситься
  renderFrame(renderer, framebuffer, camera, scene, settings);
  const int framesAfter1 = framebuffer.at(0, 0).accumulatedFrames;
  std::cout << "Frame 1 accumulatedFrames=" << framesAfter1 << '\n';
  assert(framesAfter1 == 1);

  // Кадр 2: камера не двигается, accumulation продолжается
  renderFrame(renderer, framebuffer, camera, scene, settings);
  const int framesAfter2 = framebuffer.at(0, 0).accumulatedFrames;
  std::cout << "Frame 2 accumulatedFrames=" << framesAfter2 << '\n';
  assert(framesAfter2 == 2);

  // Кадр 3: камера двигается, accumulation должен сброситься
  camera.setPosition(Vec3{1.0F, 0.0F, 0.0F});
  renderFrame(renderer, framebuffer, camera, scene, settings);
  const int framesAfter3 = framebuffer.at(0, 0).accumulatedFrames;
  std::cout << "Frame 3 (moved) accumulatedFrames=" << framesAfter3 << '\n';
  assert(framesAfter3 == 1);

  // Кадр 4: камера не двигается, accumulation продолжается
  renderFrame(renderer, framebuffer, camera, scene, settings);
  const int framesAfter4 = framebuffer.at(0, 0).accumulatedFrames;
  std::cout << "Frame 4 accumulatedFrames=" << framesAfter4 << '\n';
  assert(framesAfter4 == 2);

  // Кадр 5: имитируем движение через updateFromInput (как в Application)
  {
    Camera movingCamera;
    movingCamera.setAspect(1.0F);
    movingCamera.setPosition(Vec3{0.0F, 0.0F, 0.0F});

    // Фиктивный Input с нажатым W
    struct FakeInput : astraglyph::Input {
      bool isKeyDown(astraglyph::Key key) const noexcept override {
        return key == astraglyph::Key::W;
      }
      bool isMouseCaptured() const noexcept override { return false; }
      astraglyph::Vec2 mouseDelta() const noexcept override { return {}; }
      void beginFrame() noexcept override {}
      void pollEvents(astraglyph::Window&) override {}
      bool wasKeyPressed(astraglyph::Key) const noexcept override { return false; }
    } fakeInput;

    AsciiFramebuffer fb2;
    Renderer ren2;

    // Кадр 1
    renderFrame(ren2, fb2, movingCamera, scene, settings);
    assert(fb2.at(0, 0).accumulatedFrames == 1);

    // Кадр 2: та же позиция
    renderFrame(ren2, fb2, movingCamera, scene, settings);
    assert(fb2.at(0, 0).accumulatedFrames == 2);

    // Кадр 3: камера двигается через updateFromInput
    movingCamera.updateFromInput(fakeInput, 0.016F);
    std::cout << "Camera position after update: " << movingCamera.position().x << ", "
              << movingCamera.position().y << ", " << movingCamera.position().z << '\n';
    renderFrame(ren2, fb2, movingCamera, scene, settings);
    std::cout << "Frame 5 (updateFromInput) accumulatedFrames=" << fb2.at(0, 0).accumulatedFrames << '\n';
    assert(fb2.at(0, 0).accumulatedFrames == 1);
  }

  std::cout << "PASS\n";
}
