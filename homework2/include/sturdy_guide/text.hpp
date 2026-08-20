#pragma once

#include <string>
#include <string_view>

namespace sturdy_guide {

// 去掉首尾空白字符；结果为空时返回 "World"。
std::string normalize_audience(std::string_view audience);

}  // namespace sturdy_guide
