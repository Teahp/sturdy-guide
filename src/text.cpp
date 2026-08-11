#include "sturdy_guide/text.hpp"

#include <cctype>
#include <cstddef>

namespace sturdy_guide {

std::string normalize_audience(const std::string_view audience) {
  std::size_t first = 0;
  while (first < audience.size() &&
         std::isspace(static_cast<unsigned char>(audience[first])) != 0) {
    ++first;
  }

  std::size_t last = audience.size();
  while (last > first &&
         std::isspace(static_cast<unsigned char>(audience[last - 1])) != 0) {
    --last;
  }

  if (first == last) {
    return "World";
  }

  return std::string(audience.substr(first, last - first));
}

}  // namespace sturdy_guide

