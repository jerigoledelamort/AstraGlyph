#include "render/GlyphAtlas.hpp"

#include <cstddef>

#include "stb_truetype.h"
#include "terminus_ttf_data.h"
#include "core/Log.hpp"

namespace astraglyph {

GlyphAtlas::GlyphAtlas(const unsigned char* ttf_data, unsigned int /*ttf_size*/)
{
  if (!stbtt_InitFont(&font_, ttf_data, 0)) {
    log(LogLevel::Error, "GlyphAtlas: failed to initialise TrueType font");
  }
}

void GlyphAtlas::build(int cellW, int cellH, const std::string& charset)
{
  cellW_ = cellW;
  cellH_ = cellH;
  cache_.clear();
  cache_.reserve(charset.size());

  for (char c : charset) {
    cache_[c] = rasterize(c);
  }
}

const std::vector<uint8_t>& GlyphAtlas::getMask(char c) const
{
  static const std::vector<uint8_t> kEmpty;
  auto it = cache_.find(c);
  if (it == cache_.end()) {
    return kEmpty;
  }
  return it->second;
}

std::vector<uint8_t> GlyphAtlas::rasterize(char c) const
{
  std::vector<uint8_t> mask(static_cast<std::size_t>(cellW_ * cellH_), 0);
  if (cellW_ <= 0 || cellH_ <= 0) {
    return mask;
  }

  const float scale = stbtt_ScaleForPixelHeight(&font_, cellH_ * 0.8f);

  int w = 0;
  int h = 0;
  int xoff = 0;
  int yoff = 0;
  unsigned char* bitmap = stbtt_GetCodepointBitmap(&font_, scale, scale,
                                                   static_cast<int>(c),
                                                   &w, &h, &xoff, &yoff);
  if (!bitmap) {
    log(LogLevel::Warning, "GlyphAtlas: failed to rasterise glyph");
    return mask;
  }

  const int dst_x = (cellW_ - w) / 2;
  const int dst_y = (cellH_ - h) / 2;

  for (int row = 0; row < h; ++row) {
    const int dy = dst_y + row;
    if (dy < 0 || dy >= cellH_) {
      continue;
    }
    for (int col = 0; col < w; ++col) {
      const int dx = dst_x + col;
      if (dx < 0 || dx >= cellW_) {
        continue;
      }
      const unsigned char pixel = bitmap[row * w + col];
      mask[static_cast<std::size_t>(dy * cellW_ + dx)] = (pixel > 128) ? 1 : 0;
    }
  }

  stbtt_FreeBitmap(bitmap, nullptr);
  return mask;
}

} // namespace astraglyph
