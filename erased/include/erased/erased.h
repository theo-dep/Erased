#pragma once

#include <array>
#include <type_traits>
#include <utility>

namespace erased {

struct erased_type_t {};

template <typename Method, typename Signature> struct MethodTraitImpl;

template <typename Method, typename ReturnType, typename ErasedType,
          typename... Args>
struct MethodTraitImpl<Method, ReturnType (*)(ErasedType &, Args...)> {
  static constexpr bool is_const = std::is_const_v<ErasedType>;

  using function_pointer = ReturnType (*)(Args...);
};

template <typename Method>
using MethodTrait =
    MethodTraitImpl<Method,
                    decltype(&Method::template invoker<erased::erased_type_t>)>;

template <typename Method>
using MethodPtr = typename MethodTrait<Method>::function_pointer;

template <typename Method>
constexpr bool IsConst = MethodTrait<Method>::is_const;

template <typename Method, typename Signature, bool IsConst>
struct MethodAndSignature;

template <typename Method>
using CreateMethodAndSignature =
    MethodAndSignature<Method, MethodPtr<Method>, IsConst<Method>>;

template <typename...> struct base;

template <typename Method, typename R, typename... Args>
struct base<MethodAndSignature<Method, R (*)(Args...), true>> {
  constexpr virtual ~base() = default;

  constexpr virtual R invoker(Method, Args...) const = 0;
};

template <typename Method, typename R, typename... Args>
struct base<MethodAndSignature<Method, R (*)(Args...), false>> {
  constexpr virtual ~base() = default;

  constexpr virtual R invoker(Method, Args...) = 0;
};

template <typename Method, typename R, typename... Args, typename... Others>
struct base<MethodAndSignature<Method, R (*)(Args...), true>, Others...>
    : base<Others...> {
  using base<Others...>::invoker;
  constexpr virtual R invoker(Method, Args...) const = 0;
};

template <typename Method, typename R, typename... Args, typename... Others>
struct base<MethodAndSignature<Method, R (*)(Args...), false>, Others...>
    : base<Others...> {
  using base<Others...>::invoker;
  constexpr virtual R invoker(Method, Args...) = 0;
};

template <typename Base, typename Type, typename... Ts> struct concrete;

template <typename Base, typename Type, typename Method, typename R,
          typename... Args>
struct concrete<Base, Type, MethodAndSignature<Method, R (*)(Args...), true>>
    : Base {
  Type m_object;
  using Base::invoker;

  constexpr concrete(Type object) : m_object{std::move(object)} {}

  constexpr R invoker(Method, Args... args) const override {
    return Method::invoker(m_object, static_cast<Args &&>(args)...);
  }
};

template <typename Base, typename Type, typename Method, typename R,
          typename... Args>
struct concrete<Base, Type, MethodAndSignature<Method, R (*)(Args...), false>>
    : Base {
  Type m_object;
  using Base::invoker;

  constexpr concrete(Type object) : m_object{std::move(object)} {}

  constexpr R invoker(Method, Args... args) override {
    return Method::invoker(m_object, static_cast<Args &&>(args)...);
  }
};

template <typename Base, typename Type, typename Method, typename R,
          typename... Args, typename... Others>
struct concrete<Base, Type, MethodAndSignature<Method, R (*)(Args...), true>,
                Others...> : concrete<Base, Type, Others...> {
  using concrete<Base, Type, Others...>::concrete;
  using concrete<Base, Type, Others...>::invoker;
  constexpr R invoker(Method, Args... args) const override {
    return Method::invoker(this->m_object, static_cast<Args &&>(args)...);
  }
};

template <typename Base, typename Type, typename Method, typename R,
          typename... Args, typename... Others>
struct concrete<Base, Type, MethodAndSignature<Method, R (*)(Args...), false>,
                Others...> : concrete<Base, Type, Others...> {
  using concrete<Base, Type, Others...>::concrete;
  using concrete<Base, Type, Others...>::invoker;
  constexpr R invoker(Method, Args... args) override {
    return Method::invoker(this->m_object, static_cast<Args &&>(args)...);
  }
};

static_assert(sizeof(bool) == 1);

template <int Size, typename BaseType> class soo {
private:
  template <typename T, typename... Args>
  constexpr T *construct(Args &&...args) {
    if constexpr (sizeof(T) <= Size) {
      if (std::is_constant_evaluated())
        return new T{std::forward<Args...>(args)...};
    }
  }

private:
};

template <typename T, int N> constexpr bool is_dynamic() {
  if constexpr (sizeof(T) <= N)
    return std::is_constant_evaluated();
  else
    return true;
}

template <typename T, std::size_t N, typename... Args>
constexpr auto *construct(std::array<std::byte, N> &array, Args &&...args) {
  if constexpr (sizeof(T) <= N) {
    if (std::is_constant_evaluated())
      return new T{static_cast<Args &&>(args)...};
    return new (array.data()) T{static_cast<Args &&>(args)...};
  } else {
    return new T{static_cast<Args &&>(args)...};
  }
}

template <int Size, typename... Methods>
struct alignas(Size) basic_erased : public Methods... {
  using Base = base<CreateMethodAndSignature<Methods>...>;
  static_assert(Size > sizeof(Base *));

  static constexpr auto buffer_size = Size - sizeof(bool) - sizeof(Base *);

  std::array<std::byte, buffer_size> m_array;
  bool m_dynamic = false;
  Base *m_ptr;

  template <typename T>
  constexpr basic_erased(T x)
      : m_dynamic{is_dynamic<T, buffer_size>()},
        m_ptr{
            construct<concrete<Base, T, CreateMethodAndSignature<Methods>...>>(
                m_array, std::move(x))} {}

  template <typename Method> constexpr decltype(auto) invoke(Method) const {
    return m_ptr->invoker(Method{});
  }

  basic_erased(const basic_erased &) = delete;
  basic_erased &operator=(const basic_erased &) = delete;

  constexpr ~basic_erased() {
    if (m_dynamic)
      delete m_ptr;
    else
      m_ptr->~Base();
  }
};

template <typename... Methods> using erased = basic_erased<32, Methods...>;
} // namespace erased
