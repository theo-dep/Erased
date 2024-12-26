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

struct Open {
  static void invoker(auto &self, const std::string &path) { self.open(path); }

  void open(this auto &erased, const auto &path) {
    erased.invoke(Open{}, path);
  }
};
struct Close {
  static void invoker(auto &self) { self.close(); }

  void close(this auto &erased) { erased.invoke(Close{}); }
};

using Drawable = erased::erased<Draw>;
using Openable = erased::erased<Open, Close>;

struct Circle {
  void draw(std::ostream &stream) const { stream << "Circle\n"; }
};

struct Rectangle {
  void draw(std::ostream &stream) const { stream << "Rectangle\n"; }
};

int main() {
  std::cout << sizeof(Drawable) << " " << sizeof(Openable) << "\n";

  Drawable x = Circle();

  x.draw(std::cout);

  x = Rectangle();
  x.draw(std::cout);

  return 0;
}
