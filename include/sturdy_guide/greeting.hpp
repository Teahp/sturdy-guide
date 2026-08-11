#pragma once

#include <string>
#include <string_view>

namespace sturdy_guide {

// 规范化传入名称后，返回完整欢迎语。
std::string make_greeting(std::string_view audience);

}  // namespace sturdy_guide
