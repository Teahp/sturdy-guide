#include "sturdy_guide/text.hpp"
#include <iostream>
#include <string>
#include <string_view>
namespace {

bool expect_equal(const std::string_view case_name, int actual, int expected) {
  if (actual == expected) {
    return true;
  }
  std::cerr << case_name << ": expected " << expected << ", got " << actual << "\n";
  return false;
}

} // namespace

extern int add(int a, int b);

int main() {
  bool passed = true;

  passed &= expect_equal("2+3", add(2,3), 5);
  passed &= expect_equal("-2+2", add(-2,2), 0);

  return passed ? 0 : 1;
}
