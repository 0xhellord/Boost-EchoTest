#include <iostream>
#include <vector>

class Foo {
 public:
  Foo() { std::cout << "Default constructor" << std::endl; }
  Foo(const Foo& other) { std::cout << "Copy constructor" << std::endl; }
  Foo(Foo&& other) { std::cout << "Move constructor" << std::endl; }
};

void functionA(Foo& obj) {
  std::vector<Foo> vec;
  vec.push_back(std::move(obj));  // 将 obj 转移到容器中
}

int main() {
  Foo obj1;
  functionA(std::move(obj1));  // 将 obj1 转移到 functionA 中
  functionA(obj1);
  return 0;
}
