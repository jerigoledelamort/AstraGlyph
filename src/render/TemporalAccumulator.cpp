#include "render/TemporalAccumulator.hpp"

#include <algorithm>
#include <cstddef>

namespace astraglyph {

void TemporalAccumulator::resize(int width, int height)
{
  if (width_ == width && height_ == height) {
    return;
  }
  width_ = width;
  height_ = height;
  history_.assign(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_), CellHistory{});
}

void TemporalAccumulator::reset()
{
  std::fill(history_.begin(), history_.end(), CellHistory{});
}

void TemporalAccumulator::setAlpha(float alpha) noexcept
{
  directAlpha_ = alpha;
  indirectAlpha_ = alpha;
  specularAlpha_ = alpha;
}

float TemporalAccumulator::getAlpha() const noexcept
{
  return directAlpha_;
}

void TemporalAccumulator::setDirectAlpha(float alpha) noexcept
{
  directAlpha_ = alpha;
}

float TemporalAccumulator::getDirectAlpha() const noexcept
{
  return directAlpha_;
}

void TemporalAccumulator::setIndirectAlpha(float alpha) noexcept
{
  indirectAlpha_ = alpha;
}

float TemporalAccumulator::getIndirectAlpha() const noexcept
{
  return indirectAlpha_;
}

void TemporalAccumulator::setSpecularAlpha(float alpha) noexcept
{
  specularAlpha_ = alpha;
}

float TemporalAccumulator::getSpecularAlpha() const noexcept
{
  return specularAlpha_;
}

Vec3 TemporalAccumulator::accumulate(int x, int y,
                                     const Vec3& directRadiance,
                                     const Vec3& indirectRadiance,
                                     const Vec3& specularRadiance)
{
  CellHistory& cell = history_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                               static_cast<std::size_t>(x)];

  // Direct channel EMA
  {
    const float a = (cell.direct.frameCount == 0) ? 1.0F : directAlpha_;
    cell.direct.filteredRadiance = cell.direct.filteredRadiance * (1.0F - a) + directRadiance * a;
    ++cell.direct.frameCount;
  }

  // Indirect channel EMA
  {
    const float a = (cell.indirect.frameCount == 0) ? 1.0F : indirectAlpha_;
    cell.indirect.filteredRadiance = cell.indirect.filteredRadiance * (1.0F - a) + indirectRadiance * a;
    ++cell.indirect.frameCount;
  }

  // Specular channel EMA
  {
    const float a = (cell.specular.frameCount == 0) ? 1.0F : specularAlpha_;
    cell.specular.filteredRadiance = cell.specular.filteredRadiance * (1.0F - a) + specularRadiance * a;
    ++cell.specular.frameCount;
  }

  return cell.direct.filteredRadiance + cell.indirect.filteredRadiance + cell.specular.filteredRadiance;
}

Vec3 TemporalAccumulator::getAccumulatedRadiance(int x, int y) const
{
  const CellHistory& cell = history_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                                      static_cast<std::size_t>(x)];
  return cell.direct.filteredRadiance + cell.indirect.filteredRadiance + cell.specular.filteredRadiance;
}

Vec3 TemporalAccumulator::getDirectRadiance(int x, int y) const
{
  const CellHistory& cell = history_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                                      static_cast<std::size_t>(x)];
  return cell.direct.filteredRadiance;
}

Vec3 TemporalAccumulator::getIndirectRadiance(int x, int y) const
{
  const CellHistory& cell = history_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                                      static_cast<std::size_t>(x)];
  return cell.indirect.filteredRadiance;
}

Vec3 TemporalAccumulator::getSpecularRadiance(int x, int y) const
{
  const CellHistory& cell = history_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                                      static_cast<std::size_t>(x)];
  return cell.specular.filteredRadiance;
}

int TemporalAccumulator::getAccumulatedFrames(int x, int y) const
{
  const CellHistory& cell = history_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                                      static_cast<std::size_t>(x)];
  return std::max({cell.direct.frameCount, cell.indirect.frameCount, cell.specular.frameCount});
}

void TemporalAccumulator::resetCell(int x, int y)
{
  history_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
           static_cast<std::size_t>(x)] = CellHistory{};
}

} // namespace astraglyph
