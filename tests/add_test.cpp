#include "sturdy_guide/add.h"
#include <iostream>
#include <string_view>

namespace {

bool expect_equal(std::string_view case_name, int actual, int expected)
{
    if (actual == expected)
        return true;
    std::cerr << case_name << ": expected " << expected << ", got " << actual << "\n";
    return false;
}

}

int main()
{
    bool ok = true;
    ok &= expect_equal("1+2", sturdy_guide::add(1,2), 3);
    ok &= expect_equal("-1+5", sturdy_guide::add(-1,5), 4);
    return ok ? 0 : 1;
}
