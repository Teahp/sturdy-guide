#include "sturdy_guide/text.hpp"

#include <cctype>
#include <cstddef>

namespace sturdy_guide {

std::string normalize_audience(const std::string_view audience) {
  // std::isspace 要求参数可表示为 unsigned char，转换可避免负 char 导致未定义行为。
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
    // 空字符串和全空白字符串统一使用 SDK 的默认值。
    return "World";
  }

  return std::string(audience.substr(first, last - first)); //这是一个修改
}

}  // namespace sturdy_guide
