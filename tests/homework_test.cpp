#include "sturdy_guide/homework.hpp"

#include <iostream>
#include <string>
#include <string_view>

namespace {

// 这个小项目不引入测试框架，用返回值向 CTest 报告成功或失败。
bool expect_equal(const std::string_view case_name, const std::string& actual,
                  const std::string_view expected) {
  if (actual == expected) {
    return true;
  }

  std::cerr << case_name << ": expected '" << expected << "', got '" << actual
            << "'\n";
  return false;
}

}  // namespace

int main() {
  bool passed = true;
  passed &= expect_equal("loud named audience",
                         sturdy_guide::make_greeting_loud("Linux"),
                         "HELLO, LINUX!");
  passed &= expect_equal("loud trimmed audience",
                         sturdy_guide::make_greeting_loud("  C++  "),
                         "HELLO, C++!");
  passed &= expect_equal("loud empty audience",
                         sturdy_guide::make_greeting_loud(""),
                         "HELLO, WORLD!");
  return passed ? 0 : 1;
}
