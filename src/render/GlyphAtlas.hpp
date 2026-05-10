#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "stb_truetype.h"

namespace astraglyph {

class GlyphAtlas {
public:
  GlyphAtlas(const unsigned char* ttf_data, unsigned int ttf_size);

  void build(int cellW, int cellH, const std::string& charset = " .:-=+*#%@");

  [[nodiscard]] const std::vector<uint8_t>& getMask(char c) const;

private:
  [[nodiscard]] std::vector<uint8_t> rasterize(char c) const;

  stbtt_fontinfo font_{};
  int cellW_ = 0;
  int cellH_ = 0;
  std::unordered_map<char, std::vector<uint8_t>> cache_;
};

} // namespace astraglyph
