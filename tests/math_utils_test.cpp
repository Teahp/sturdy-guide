#include <iostream>
#include "sturdy_guide/math_utils.hpp"

#define CHECK(cond, msg) \
    if (!(cond)) { \
        std::cerr << "❌ FAIL: " << msg << "\n"; \
        return 1; \
    }

int main()
{
    CHECK(add(1,2) == 3, "1+2=3");
    CHECK(add(-1,5) == 4, "-1+5=4");
    CHECK(add(0,0) == 0, "0+0=0");

    std::cout << "✅ All math utils test passed\n";
    return 0;
}