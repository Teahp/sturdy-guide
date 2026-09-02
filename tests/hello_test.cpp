#include "sturdy_guide/hello.hpp"

#include <iostream>
#include <string>

int main() {
  const std::string actual = sturdy_guide::make_hello();
  if (actual == "Hello from sturdy-guide!") {
    std::cout << "hello_test: passed\n";
    return 0;
  }
  std::cerr << "hello_test: expected 'Hello from sturdy-guide!', got '" << actual
            << "'\n";
  return 1;
}
