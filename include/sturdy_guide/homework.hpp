#pragma once

#include <string>
#include <string_view>

namespace sturdy_guide {

// 返回全大写的欢迎语，例如 make_greeting_loud("Linux") -> "HELLO, LINUX!"。
std::string make_greeting_loud(std::string_view audience);

}  // namespace sturdy_guide
