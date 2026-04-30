#include "geometry/MeshFactory.hpp"
#include "render/Integrator.hpp"
#include "render/RayTracer.hpp"
#include "render/RenderSettings.hpp"
#include "scene/Scene.hpp"

#include <cassert>

namespace {

astraglyph::Scene makeAreaLightScene(bool addOccluder)
{
  using namespace astraglyph;

  Scene scene;
  scene.addMaterial(Material{Vec3{1.0F, 1.0F, 1.0F}, 0.5F, 0.0F, 0.0F});
  scene.addMesh(MeshFactory::makeTriangle(
      Vec3{-1.0F, -1.0F, 0.0F},
      Vec3{1.0F, -1.0F, 0.0F},
      Vec3{0.0F, 1.0F, 0.0F},
      0));

  if (addOccluder) {
    scene.addMesh(MeshFactory::makeTriangle(
        Vec3{-1.5F, -1.5F, 1.0F},
        Vec3{1.5F, -1.5F, 1.0F},
        Vec3{1.5F, 1.5F, 1.0F},
        0));
    scene.addMesh(MeshFactory::makeTriangle(
        Vec3{-1.5F, -1.5F, 1.0F},
        Vec3{1.5F, 1.5F, 1.0F},
        Vec3{-1.5F, 1.5F, 1.0F},
        0));
  }

  scene.addLight(Light{
      LightType::Area,
      Vec3{},
      Vec3{0.0F, 0.0F, 2.0F},
      Vec3{1.0F, 1.0F, 1.0F},
      1.0F,
      Vec3{1.0F, 0.0F, 0.0F},
      Vec3{0.0F, 1.0F, 0.0F},
      1.0F,
      1.0F,
  });

  return scene;
}

} // namespace

int main()
{
  using namespace astraglyph;

  const Ray ray{Vec3{0.0F, 0.0F, 0.5F}, Vec3{0.0F, 0.0F, -1.0F}, 0.0F, 10.0F};
  const RayTracer rayTracer;
  const Integrator integrator;

  RenderSettings settings;
  settings.backgroundColor = Vec3{};
  settings.ambientStrength = 0.0F;
  settings.enableReflections = false;
  settings.enableShadows = true;
  settings.enableSoftShadows = true;
  settings.shadowSamples = 4;
  settings.shadowBias = 0.001F;

  RenderMetrics softMetrics;
  const Vec3 softLit = integrator.traceRadiance(
      ray,
      makeAreaLightScene(false),
      rayTracer,
      settings,
      softMetrics);
  assert(softLit.z > 0.0F);
  assert(softMetrics.shadowRays == 4U);
  assert(softMetrics.occludedShadowRays == 0U);
  assert(softMetrics.averageShadowSamples == 4.0F);

  RenderMetrics hardAreaMetrics;
  settings.enableSoftShadows = false;
  const Vec3 hardAreaLit = integrator.traceRadiance(
      ray,
      makeAreaLightScene(false),
      rayTracer,
      settings,
      hardAreaMetrics);
  assert(hardAreaLit.z > 0.0F);
  assert(hardAreaMetrics.shadowRays == 1U);
  assert(hardAreaMetrics.averageShadowSamples == 1.0F);

  RenderMetrics noShadowMetrics;
  settings.enableShadows = false;
  const Vec3 unshadowed = integrator.traceRadiance(
      ray,
      makeAreaLightScene(false),
      rayTracer,
      settings,
      noShadowMetrics);
  assert(unshadowed.z > 0.0F);
  assert(noShadowMetrics.shadowRays == 0U);
  assert(noShadowMetrics.occludedShadowRays == 0U);

  RenderMetrics occludedMetrics;
  settings.enableShadows = true;
  settings.enableSoftShadows = true;
  settings.shadowSamples = 8;
  const Vec3 occluded = integrator.traceRadiance(
      ray,
      makeAreaLightScene(true),
      rayTracer,
      settings,
      occludedMetrics);
  assert(occluded.z < softLit.z);
  assert(occludedMetrics.shadowRays == 8U);
  assert(occludedMetrics.occludedShadowRays > 0U);
  assert(occludedMetrics.averageShadowSamples == 8.0F);
}
