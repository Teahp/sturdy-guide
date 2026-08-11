#include "sturdy_guide/text.hpp"

#include <iostream>
#include <string>
#include <string_view>

namespace {

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
  passed &= expect_equal("plain text", sturdy_guide::normalize_audience("Git"),
                         "Git");
  passed &= expect_equal("surrounding whitespace",
                         sturdy_guide::normalize_audience("\t Ninja \n"),
                         "Ninja");
  passed &= expect_equal("whitespace only",
                         sturdy_guide::normalize_audience("   "), "World");
  return passed ? 0 : 1;
}

