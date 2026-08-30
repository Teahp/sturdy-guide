#include "sturdy_guide/gcd.hpp"

namespace sturdy_guide {

int gcd(const int a, const int b) {
  int larger = a;
  int smaller = b;
  while (smaller != 0) {
    const int remainder = larger % smaller;
    larger = smaller;
    smaller = remainder;
  }
  return larger;
}

} 
