#pragma once

#include <string>
#include <string_view>

namespace sturdy_guide {

// Returns a greeting after normalizing the supplied audience name.
std::string make_greeting(std::string_view audience);

}  // namespace sturdy_guide

