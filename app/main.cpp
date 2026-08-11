#include "sturdy_guide/greeting.hpp"

#include <iostream>
#include <string_view>

int main(const int argc, const char* const argv[]) {
  const std::string_view audience = argc > 1 ? argv[1] : "Linux";
  std::cout << sturdy_guide::make_greeting(audience) << '\n';
  return 0;
}

