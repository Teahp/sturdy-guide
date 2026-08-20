#include "sturdy_guide/greeting.hpp"

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
  passed &= expect_equal("named audience", sturdy_guide::make_greeting("Linux"),
                         "Hello, Linux!");
  passed &= expect_equal("trimmed audience",
                         sturdy_guide::make_greeting("  C++  "), "Hello, C++!");
  passed &= expect_equal("empty audience", sturdy_guide::make_greeting(""),
                         "Hello, World!");
  return passed ? 0 : 1;
}
