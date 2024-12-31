# Erased
Erased is your constexpr friendly type erasure wrapper type.
It is meant to be a fast and easy to use C++ type-erasure implementation.

It is heavily inspired from [AnyAny](https://github.com/kelbon/AnyAny).

## Basics
Here are the currently supported features
1. `constexpr` friendly (in Clang and GCC, since MSVC does not support properly `virtual` function in a constexpr context)
2. Small Object Optimization
3. Copy and `noexcept` move operations
4. Extendible with multiple behaviors
5. Macros helper to remove more and more boilerplate.
6. Natural syntax at call site.

Here is an exemple of use:

```cpp
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

using Surface = erased::erased<ComputeArea, Perimeter>;

void f(const Surface &x) {
    double area = x.computeArea();
    double perimeter = x.perimeter();
}
```

<details open>
<summary>Here is the same with macros</summary>

```cpp
ERASED_MAKE_BEHAVIOR(ComputeArea, computeArea,
                     (&self) requires(self.computeArea())->double);
ERASED_MAKE_BEHAVIOR(Perimeter, perimeter,
                     (const &self) requires(self.perimeter())->double);

using Surface =
    erased::erased<ComputeArea, Perimeter, erased::Copy, erased::Move>;
```
</details>

