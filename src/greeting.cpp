#include "sturdy_guide/greeting.hpp"

#include "sturdy_guide/text.hpp"
namespace sturdy_guide {

std::string make_greeting(const std::string_view audience) {
  // 规范化逻辑放在另一个源文件中，展示库内多个实现文件可以互相协作。
  return "Hello, " + normalize_audience(audience) + "!";
}

}// namespace sturdy_guide
