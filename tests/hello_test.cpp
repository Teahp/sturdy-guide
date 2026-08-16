#include <cassert>
#include <string>

std::string hello();

int main() {
    assert(hello() == "Hello, SRM!");
    return 0;
}
