#include "fmt/format.h"
#include "fmt/ranges.h"

#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
  {
    // string formating
    std::string s = fmt::format("The answer is {}.\n", 42);  // NOLINT(readability-magic-numbers)
    std::cout << s;
  }
  return 0;
}
