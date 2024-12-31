# Erased
Erased is your constexpr friendly type erasure wrapper type.
It is meant to be a fast and easy-to-use C++ type-erasure implementation.

It is heavily inspired by [AnyAny](https://github.com/kelbon/AnyAny).

The [tested compilers](https://godbolt.org/z/ss8PE6zc3) are:
1. MSVC (19.32 and above): `constexpr` does not work since MSVC does not handle `virtual` function in a constexpr context yet.
2. Clang (19.1 and above)
3. GCC (14.1 and above)

## Basics
Here are the currently supported features
1. `constexpr` friendly
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

<details close>
<summary>Here is the same with macros</summary>

```cpp
ERASED_MAKE_BEHAVIOR(ComputeArea, computeArea,
                     (&self) requires(self.computeArea())->double);
ERASED_MAKE_BEHAVIOR(Perimeter, perimeter,
                     (const &self) requires(self.perimeter())->double);

using Surface =
    erased::erased<ComputeArea, Perimeter>;
```
</details>

## Erased provided behaviors
The `erased::erased` type has only a constructor and destructor by default. We provide these behaviors to extend easily the given type:
1. Copy: Add copy constructor and copy assignment operator
2. Move: Add noexcept move constructor and noexcept move assignment operator
