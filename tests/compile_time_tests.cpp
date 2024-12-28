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
  constexpr double computeArea() { return radius * radius * 3.14; }
  constexpr double perimeter() const { return radius * 6.28; }

  double radius = 1.0;
};

struct Rectangle {
  constexpr double computeArea() { return a * b; }
  constexpr double perimeter() const { return 2.0 * (a + b); }

  double a = 1.0;
  double b = 1.0;
};

using Surface =
    erased::erased<ComputeArea, Perimeter, erased::Copy, erased::Move>;

using MoveOnlySurface = erased::erased<ComputeArea, Perimeter, erased::Move>;
using CopyOnlySurface = erased::erased<ComputeArea, Perimeter, erased::Copy>;

using OnlySurface = erased::erased<ComputeArea, Perimeter>;

static_assert(std::is_nothrow_move_constructible_v<Surface>);
static_assert(std::is_nothrow_move_assignable_v<Surface>);
static_assert(std::is_copy_constructible_v<Surface>);
static_assert(std::is_copy_assignable_v<Surface>);

static_assert(std::is_nothrow_move_constructible_v<MoveOnlySurface>);
static_assert(std::is_nothrow_move_assignable_v<MoveOnlySurface>);
static_assert(!std::is_copy_constructible_v<MoveOnlySurface>);
static_assert(!std::is_copy_assignable_v<MoveOnlySurface>);

static_assert(!std::is_nothrow_move_constructible_v<CopyOnlySurface>);
static_assert(!std::is_nothrow_move_assignable_v<CopyOnlySurface>);
static_assert(std::is_move_constructible_v<CopyOnlySurface>);
static_assert(std::is_move_assignable_v<CopyOnlySurface>);
static_assert(std::is_copy_constructible_v<CopyOnlySurface>);
static_assert(std::is_copy_assignable_v<CopyOnlySurface>);

static_assert(!std::is_nothrow_move_constructible_v<OnlySurface>);
static_assert(!std::is_nothrow_move_assignable_v<OnlySurface>);
static_assert(!std::is_move_constructible_v<OnlySurface>);
static_assert(!std::is_move_assignable_v<OnlySurface>);
static_assert(!std::is_copy_constructible_v<OnlySurface>);
static_assert(!std::is_copy_assignable_v<OnlySurface>);

constexpr double simpleComputation(Surface x) {
  return x.perimeter() + x.computeArea();
}

constexpr double simpleMove() {
  Surface x = Circle();
  x = Rectangle();

  return simpleComputation(std::move(x));
}

constexpr double simpleCopy() {
  Surface x = Circle(5.0);
  Surface y(x);
  Surface z = Rectangle(10.0);
  x = z;
  return simpleComputation(y) + simpleComputation(x);
}

constexpr double in_place_construction() {
  Surface x(std::in_place_type<Circle>, 10.0);
  Surface y(std::in_place_type<Rectangle>, 10.0, 5.0);

  return y.computeArea() + x.perimeter();
}

void tests() {
  static_assert(simpleComputation(Circle(2.0)) ==
                Circle(2.0).computeArea() + Circle(2.0).perimeter());
  static_assert(simpleComputation(Rectangle(3.0)) ==
                Rectangle(3.0).computeArea() + Rectangle(3.0).perimeter());

  static_assert(simpleMove() == 1.0 + 4.0);

  static_assert(simpleCopy() ==
                (Circle(5.0).perimeter() + Circle(5.0).computeArea()) +
                    Rectangle(10.0).perimeter() +
                    Rectangle(10.0).computeArea());

  static_assert(in_place_construction() ==
                (Rectangle(10, 5).computeArea() + Circle(10.0).perimeter()));
}

int main() {}
