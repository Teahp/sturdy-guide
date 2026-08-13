#include "sturdy_guide/version.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect_equal(const std::string_view case_name, const std::string_view actual,
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
  return expect_equal("project version", sturdy_guide::project_version(),
                      "0.1.0")
             ? 0
             : 1;
}
