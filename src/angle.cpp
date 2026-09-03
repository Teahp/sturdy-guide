#include "sturdy_guide/angle.hpp"

namespace sturdy_guide {

namespace {
constexpr double pi = 3.14159265358979323846;
}

double degrees_to_radians(double degrees) {
    return degrees * (pi / 180.0);
}

double radians_to_degrees(double radians) {
    return radians * (180.0 / pi);
}

}
