#include "sturdy_guide/new.hpp"

#include <string>
#include <string_view>

namespace sturdy_guide {

std::string new_greeting(std::string_view audience) {
  return "Reversed: " + std::string(audience.rbegin(), audience.rend()) + "!";
}

}  // namespace sturdy_guide