#include "geometry/MeshFactory.hpp"
#include "render/Integrator.hpp"
#include "render/RayTracer.hpp"
#include "render/RenderSettings.hpp"
#include "scene/Scene.hpp"

#include <cassert>

namespace {

astraglyph::Scene makeReflectionScene(float reflectivity)
{
  using namespace astraglyph;

  Scene scene;
  scene.addMaterial(Material{Vec3{1.0F, 1.0F, 1.0F}, 0.5F, 0.0F, reflectivity});
  scene.addMaterial(Material{
      Vec3{0.0F, 0.0F, 0.0F},
      0.5F,
      0.0F,
      0.0F,
      Vec3{0.8F, 0.7F, 0.6F},
      2.0F,
  });

  scene.addMesh(MeshFactory::makeTriangle(
      Vec3{-1.0F, -1.0F, 0.0F},
      Vec3{0.0F, 1.0F, 0.0F},
      Vec3{1.0F, -1.0F, 0.0F},
      0));
  scene.addMesh(MeshFactory::makeTriangle(
      Vec3{-1.0F, -1.0F, -2.0F},
      Vec3{1.0F, -1.0F, -2.0F},
      Vec3{0.0F, 1.0F, -2.0F},
      1));
  return scene;
}

} // namespace

int main()
{
  using namespace astraglyph;

  const Ray ray{Vec3{0.0F, 0.0F, -1.0F}, Vec3{0.0F, 0.0F, 1.0F}, 0.0F, 10.0F};
  const RayTracer rayTracer;
  const Integrator integrator;

  RenderSettings settings;
  settings.backgroundColor = Vec3{};
  settings.ambientStrength = 0.0F;
  settings.enableShadows = false;
  settings.enableReflections = true;
  settings.reflectionBias = 0.001F;

  RenderMetrics noBounceMetrics;
  settings.maxBounces = 0;
  const Vec3 noBounce = integrator.traceRadiance(
      ray,
      makeReflectionScene(1.0F),
      rayTracer,
      settings,
      noBounceMetrics);
  assert(noBounce.x == 0.0F);
  assert(noBounceMetrics.reflectionRays == 0U);
  assert(noBounceMetrics.secondaryRays == 0U);

  RenderMetrics zeroReflectivityMetrics;
  settings.maxBounces = 2;
  const Vec3 zeroReflectivity = integrator.traceRadiance(
      ray,
      makeReflectionScene(0.0F),
      rayTracer,
      settings,
      zeroReflectivityMetrics);
  assert(zeroReflectivity.x == 0.0F);
  assert(zeroReflectivityMetrics.reflectionRays == 0U);
  assert(zeroReflectivityMetrics.secondaryRays == 0U);

  RenderMetrics fullReflectionMetrics;
  settings.maxBounces = 1;
  const Vec3 fullReflection = integrator.traceRadiance(
      ray,
      makeReflectionScene(1.0F),
      rayTracer,
      settings,
      fullReflectionMetrics);
  assert(fullReflection.x > 1.5F);
  assert(fullReflection.y > 1.3F);
  assert(fullReflection.z > 1.1F);
  assert(fullReflectionMetrics.reflectionRays == 1U);
  assert(fullReflectionMetrics.secondaryRays == 1U);
  assert(fullReflectionMetrics.maxBounceReached == 1);

  RenderMetrics doubleBounceMetrics;
  settings.maxBounces = 2;
  const Vec3 doubleBounce = integrator.traceRadiance(
      ray,
      makeReflectionScene(1.0F),
      rayTracer,
      settings,
      doubleBounceMetrics);
  assert(doubleBounce.x == fullReflection.x);
  assert(doubleBounceMetrics.reflectionRays == 1U);
  assert(doubleBounceMetrics.secondaryRays == 1U);
  assert(doubleBounceMetrics.maxBounceReached == 1);

  RenderMetrics mixedReflectionMetrics;
  const Vec3 mixedReflection = integrator.traceRadiance(
      ray,
      makeReflectionScene(0.5F),
      rayTracer,
      settings,
      mixedReflectionMetrics);
  assert(mixedReflection.x > 0.7F && mixedReflection.x < fullReflection.x);
  assert(mixedReflectionMetrics.reflectionRays == 1U);

  RenderMetrics disabledReflectionMetrics;
  settings.enableReflections = false;
  const Vec3 disabledReflection = integrator.traceRadiance(
      ray,
      makeReflectionScene(1.0F),
      rayTracer,
      settings,
      disabledReflectionMetrics);
  assert(disabledReflection.x == 0.0F);
  assert(disabledReflectionMetrics.reflectionRays == 0U);
  assert(disabledReflectionMetrics.secondaryRays == 0U);
}
