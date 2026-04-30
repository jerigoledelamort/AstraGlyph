#include "geometry/Triangle.hpp"
#include "render/AsciiFramebuffer.hpp"
#include "render/Bvh.hpp"
#include "render/Renderer.hpp"
#include "render/RayTracer.hpp"
#include "render/RenderMetrics.hpp"
#include "render/RenderSettings.hpp"
#include "scene/Camera.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

namespace {

bool almostEqual(float lhs, float rhs, float epsilon = 1.0e-5F)
{
  if (std::isinf(lhs) || std::isinf(rhs)) {
    return lhs == rhs;
  }
  return std::fabs(lhs - rhs) <= epsilon;
}

astraglyph::Triangle makeTriangle(float z, int materialId, float xOffset = 0.0F, float yOffset = 0.0F)
{
  using namespace astraglyph;

  return Triangle{
      Vec3{xOffset + 0.0F, yOffset + 0.0F, z},
      Vec3{xOffset + 1.0F, yOffset + 0.0F, z},
      Vec3{xOffset + 0.0F, yOffset + 1.0F, z},
      Vec3{0.0F, 0.0F, 1.0F},
      Vec3{0.0F, 0.0F, 1.0F},
      Vec3{0.0F, 0.0F, 1.0F},
      Vec2{},
      Vec2{},
      Vec2{},
      materialId};
}

void assertSameHit(const astraglyph::HitInfo& lhs, const astraglyph::HitInfo& rhs)
{
  assert(lhs.hit == rhs.hit);
  if (!lhs.hit && !rhs.hit) {
    return;
  }
  assert(almostEqual(lhs.t, rhs.t));
  assert(lhs.materialId == rhs.materialId);
  assert(almostEqual(lhs.position.x, rhs.position.x));
  assert(almostEqual(lhs.position.y, rhs.position.y));
  assert(almostEqual(lhs.position.z, rhs.position.z));
}

} // namespace

int main()
{
  using namespace astraglyph;

  {
    Bvh bvh;
    bvh.build({});
    assert(bvh.triangleCount() == 0U);
    assert(bvh.nodeCount() == 0U);
    assert(bvh.leafCount() == 0U);

    std::vector<Triangle> smallTriangles{
        makeTriangle(0.0F, 1, 0.0F, 0.0F),
        makeTriangle(0.0F, 2, 2.0F, 0.0F),
        makeTriangle(0.0F, 3, 4.0F, 0.0F),
        makeTriangle(0.0F, 4, 6.0F, 0.0F),
    };
    bvh.build(smallTriangles);
    assert(bvh.triangleCount() == smallTriangles.size());
    assert(bvh.nodeCount() == 1U);
    assert(bvh.leafCount() == 1U);

    smallTriangles.push_back(makeTriangle(0.0F, 5, 8.0F, 0.0F));
    bvh.build(smallTriangles);
    assert(bvh.triangleCount() == smallTriangles.size());
    assert(bvh.nodeCount() > 1U);
    assert(bvh.leafCount() >= 2U);
  }

  {
    Bvh bvh;
    bvh.build({makeTriangle(0.0F, 10)});

    const Ray missRay{Vec3{2.0F, 2.0F, 1.0F}, Vec3{0.0F, 0.0F, -1.0F}};
    RenderMetrics metrics;
    HitInfo hit{};
    assert(!bvh.intersect(missRay, hit, false, &metrics));
    assert(!hit.hit);
    assert(metrics.bvhNodeTests > 0U);
    assert(metrics.bvhAabbTests > 0U);
  }

  {
    Bvh bvh;
    bvh.build({makeTriangle(-1.0F, 1), makeTriangle(0.0F, 2)});

    const Ray ray{Vec3{0.25F, 0.25F, 1.0F}, Vec3{0.0F, 0.0F, -1.0F}};
    RenderMetrics metrics;
    HitInfo hit{};
    assert(bvh.intersect(ray, hit, false, &metrics));
    assert(hit.hit);
    assert(almostEqual(hit.t, 1.0F));
    assert(hit.materialId == 2);
    assert(metrics.triangleTests > 0U);
  }

  {
    const std::vector<Triangle> triangles{
        makeTriangle(-2.0F, 11),
        makeTriangle(-1.0F, 12),
        makeTriangle(0.0F, 13),
        makeTriangle(0.5F, 14),
    };

    RayTracer rayTracer;
    rayTracer.buildAcceleration(triangles);

    RenderSettings settings;
    settings.enableBvh = true;
    settings.bvhLeafSize = 4;
    const Ray ray{Vec3{0.25F, 0.25F, 1.0F}, Vec3{0.0F, 0.0F, -1.0F}};
    const HitInfo hit = rayTracer.traceClosest(ray, triangles, settings);
    assert(hit.hit);
    assert(almostEqual(hit.t, 0.5F));
    assert(hit.materialId == 14);
  }

  {
    const std::vector<Triangle> triangles{
        makeTriangle(0.0F, 21, 0.0F, 0.0F),
        makeTriangle(-1.0F, 22, -1.5F, 0.0F),
        makeTriangle(-2.0F, 23, 1.5F, 0.0F),
        makeTriangle(-0.5F, 24, 0.0F, 1.5F),
        makeTriangle(-1.5F, 25, 0.0F, -1.5F),
    };

    std::vector<Ray> rays{
        Ray{Vec3{0.25F, 0.25F, 1.0F}, Vec3{0.0F, 0.0F, -1.0F}},
        Ray{Vec3{-1.25F, 0.25F, 1.0F}, Vec3{0.0F, 0.0F, -1.0F}},
        Ray{Vec3{1.75F, 0.25F, 1.0F}, Vec3{0.0F, 0.0F, -1.0F}},
        Ray{Vec3{4.0F, 4.0F, 1.0F}, Vec3{0.0F, 0.0F, -1.0F}},
    };

    RayTracer rayTracer;
    rayTracer.buildAcceleration(triangles);

    RenderSettings bruteSettings;
    bruteSettings.enableBvh = false;

    RenderSettings bvhSettings;
    bvhSettings.enableBvh = true;
    bvhSettings.bvhLeafSize = 4;

    RenderMetrics bruteMetrics;
    RenderMetrics bvhMetrics;
    for (const Ray& ray : rays) {
      const HitInfo bruteHit = rayTracer.traceClosest(ray, triangles, bruteSettings, &bruteMetrics);
      const HitInfo bvhHit = rayTracer.traceClosest(ray, triangles, bvhSettings, &bvhMetrics);
      assertSameHit(bruteHit, bvhHit);
    }

    assert(bvhMetrics.bvhNodes > 0U);
    assert(bvhMetrics.bvhLeaves > 0U);
    assert(bvhMetrics.triangleTests < bruteMetrics.triangleTests);
  }

  {
    RenderSettings bruteSettings;
    bruteSettings.gridWidth = 16;
    bruteSettings.gridHeight = 9;
    bruteSettings.colorOutput = false;
    bruteSettings.samplesPerCell = 1;
    bruteSettings.maxSamplesPerCell = 1;
    bruteSettings.jitteredSampling = false;
    bruteSettings.adaptiveSampling = false;
    bruteSettings.enableSoftShadows = false;
    bruteSettings.shadowSamples = 1;
    bruteSettings.enableReflections = false;
    bruteSettings.maxBounces = 0;
    bruteSettings.enableBvh = false;

    RenderSettings bvhSettings = bruteSettings;
    bvhSettings.enableBvh = true;
    bvhSettings.bvhLeafSize = 4;

    Camera camera;
    camera.setAspect(static_cast<float>(bruteSettings.gridWidth) / static_cast<float>(bruteSettings.gridHeight));

    AsciiFramebuffer bruteFramebuffer;
    AsciiFramebuffer bvhFramebuffer;
    Renderer renderer;

    renderer.render(bruteFramebuffer, camera, bruteSettings);
    const RenderMetrics bruteMetrics = renderer.metrics();

    renderer.render(bvhFramebuffer, camera, bvhSettings);
    const RenderMetrics bvhMetrics = renderer.metrics();

    assert(bruteFramebuffer.width() == bvhFramebuffer.width());
    assert(bruteFramebuffer.height() == bvhFramebuffer.height());
    for (std::size_t index = 0; index < bruteFramebuffer.data().size(); ++index) {
      const AsciiCell& bruteCell = bruteFramebuffer.data()[index];
      const AsciiCell& bvhCell = bvhFramebuffer.data()[index];
      assert(bruteCell.glyph == bvhCell.glyph);
      assert(almostEqual(bruteCell.luminance, bvhCell.luminance, 1.0e-4F));
      assert(almostEqual(bruteCell.depth, bvhCell.depth, 1.0e-4F));
      assert(bruteCell.sampleCount == bvhCell.sampleCount);
    }

    assert(bvhMetrics.triangleTests < bruteMetrics.triangleTests);
    assert(bvhMetrics.bvhNodes > 0U);
    assert(bvhMetrics.bvhLeaves > 0U);

    std::cout
        << "renderer_compare brute_triangle_tests=" << bruteMetrics.triangleTests
        << " bvh_triangle_tests=" << bvhMetrics.triangleTests
        << " bvh_nodes=" << bvhMetrics.bvhNodes
        << " bvh_leaves=" << bvhMetrics.bvhLeaves << '\n';
  }

  {
    const std::vector<Triangle> triangles{makeTriangle(-5.0F, 30)};

    RayTracer rayTracer;
    rayTracer.buildAcceleration(triangles);

    RenderSettings settings;
    settings.enableBvh = true;
    settings.bvhLeafSize = 4;

    RenderMetrics metrics;
    const Ray ray{Vec3{0.25F, 0.25F, 1.0F}, Vec3{0.0F, 0.0F, -1.0F}, 0.001F, 4.0F};
    const HitInfo hit = rayTracer.traceClosest(ray, triangles, settings, &metrics);
    assert(!hit.hit);
    assert(metrics.bvhAabbTests > 0U);
    assert(metrics.triangleTests == 0U);
  }
}
