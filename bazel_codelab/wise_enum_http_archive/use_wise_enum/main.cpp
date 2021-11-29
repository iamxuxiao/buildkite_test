#include "wise_enum.h"
#include <cassert>

#include <iostream>
#include <vector>

#define _unused(x) ((void)(x))

WISE_ENUM(Color, (GREEN, 2), RED);


// bazel run --compilation_mode=dbg --cpu="k8"  //xpilot/hello_world/wise_enum:wise_enum_hello_world
int main(int argc, char* argv[]) {
  
  // enumerate 
  std::cout << "Enum values and names:\n";
  for (auto e : wise_enum::range<Color>) {
    std::cout << static_cast<int>(e.value) << " " << e.name << "\n";
  }

  // Convert any enum to a string
  std::cout << wise_enum::to_string(Color::RED) << "\n";

  // Convert any string to an optional<enum>
  const auto x1 = wise_enum::from_string<Color>("GREEN");
  const auto x2 = wise_enum::from_string<Color>("Greeeeeeen");

  _unused(x1);
  _unused(x2);
  
  assert(x1.value() == Color::GREEN);
  assert(!x2);
  
  return 0;
}
