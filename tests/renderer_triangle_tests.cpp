#include "render/AsciiFramebuffer.hpp"
#include "render/Renderer.hpp"
#include "render/RenderSettings.hpp"
#include "scene/Camera.hpp"

#include <cassert>
#include <cstddef>

int main()
{
  using namespace astraglyph;

  RenderSettings settings;
  settings.gridWidth = 80;
  settings.gridHeight = 45;
  settings.colorOutput = false;
  settings.enableSoftShadows = false;
  settings.shadowSamples = 1;
  settings.enableReflections = false;
  settings.maxBounces = 0;

  Camera camera;
  camera.setAspect(static_cast<float>(settings.gridWidth) / static_cast<float>(settings.gridHeight));

  AsciiFramebuffer framebuffer;
  const Renderer renderer;
  renderer.render(framebuffer, camera, settings);

  std::size_t visibleCells = 0;
  for (const AsciiCell& cell : framebuffer.data()) {
    if (cell.sampleCount > 0 && cell.depth < 1000.0F && cell.glyph != ' ') {
      ++visibleCells;
    }
  }

  assert(framebuffer.width() == static_cast<std::size_t>(settings.gridWidth));
  assert(framebuffer.height() == static_cast<std::size_t>(settings.gridHeight));
  assert(visibleCells > 0);
}
