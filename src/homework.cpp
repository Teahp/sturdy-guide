#include "sturdy_guide/homework.hpp"

#include "sturdy_guide/greeting.hpp"

#include <algorithm>
#include <cctype>

namespace sturdy_guide {

std::string make_greeting_loud(const std::string_view audience) {
  // 复用库内已有的 make_greeting，展示库内多个实现文件可以互相协作。
  std::string loud = make_greeting(audience);
  std::transform(loud.begin(), loud.end(), loud.begin(),
                 [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
  return loud;
}

}  // namespace sturdy_guide
