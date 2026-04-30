#include "core/Log.hpp"

#include <iostream>

namespace astraglyph {
namespace {

const char* label(LogLevel level) noexcept
{
  switch (level) {
  case LogLevel::Info:
    return "info";
  case LogLevel::Warning:
    return "warning";
  case LogLevel::Error:
    return "error";
  }

  return "info";
}

} // namespace

void log(LogLevel level, std::string_view message)
{
  std::clog << '[' << label(level) << "] " << message << '\n';
}

} // namespace astraglyph
