#pragma once

#include <cstdint>

namespace astraglyph {

struct RenderMetrics {
  std::uint64_t primaryRays{0};
  std::uint64_t secondaryRays{0};
  std::uint64_t reflectionRays{0};
  std::uint64_t shadowRays{0};
  std::uint64_t occludedShadowRays{0};
  std::uint64_t shadowQueries{0};
  float averageShadowSamples{0.0F};
  std::uint64_t bvhNodeTests{0};
  std::uint64_t bvhAabbTests{0};
  std::uint64_t triangleTests{0};
  std::uint64_t bvhLeafTests{0};
  std::uint64_t bvhNodes{0};
  std::uint64_t bvhLeaves{0};
  std::uint64_t hits{0};
  std::uint64_t misses{0};
  float averageSamplesPerCell{0.0F};
  int maxSamplesUsed{0};
  std::uint64_t adaptiveCells{0};
  std::uint64_t totalCells{0};
  int maxBounceReached{0};
  int accumulatedFrames{0};
  bool temporalAccumulation{false};
  int threadCountUsed{0};
  bool enableMultithreading{false};
  bool renderProfilingEnabled{false};
  double bvhTraversalMs{0.0};
  double triangleIntersectionMs{0.0};
  double shadingMs{0.0};
  double finalOutputMs{0.0};
  double totalRenderMs{0.0};
};

} // namespace astraglyph
