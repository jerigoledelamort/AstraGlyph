#include "render/GlyphAtlas.hpp"
#include "terminus_ttf_data.h"

#include <algorithm>
#include <cassert>
#include <cstddef>

int main()
{
  // Map # at 10x12 non-empty
  {
    astraglyph::GlyphAtlas atlas(font_data, font_data_len);
    atlas.build(10, 12, "#");
    const auto& mask = atlas.getMask('#');
    assert(mask.size() == static_cast<std::size_t>(10 * 12));
    const bool hasOne = std::find(mask.begin(), mask.end(), 1) != mask.end();
    assert(hasOne);
  }

  // Space returns all zeros
  {
    astraglyph::GlyphAtlas atlas(font_data, font_data_len);
    atlas.build(8, 8, " ");
    const auto& mask = atlas.getMask(' ');
    assert(mask.size() == static_cast<std::size_t>(8 * 8));
    const bool allZero = std::all_of(mask.begin(), mask.end(),
                                     [](std::uint8_t v) { return v == 0; });
    assert(allZero);
  }

  // Cache is deterministic
  {
    astraglyph::GlyphAtlas atlas(font_data, font_data_len);
    atlas.build(10, 10, "A");
    const auto& mask1 = atlas.getMask('A');
    const auto& mask2 = atlas.getMask('A');
    assert(mask1 == mask2);
  }

  // Unknown char returns empty
  {
    astraglyph::GlyphAtlas atlas(font_data, font_data_len);
    atlas.build(10, 10, "A");
    const auto& mask = atlas.getMask('~');
    assert(mask.empty());
  }

  return 0;
}
