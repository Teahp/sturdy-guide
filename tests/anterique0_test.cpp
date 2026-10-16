#include "sturdy_guide/anterique0.hpp"

#include <iostream>
#include <string>
#include <string_view>

namespace {

// 与仓库既有测试保持一致：用返回值向 CTest 报告成功或失败。
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
  passed &= expect_equal("blue armor", sturdy_guide::armor_color_name(0), "Blue");
  passed &= expect_equal("red armor", sturdy_guide::armor_color_name(1), "Red");
  passed &= expect_equal("unknown armor", sturdy_guide::armor_color_name(9),
                         "Unknown");
  return passed ? 0 : 1;
}