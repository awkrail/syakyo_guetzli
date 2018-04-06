#include <memory>
#include <iostream>

int main() {
  // 空のshard_ptr
  std::shared_ptr<int> ptr;

  {
    std::shared_ptr<int> ptr2(new int(0));

    ptr = ptr2;
    *ptr += 10;
    *ptr2 += 20;
  }

  std::cout << "ptr = " << *ptr << std::endl; 
}