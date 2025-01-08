#include <erased/erased.h>
#include <erased/ref.h>
#include <gtest/gtest.h>

ERASED_MAKE_BEHAVIOR(ComputeArea, computeArea,
                     (&self) requires(self.computeArea())->double);
ERASED_MAKE_BEHAVIOR(Perimeter, perimeter,
                     (const &self) requires(self.perimeter())->double);

using Surface =
    erased::erased<ComputeArea, Perimeter, erased::Copy, erased::Move>;

using SurfaceRef = erased::ref<ComputeArea, Perimeter>;

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

constexpr double simpleComputationRef(SurfaceRef ref) {
  return ref.perimeter() + ref.computeArea();
}

ERASED_MAKE_BEHAVIOR(
    Computer, compute,
    (const &self, int value) requires(self.compute(value))->int);

using Computable = erased::erased<Computer>;

constexpr int compute(Computable x, int value) { return x.compute(value); }

struct Double {
  constexpr int compute(int value) const { return value + value; }
};

struct Square {
  constexpr int compute(int value) const { return value * value; }
};

#ifndef _MSC_VER
TEST(Tests, CompileTimeTestsErased) {
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

  static_assert(compute(Double{}, 10) == 20);
  static_assert(compute(Square{}, 10) == 100);
}

constexpr auto simpleComputationRefCircle() {
  Circle circle{1.0};
  return simpleComputationRef(circle);
}

constexpr auto simpleComputationRefRectangle() {
  Rectangle rectangle{10.0, 5.0};
  return simpleComputationRef(rectangle);
}

TEST(Tests, CompileTimeTestsRef) {
  static_assert(simpleComputationRefCircle() ==
                Circle{1.0}.perimeter() + Circle{1.0}.computeArea());
  static_assert(simpleComputationRefRectangle() ==
                Rectangle{10.0, 5.0}.perimeter() +
                    Rectangle{10.0, 5.0}.computeArea());
}
#endif

TEST(Tests, tests) {
  ASSERT_EQ(simpleComputation(Circle(2.0)),
            Circle(2.0).computeArea() + Circle(2.0).perimeter());

  ASSERT_EQ(simpleComputation(Rectangle(3.0)),
            Rectangle(3.0).computeArea() + Rectangle(3.0).perimeter());

  ASSERT_EQ(simpleMove(), 1.0 + 4.0);

  ASSERT_EQ(simpleCopy(),
            (Circle(5.0).perimeter() + Circle(5.0).computeArea()) +
                Rectangle(10.0).perimeter() + Rectangle(10.0).computeArea());

  ASSERT_EQ(in_place_construction(),
            (Rectangle(10, 5).computeArea() + Circle(10.0).perimeter()));

  ASSERT_EQ(compute(Double{}, 10), 20);
  ASSERT_EQ(compute(Square{}, 10), 100);
}
