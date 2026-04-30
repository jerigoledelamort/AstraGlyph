#include "geometry/MeshFactory.hpp"
#include "render/Integrator.hpp"
#include "render/RayTracer.hpp"
#include "render/RenderSettings.hpp"
#include "scene/Scene.hpp"

#include <cassert>

int main()
{
  using namespace astraglyph;

  Scene scene;
  scene.addMaterial(Material{Vec3{1.0F, 1.0F, 1.0F}, 0.5F});
  scene.addLight(Light{LightType::Directional, Vec3{0.0F, 0.0F, -1.0F}, Vec3{}, Vec3{1.0F, 1.0F, 1.0F}, 1.0F});

  scene.addMesh(MeshFactory::makeTriangle(
      Vec3{-1.0F, -1.0F, 0.0F},
      Vec3{1.0F, -1.0F, 0.0F},
      Vec3{0.0F, 1.0F, 0.0F},
      0));
  scene.addMesh(MeshFactory::makeTriangle(
      Vec3{-0.25F, -0.25F, 0.5F},
      Vec3{0.25F, -0.25F, 0.5F},
      Vec3{0.0F, 0.25F, 0.5F},
      0));

  RenderSettings settings;
  settings.ambientStrength = 0.1F;
  settings.backgroundColor = Vec3{};
  settings.shadowBias = 0.001F;

  const Ray ray{Vec3{0.0F, 0.0F, -1.0F}, Vec3{0.0F, 0.0F, 1.0F}, 0.0F, 10.0F};
  const RayTracer rayTracer;
  const Integrator integrator;

  RenderMetrics shadowMetrics;
  settings.enableShadows = true;
  const Vec3 shadowed = integrator.traceRadiance(ray, scene, rayTracer, settings, shadowMetrics);

  RenderMetrics litMetrics;
  settings.enableShadows = false;
  const Vec3 lit = integrator.traceRadiance(ray, scene, rayTracer, settings, litMetrics);

  assert(shadowed.x < lit.x);
  assert(shadowed.x >= settings.ambientStrength);
  assert(litMetrics.primaryRays == 1U);
  assert(shadowMetrics.primaryRays == 1U);
  assert(shadowMetrics.shadowRays == 1U);
  assert(shadowMetrics.triangleTests > litMetrics.triangleTests);
  assert(shadowMetrics.hits > 0U);

  Scene pointScene;
  pointScene.addMaterial(Material{Vec3{1.0F, 1.0F, 1.0F}, 0.5F});
  pointScene.addLight(Light{LightType::Point, Vec3{}, Vec3{0.0F, 0.0F, 1.0F}, Vec3{1.0F, 1.0F, 1.0F}, 1.0F});
  pointScene.addMesh(MeshFactory::makeTriangle(
      Vec3{-1.0F, -1.0F, 0.0F},
      Vec3{1.0F, -1.0F, 0.0F},
      Vec3{0.0F, 1.0F, 0.0F},
      0));
  pointScene.addMesh(MeshFactory::makeTriangle(
      Vec3{-0.25F, -0.25F, 0.5F},
      Vec3{0.25F, -0.25F, 0.5F},
      Vec3{0.0F, 0.25F, 0.5F},
      0));

  RenderMetrics pointShadowMetrics;
  settings.enableShadows = true;
  const Vec3 pointShadowed = integrator.traceRadiance(ray, pointScene, rayTracer, settings, pointShadowMetrics);

  RenderMetrics pointLitMetrics;
  settings.enableShadows = false;
  const Vec3 pointLit = integrator.traceRadiance(ray, pointScene, rayTracer, settings, pointLitMetrics);

  assert(pointShadowed.x < pointLit.x);
  assert(pointShadowMetrics.shadowRays == 1U);
  assert(pointShadowMetrics.occludedShadowRays == 1U);
}
