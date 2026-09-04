#include "sturdy_guide/gcd.hpp"

#include <iostream>
#include <string>
#include <string_view>

namespace {

bool expect_equal(const std::string_view case_name, const int actual,
                  const int expected) {
  if (actual == expected) {
    return true;
  }
  std::cerr << case_name << ": expected " << expected << ", got " << actual
            << '\n';
  return false;
}

}  // namespace

int main() {
  bool passed = true;
  passed &= expect_equal("coprime", sturdy_guide::gcd(12, 35), 1);
  passed &= expect_equal("common factor", sturdy_guide::gcd(12, 18), 6);
  passed &= expect_equal("one zero", sturdy_guide::gcd(0, 7), 7);
  passed &= expect_equal("both zero", sturdy_guide::gcd(0, 0), 0);
  passed &= expect_equal("order independent", sturdy_guide::gcd(18, 12), 6);
  return passed ? 0 : 1;
}
