#include "render/AsciiMapper.hpp"

#include <cassert>

int main()
{
  const astraglyph::AsciiMapper mapper;
  assert(mapper.map(0.0F) == ' ');
  assert(mapper.map(1.0F) == '@');
  assert(mapper.map(-1.0F) == ' ');
  assert(mapper.map(2.0F) == '@');
}
