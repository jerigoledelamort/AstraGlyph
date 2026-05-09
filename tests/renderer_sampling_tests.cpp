#include "render/AsciiFramebuffer.hpp"
#include "render/Renderer.hpp"
#include "render/RenderSettings.hpp"
#include "scene/Camera.hpp"
#include "scene/Scene.hpp"

#include <cassert>
#include <cstdint>
#include <cstddef>

namespace {

void verifyFixedSampleCount(
    astraglyph::Renderer& renderer,
    astraglyph::AsciiFramebuffer& framebuffer,
    const astraglyph::Camera& camera,
    const astraglyph::Scene& scene,
    astraglyph::RenderSettings settings,
    int expectedSamples)
{
  using namespace astraglyph;

  settings.samplesPerCell = expectedSamples;
  settings.maxSamplesPerCell = expectedSamples;
  settings.adaptiveSampling = false;
  renderer.render(framebuffer, camera, scene, settings);

  const RenderMetrics& metrics = renderer.metrics();
  const std::uint64_t totalCells =
      static_cast<std::uint64_t>(settings.gridWidth) * static_cast<std::uint64_t>(settings.gridHeight);
  assert(metrics.primaryRays == totalCells * static_cast<std::uint64_t>(expectedSamples));
  assert(metrics.averageSamplesPerCell == static_cast<float>(expectedSamples));
  assert(metrics.maxSamplesUsed == expectedSamples);
  assert(metrics.adaptiveCells == 0U);
  assert(metrics.totalCells == totalCells);
  for (const AsciiCell& cell : framebuffer.data()) {
    assert(cell.sampleCount == expectedSamples);
  }
}

} // namespace

int main()
{
  using namespace astraglyph;

  RenderSettings settings;
  settings.gridWidth = 12;
  settings.gridHeight = 8;
  settings.windowWidth = 12;
  settings.windowHeight = 8;
  settings.cellPixelSize = 1;
  settings.colorOutput = false;
  settings.samplesPerCell = 4;
  settings.maxSamplesPerCell = 8;
  settings.jitteredSampling = false;
  settings.adaptiveSampling = false;
  settings.enableSoftShadows = false;
  settings.shadowSamples = 1;
  settings.enableReflections = false;
  settings.maxBounces = 0;

  Camera camera;
  camera.setAspect(static_cast<float>(settings.gridWidth) / static_cast<float>(settings.gridHeight));

  const Scene scene = Scene::createDefaultScene();
  AsciiFramebuffer framebuffer;
  Renderer renderer;
  verifyFixedSampleCount(renderer, framebuffer, camera, scene, settings, 1);
  verifyFixedSampleCount(renderer, framebuffer, camera, scene, settings, 4);
  verifyFixedSampleCount(renderer, framebuffer, camera, scene, settings, 8);
  verifyFixedSampleCount(renderer, framebuffer, camera, scene, settings, 16);

  settings.adaptiveSampling = true;
  settings.samplesPerCell = 4;
  settings.maxSamplesPerCell = 8;
  settings.adaptiveVarianceThreshold = -1.0F;
  renderer.render(framebuffer, camera, scene, settings);

  const RenderMetrics& adaptiveMetrics = renderer.metrics();
  const std::uint64_t totalCells =
      static_cast<std::uint64_t>(settings.gridWidth) * static_cast<std::uint64_t>(settings.gridHeight);
  assert(adaptiveMetrics.primaryRays == totalCells * 8U);
  assert(adaptiveMetrics.averageSamplesPerCell == 8.0F);
  assert(adaptiveMetrics.maxSamplesUsed == 8);
  assert(adaptiveMetrics.adaptiveCells == totalCells);
  assert(adaptiveMetrics.totalCells == totalCells);
  for (const AsciiCell& cell : framebuffer.data()) {
    assert(cell.sampleCount == 8);
  }
}
