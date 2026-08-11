#include "sturdy_guide/greeting.hpp"

#include "sturdy_guide/text.hpp"

namespace sturdy_guide {

std::string make_greeting(const std::string_view audience) {
  return "Hello, " + normalize_audience(audience) + "!";
}

}  // namespace sturdy_guide

