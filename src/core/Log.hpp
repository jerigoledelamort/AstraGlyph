#pragma once

#include <string_view>

namespace astraglyph {

enum class LogLevel {
  Info,
  Warning,
  Error
};

void log(LogLevel level, std::string_view message);

} // namespace astraglyph
