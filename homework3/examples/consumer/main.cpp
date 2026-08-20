#include <sturdy_guide/greeting.hpp>

#include <iostream>

int main() {
  // consumer 只包含公共头文件，不接触 SDK 的 src/ 实现。
  std::cout << sturdy_guide::make_greeting("SDK consumer") << '\n';
  return 0;
}
