#pragma once

#include <string>
#include <string_view>

namespace sturdy_guide {

// Trims surrounding ASCII whitespace and uses "World" for an empty value.
std::string normalize_audience(std::string_view audience);

}  // namespace sturdy_guide

