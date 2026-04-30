#include "geometry/Triangle.hpp"
#include "render/RayTracer.hpp"

#include <cassert>
#include <cmath>
#include <vector>

namespace {

bool almostEqual(float lhs, float rhs, float epsilon = 1.0e-5F)
{
  return std::fabs(lhs - rhs) <= epsilon;
}

} // namespace

int main()
{
  using namespace astraglyph;

  const Triangle triangle{
      Vec3{0.0F, 0.0F, 0.0F},
      Vec3{1.0F, 0.0F, 0.0F},
      Vec3{0.0F, 1.0F, 0.0F},
      Vec3{0.0F, 0.0F, 1.0F},
      Vec3{0.0F, 0.0F, 1.0F},
      Vec3{0.0F, 0.0F, 1.0F}};

  // 1. Ray hits triangle center.
  const Ray centerRay{Vec3{0.25F, 0.25F, 1.0F}, Vec3{0.0F, 0.0F, -1.0F}};
  HitInfo centerHit{};
  assert(triangle.intersect(centerRay, centerHit, false));
  assert(centerHit.hit);
  assert(almostEqual(centerHit.t, 1.0F, 1.0e-4F));

  // 2. Ray misses triangle.
  const Ray missRay{Vec3{2.0F, 2.0F, 1.0F}, Vec3{0.0F, 0.0F, -1.0F}};
  HitInfo missHit{};
  assert(!triangle.intersect(missRay, missHit, false));

  // 3. Ray hits near edge.
  const Ray edgeRay{Vec3{0.001F, 0.001F, 1.0F}, Vec3{0.0F, 0.0F, -1.0F}};
  HitInfo edgeHit{};
  assert(triangle.intersect(edgeRay, edgeHit, false));
  assert(edgeHit.hit);

  // 4. tMin blocks hit.
  const Ray blockedByTMin{Vec3{0.25F, 0.25F, 1.0F}, Vec3{0.0F, 0.0F, -1.0F}, 1.01F, 10.0F};
  HitInfo tMinHit{};
  assert(!triangle.intersect(blockedByTMin, tMinHit, false));

  // 5. tMax blocks hit.
  const Ray blockedByTMax{Vec3{0.25F, 0.25F, 1.0F}, Vec3{0.0F, 0.0F, -1.0F}, 0.0F, 0.99F};
  HitInfo tMaxHit{};
  assert(!triangle.intersect(blockedByTMax, tMaxHit, false));

  // 6. Backface culling works.
  const Ray backfaceRay{Vec3{0.25F, 0.25F, -1.0F}, Vec3{0.0F, 0.0F, 1.0F}};
  HitInfo cullDisabledHit{};
  assert(triangle.intersect(backfaceRay, cullDisabledHit, false));
  HitInfo cullEnabledHit{};
  assert(!triangle.intersect(backfaceRay, cullEnabledHit, true));

  // 7. Barycentric coordinates roughly correct.
  const Vec3 bary = centerHit.barycentric;
  assert(almostEqual(bary.x + bary.y + bary.z, 1.0F, 1.0e-4F));
  assert(almostEqual(bary.x, 0.5F, 1.0e-3F));
  assert(almostEqual(bary.y, 0.25F, 1.0e-3F));
  assert(almostEqual(bary.z, 0.25F, 1.0e-3F));

  // 8. RayTracer returns the closest triangle hit.
  const Triangle farTriangle{
      Vec3{0.0F, 0.0F, -1.0F},
      Vec3{1.0F, 0.0F, -1.0F},
      Vec3{0.0F, 1.0F, -1.0F},
      Vec3{0.0F, 0.0F, 1.0F},
      Vec3{0.0F, 0.0F, 1.0F},
      Vec3{0.0F, 0.0F, 1.0F},
      Vec2{},
      Vec2{},
      Vec2{},
      7};
  const Triangle nearTriangle{
      Vec3{0.0F, 0.0F, 0.0F},
      Vec3{1.0F, 0.0F, 0.0F},
      Vec3{0.0F, 1.0F, 0.0F},
      Vec3{0.0F, 0.0F, 1.0F},
      Vec3{0.0F, 0.0F, 1.0F},
      Vec3{0.0F, 0.0F, 1.0F},
      Vec2{},
      Vec2{},
      Vec2{},
      3};
  const RayTracer rayTracer;
  const RenderSettings settings;
  const HitInfo closestHit =
      rayTracer.traceClosest(centerRay, std::vector<Triangle>{farTriangle, nearTriangle}, settings);
  assert(closestHit.hit);
  assert(almostEqual(closestHit.t, 1.0F, 1.0e-4F));
  assert(closestHit.materialId == 3);
}
