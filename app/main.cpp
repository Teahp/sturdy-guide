#include "sturdy_guide/greeting.hpp"
#include "sturdy_guide/new.hpp"

#include <iostream>
#include <string_view>

int main(const int argc, const char* const argv[]) {
  // 未提供命令行参数时，使用适合演示的默认名称。
  const std::string_view audience = argc > 1 ? argv[1] : "Linux";
  // std::cout << sturdy_guide::make_greeting(audience) << '\n';
  std::cout << sturdy_guide::new_greeting(audience) << '\n';

  return 0;
}
