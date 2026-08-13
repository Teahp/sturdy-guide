#include "sturdy_guide/new.hpp"


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
  passed &= expect_equal("named audience", sturdy_guide::new_greeting("Linux"),
                         "Reversed: xuniL!");
  passed &= expect_equal("trimmed audience",
                         sturdy_guide::new_greeting("C++"), "Reversed: ++C!");
  passed &= expect_equal("empty audience", sturdy_guide::new_greeting(""),
                         "Reversed: !");
  return passed ? 0 : 1;
}