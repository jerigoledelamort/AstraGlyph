#include "math/Aabb.hpp"
#include "math/Ray.hpp"
#include "math/Vec3.hpp"

#include <cassert>
#include <cmath>

namespace {

bool almostEqual(float lhs, float rhs, float epsilon = 1.0e-5F)
{
  return std::fabs(lhs - rhs) <= epsilon;
}

} // namespace

int main()
{
  using namespace astraglyph;

  const Vec3 a{1.0F, 2.0F, 3.0F};
  const Vec3 b{4.0F, 5.0F, 6.0F};
  const astraglyph::Vec3 sum = a + b;
  assert(sum.x == 5.0F);
  assert(sum.y == 7.0F);
  assert(sum.z == 9.0F);
  const Vec3 diff = b - a;
  assert(diff.x == 3.0F);
  assert(diff.y == 3.0F);
  assert(diff.z == 3.0F);

  assert(almostEqual(dot(a, b), 32.0F));

  const Vec3 crossed = cross(
      Vec3{1.0F, 0.0F, 0.0F},
      Vec3{0.0F, 1.0F, 0.0F});
  assert(crossed.x == 0.0F);
  assert(crossed.y == 0.0F);
  assert(crossed.z == 1.0F);

  const Vec3 normalized = normalize(Vec3{0.0F, 3.0F, 4.0F});
  assert(almostEqual(normalized.x, 0.0F));
  assert(almostEqual(normalized.y, 0.6F));
  assert(almostEqual(normalized.z, 0.8F));
  assert(almostEqual(length(normalized), 1.0F));

  const Vec3 reflected = reflect(Vec3{0.0F, -1.0F, 0.0F}, Vec3{0.0F, 1.0F, 0.0F});
  assert(almostEqual(reflected.x, 0.0F));
  assert(almostEqual(reflected.y, 1.0F));
  assert(almostEqual(reflected.z, 0.0F));

  const Ray ray{
      Vec3{1.0F, 2.0F, 3.0F},
      Vec3{0.0F, 0.0F, -2.0F},
      0.0F,
      100.0F};
  const Vec3 point = ray.at(1.5F);
  assert(almostEqual(point.x, 1.0F));
  assert(almostEqual(point.y, 2.0F));
  assert(almostEqual(point.z, 0.0F));

  Aabb box;
  assert(box.empty());
  box.expand(Vec3{-1.0F, 0.0F, 1.0F});
  box.expand(Vec3{2.0F, 3.0F, 6.0F});
  assert(!box.empty());
  assert(almostEqual(box.min.x, -1.0F));
  assert(almostEqual(box.min.y, 0.0F));
  assert(almostEqual(box.min.z, 1.0F));
  assert(almostEqual(box.max.x, 2.0F));
  assert(almostEqual(box.max.y, 3.0F));
  assert(almostEqual(box.max.z, 6.0F));
  assert(box.longestAxis() == 2);

  const Ray hitRay{
      Vec3{-2.0F, 1.0F, 2.0F},
      Vec3{1.0F, 0.0F, 0.0F},
      0.0F,
      100.0F};
  float tMin = 0.0F;
  float tMax = 0.0F;
  assert(box.intersect(hitRay, tMin, tMax));
  assert(tMin >= 1.0F);
  assert(tMax >= tMin);

  const Ray missRay{
      Vec3{-2.0F, 5.0F, 2.0F},
      Vec3{1.0F, 0.0F, 0.0F},
      0.0F,
      100.0F};
  assert(!box.intersect(missRay, tMin, tMax));
}
