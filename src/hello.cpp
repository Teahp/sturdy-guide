#include <string>

namespace sturdy_guide {

// A simple greeting independent from greeting.cpp, showing that the library
// can grow with more source files.
std::string make_hello() {
  return "Hello from sturdy-guide!";
}

}  // namespace sturdy_guide
