#include "sturdy_guide/angle.hpp"

#include <cmath>
#include <iostream>

bool check(double actual, double expected) {
    if (!std::isfinite(actual) ||
        std::abs(actual - expected) > 1e-10) {
        std::cerr << "Expected " << expected
                  << ", got " << actual << '\n';
        return false;
    }
    return true;
}

int main() {
    constexpr double pi = 3.14159265358979323846;
    int failures = 0;

    if (!check(sturdy_guide::degrees_to_radians(0.0), 0.0))
        ++failures;

    if (!check(sturdy_guide::degrees_to_radians(180.0), pi))
        ++failures;

    if (!check(sturdy_guide::degrees_to_radians(-90.0), -pi / 2.0))
        ++failures;

    if (!check(sturdy_guide::degrees_to_radians(720.0), 4.0 * pi))
        ++failures;

    if (!check(sturdy_guide::radians_to_degrees(0.0), 0.0))
        ++failures;

    if (!check(sturdy_guide::radians_to_degrees(pi), 180.0))
        ++failures;

    if (!check(sturdy_guide::radians_to_degrees(-pi / 2.0), -90.0))
        ++failures;

    return failures == 0 ? 0 : 1;
}