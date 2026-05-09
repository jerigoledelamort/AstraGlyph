#pragma once

#include <vector>

#include "math/Vec3.hpp"

namespace astraglyph {

class TemporalAccumulator {
public:
  void resize(int width, int height);
  void reset();

  // Legacy single-alpha API (affects both channels)
  void setAlpha(float alpha) noexcept;
  [[nodiscard]] float getAlpha() const noexcept;

  // Per-channel alpha control
  void setDirectAlpha(float alpha) noexcept;
  [[nodiscard]] float getDirectAlpha() const noexcept;
  void setIndirectAlpha(float alpha) noexcept;
  [[nodiscard]] float getIndirectAlpha() const noexcept;
  void setSpecularAlpha(float alpha) noexcept;
  [[nodiscard]] float getSpecularAlpha() const noexcept;

  // Накапливает для клетки (x, y) ра́дианс текущего кадра (EMA) раздельно
  // по direct, indirect и specular каналам. Возвращает сумму отфильтрованных каналов.
  Vec3 accumulate(int x, int y,
                  const Vec3& directRadiance,
                  const Vec3& indirectRadiance,
                  const Vec3& specularRadiance);

  // Текущее отфильтрованное значение (сумма каналов)
  Vec3 getAccumulatedRadiance(int x, int y) const;

  // Отдельные каналы
  Vec3 getDirectRadiance(int x, int y) const;
  Vec3 getIndirectRadiance(int x, int y) const;
  Vec3 getSpecularRadiance(int x, int y) const;

  // Число накопленных кадров для клетки (максимум из трёх каналов)
  int getAccumulatedFrames(int x, int y) const;

  // Пригодится позже для выборочного сброса
  void resetCell(int x, int y);

private:
  struct ChannelHistory {
    Vec3 filteredRadiance{0.0f, 0.0f, 0.0f};
    int frameCount{0};
  };

  struct CellHistory {
    ChannelHistory direct;
    ChannelHistory indirect;
    ChannelHistory specular;
  };

  std::vector<CellHistory> history_;
  int width_{0};
  int height_{0};
  float directAlpha_{0.6f};
  float indirectAlpha_{0.15f};
  float specularAlpha_{0.8f};
};

} // namespace astraglyph
