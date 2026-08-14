#include "sturdy_guide/version.hpp"

namespace sturdy_guide {

std::string project_version() {
#ifdef STURDY_GUIDE_PROJECT_VERSION
  return STURDY_GUIDE_PROJECT_VERSION;
#else
  return "unknown";
#endif
}

}  // namespace sturdy_guide
