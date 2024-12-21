#include "erased/erased.h"
#include <iostream>

using namespace std;

struct Draw {
  static void invoker(const auto &self, std::ostream &stream) {
    self.draw(stream);
  }

  // not necessary, but makes the client code easier to write
  void draw(this const auto &erased, auto &stream) {
    erased.invoke(Draw{}, stream);
  }
};

using Drawable = erased::erased<Draw>;

struct Circle {
  void draw(std::ostream &stream) const { stream << "Circle\n"; }
};

int main() {
  Drawable drawable = Circle{};
  drawable.draw(std::cout);
  return 0;
}
