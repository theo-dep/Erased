#include <erased/erased.h>

struct ComputeArea {
  constexpr static double invoker(auto &self) { return self.computeArea(); }

  // not necessary, but makes the client code easier to write
  constexpr double computeArea(this auto &erased) {
    return erased.invoke(ComputeArea{});
  }
};

struct Perimeter {
  constexpr static double invoker(const auto &self) { return self.perimeter(); }

  // not necessary, but makes the client code easier to write
  constexpr double perimeter(this const auto &erased) {
    return erased.invoke(Perimeter{});
  }
};

struct Circle {
  constexpr double computeArea() { return 3.14; }
  constexpr double perimeter() const { return 6.28; }
};

struct Rectangle {
  constexpr double computeArea() { return 1.0; }
  constexpr double perimeter() const { return 4.0; }
};

using Surface = erased::erased<ComputeArea, Perimeter>;

constexpr double simpleComputation(Surface x) {
  return x.perimeter() + x.computeArea();
}

constexpr double simpleMove() {
  Surface x = Circle();
  x = Rectangle();

  return simpleComputation(std::move(x));
}

constexpr double simpleCopy() {
  Surface x = Circle();
  Surface y(x);
  Surface z = Rectangle();
  x = z;
  return simpleComputation(y) + simpleComputation(x);
}

void tests() {
  static_assert(simpleComputation(Circle()) == 6.28 + 3.14);
  static_assert(simpleComputation(Rectangle()) == 4.0 + 1.0);

  static_assert(simpleMove() == 1.0 + 4.0);
  static_assert(simpleCopy() == (6.28 + 3.14) + (1.0 + 4.0));
}

int main() {}
