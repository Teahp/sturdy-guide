#include "sturdy_guide/anterique0.hpp"

namespace sturdy_guide {

std::string armor_color_name(const int armor_id) {
  switch (armor_id) {
    case 0:
      return "Blue";
    case 1:
      return "Red";
    default:
      return "Unknown";
  }
}

}  // namespace sturdy_guide