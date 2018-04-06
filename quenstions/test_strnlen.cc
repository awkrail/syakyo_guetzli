#include <iostream>

int main(int argc, char** argv) {
  int opt_idx = 1;
  std::cout << argv[opt_idx] << std::endl;
  std::cout << strnlen(argv[opt_idx], 2) << std::endl;
}