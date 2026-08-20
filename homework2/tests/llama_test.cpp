#include "sturdy_guide/llama.hpp"

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
  passed &= expect_equal("llama reference",
                         sturdy_guide::llama_reference(),
                         "本地边端部署模型可以参考该仓库：https://github.com/ggml-org/llama.cpp");
  return passed ? 0 : 1;
}