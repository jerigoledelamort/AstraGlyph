#include "render/Sampler.hpp"

#include <cassert>
#include <cmath>

namespace {

void expectNear(float actual, float expected)
{
  assert(std::fabs(actual - expected) < 1.0e-5F);
}

} // namespace

int main()
{
  using namespace astraglyph;

  const Sampler sampler;
  RenderSettings settings;
  settings.jitteredSampling = false;
  settings.adaptiveSampling = false;
  settings.maxSamplesPerCell = 16;

  settings.samplesPerCell = 1;
  Sample center = sampler.generateSubCellSample(3, 5, 0, settings);
  expectNear(center.uv.x, 0.5F);
  expectNear(center.uv.y, 0.5F);

  settings.samplesPerCell = 2;
  Sample diagonal0 = sampler.generateSubCellSample(3, 5, 0, settings);
  Sample diagonal1 = sampler.generateSubCellSample(3, 5, 1, settings);
  expectNear(diagonal0.uv.x, 0.25F);
  expectNear(diagonal0.uv.y, 0.25F);
  expectNear(diagonal1.uv.x, 0.75F);
  expectNear(diagonal1.uv.y, 0.75F);

  settings.samplesPerCell = 4;
  Sample quad2 = sampler.generateSubCellSample(3, 5, 2, settings);
  expectNear(quad2.uv.x, 0.25F);
  expectNear(quad2.uv.y, 0.75F);

  settings.samplesPerCell = 8;
  Sample stratified8 = sampler.generateSubCellSample(3, 5, 5, settings);
  expectNear(stratified8.uv.x, 0.625F);
  expectNear(stratified8.uv.y, 0.625F);

  settings.samplesPerCell = 16;
  Sample stratified16 = sampler.generateSubCellSample(3, 5, 14, settings);
  expectNear(stratified16.uv.x, 0.125F);
  expectNear(stratified16.uv.y, 0.875F);

  settings.samplesPerCell = 4;
  settings.jitteredSampling = true;
  settings.frameIndex = 9U;
  const Sample jitterA = sampler.generateSubCellSample(7, 11, 2, settings);
  const Sample jitterB = sampler.generateSubCellSample(7, 11, 2, settings);
  expectNear(jitterA.uv.x, jitterB.uv.x);
  expectNear(jitterA.uv.y, jitterB.uv.y);
  assert(jitterA.uv.x >= 0.0F && jitterA.uv.x < 1.0F);
  assert(jitterA.uv.y >= 0.0F && jitterA.uv.y < 1.0F);

  settings.frameIndex = 10U;
  const Sample jitterC = sampler.generateSubCellSample(7, 11, 2, settings);
  assert(std::fabs(jitterA.uv.x - jitterC.uv.x) > 1.0e-5F ||
         std::fabs(jitterA.uv.y - jitterC.uv.y) > 1.0e-5F);

  const std::uint32_t seed = sampler.makeSeed(4U, 12U, 34U, 56U);
  const Vec2 areaA = sampler.sample2D(seed, 1, 4, false);
  const Vec2 areaB = sampler.sample2D(seed, 1, 4, false);
  expectNear(areaA.x, 0.75F);
  expectNear(areaA.y, 0.25F);
  expectNear(areaA.x, areaB.x);
  expectNear(areaA.y, areaB.y);

  const Vec2 areaJitterA = sampler.sample2D(seed, 3, 8, true);
  const Vec2 areaJitterB = sampler.sample2D(seed, 3, 8, true);
  expectNear(areaJitterA.x, areaJitterB.x);
  expectNear(areaJitterA.y, areaJitterB.y);
  assert(areaJitterA.x >= 0.0F && areaJitterA.x < 1.0F);
  assert(areaJitterA.y >= 0.0F && areaJitterA.y < 1.0F);
}
