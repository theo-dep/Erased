#pragma once

#include <array>
#include <type_traits>
#include <utility>

#define fwd(x) static_cast<decltype(x) &&>(x)

namespace erased {

struct erased_type_t {};

enum class constness { Const, Mutable };

template <typename Method, typename Signature> struct MethodTraitImpl;

template <typename Method, typename ReturnType, typename ErasedType,
          typename... Args>
struct MethodTraitImpl<Method, ReturnType (*)(ErasedType, Args...)> {
  static constexpr bool is_const =
      std::is_const_v<std::remove_reference_t<ErasedType>>;
  static constexpr bool is_lvalue = std::is_lvalue_reference_v<ErasedType>;
  static constexpr bool is_rvalue = std::is_rvalue_reference_v<ErasedType>;

  static constexpr constness cv_ref_function() {
    if (is_const)
      return constness::Const;
    return constness::Mutable;
  }

  static constexpr auto cv_ref = cv_ref_function();
  using function_pointer = ReturnType (*)(Args...);
};

template <typename Method>
using MethodTrait =
    MethodTraitImpl<Method,
                    decltype(&Method::template invoker<erased::erased_type_t>)>;

template <typename Method>
using MethodPtr = typename MethodTrait<Method>::function_pointer;

template <typename Method, typename F, constness> struct Signature;

template <typename Method>
using CreateSignature =
    Signature<Method, MethodPtr<Method>, MethodTrait<Method>::cv_ref>;

struct Base {
  constexpr virtual ~Base() = default;
  constexpr virtual Base *clone(bool is_dynamic,
                                std::byte *soo_buffer) const = 0;
  constexpr virtual Base *move(bool is_dynamic, std::byte *soo_buffer) = 0;
};

struct Copy {
  static void invoker(const auto &);
};
struct Move {
  static void invoker(const auto &);
};

namespace details::base {
template <typename...> struct base_with_methods;

template <> struct base_with_methods<> : Base {
  constexpr void invoker() {}
};

template <typename Method, typename R, typename... Args, typename... Others>
struct base_with_methods<Signature<Method, R (*)(Args...), constness::Const>,
                         Others...> : base_with_methods<Others...> {
  using base_with_methods<Others...>::invoker;
  constexpr virtual R invoker(Method, Args...) const = 0;
};

template <typename Method, typename R, typename... Args, typename... Others>
struct base_with_methods<Signature<Method, R (*)(Args...), constness::Mutable>,
                         Others...> : base_with_methods<Others...> {
  using base_with_methods<Others...>::invoker;
  constexpr virtual R invoker(Method, Args...) = 0;
};

} // namespace details::base

namespace details::concrete {
template <typename Base, typename Type, typename... Ts> struct concrete_method;

template <typename Base, typename Type>
struct concrete_method<Base, Type> : Base {
  using Base::invoker;
  Type m_object;

  template <typename... Args>
  constexpr concrete_method(Args &&...args) : m_object{fwd(args)...} {}
};

template <typename Base, typename Type, typename Method, typename R,
          typename... Args, typename... Others>
struct concrete_method<
    Base, Type, Signature<Method, R (*)(Args...), constness::Const>, Others...>
    : concrete_method<Base, Type, Others...> {
  using concrete_method<Base, Type, Others...>::concrete_method;
  using concrete_method<Base, Type, Others...>::invoker;
  constexpr R invoker(Method, Args... args) const override {
    return Method::invoker(this->m_object, fwd(args)...);
  }
};

template <typename Base, typename Type, typename Method, typename R,
          typename... Args, typename... Others>
struct concrete_method<Base, Type,
                       Signature<Method, R (*)(Args...), constness::Mutable>,
                       Others...> : concrete_method<Base, Type, Others...> {
  using concrete_method<Base, Type, Others...>::concrete_method;
  using concrete_method<Base, Type, Others...>::invoker;
  constexpr R invoker(Method, Args... args) override {
    return Method::invoker(this->m_object, fwd(args)...);
  }
};

template <typename Base, typename Type, bool Movable, bool Copyable,
          typename... Signatures>
struct concrete : concrete_method<Base, Type, Signatures...> {
  using concrete_method<Base, Type, Signatures...>::concrete_method;
  using concrete_method<Base, Type, Signatures...>::invoker;

  constexpr virtual Base *clone(bool is_dynamic,
                                std::byte *soo_buffer) const override {
    if constexpr (Copyable) {
      if (is_dynamic)
        return new concrete{this->m_object};
      return new (soo_buffer) concrete{this->m_object};
    } else {
      return nullptr;
    }
  }

  constexpr virtual Base *move(bool is_dynamic,
                               std::byte *soo_buffer) override {
    if constexpr (Movable) {
      if (is_dynamic)
        return new concrete{std::move(this->m_object)};
      return new (soo_buffer) concrete{std::move(this->m_object)};
    } else {
      return nullptr;
    }
  }
};

} // namespace details::concrete
static_assert(sizeof(bool) == 1);

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
      return new T{fwd(args)...};
    return new (array.data()) T{fwd(args)...};
  } else {
    return new T{fwd(args)...};
  }
}

template <typename T, typename... List> constexpr bool contains() {
  return (std::is_same_v<T, List> || ...);
}

template <int Size, typename... Methods>
struct alignas(Size) basic_erased : public Methods... {
  using BaseMethods =
      details::base::base_with_methods<CreateSignature<Methods>...>;
  static_assert(Size > sizeof(Base *));

  static constexpr bool copyable = contains<Copy, Methods...>();
  static constexpr bool movable = contains<Move, Methods...>();

  static constexpr auto buffer_size = Size - sizeof(bool) - sizeof(Base *);

  std::array<std::byte, buffer_size> m_array;
  bool m_dynamic = false;
  Base *m_ptr;

  template <typename T>
  constexpr basic_erased(T x)
      : m_dynamic{is_dynamic<
            details::concrete::concrete<BaseMethods, T, movable, copyable,
                                        CreateSignature<Methods>...>,
            buffer_size>()},
        m_ptr{construct<details::concrete::concrete<
            BaseMethods, T, movable, copyable, CreateSignature<Methods>...>>(
            m_array, std::move(x))} {}

  template <typename T, typename... Args>
  constexpr basic_erased(std::in_place_type_t<T>, Args &&...args)
      : m_dynamic{is_dynamic<
            details::concrete::concrete<BaseMethods, T, movable, copyable,
                                        CreateSignature<Methods>...>,
            buffer_size>()},
        m_ptr{construct<details::concrete::concrete<
            BaseMethods, T, movable, copyable, CreateSignature<Methods>...>>(
            m_array, static_cast<Args &&>(args)...)} {}

  constexpr decltype(auto) invoke(auto method, auto &&...xs) const {
    return static_cast<const BaseMethods *>(m_ptr)->invoker(method, fwd(xs)...);
  }

  constexpr decltype(auto) invoke(auto method, auto &&...xs) {
    return static_cast<BaseMethods *>(m_ptr)->invoker(method, fwd(xs)...);
  }

  constexpr basic_erased(basic_erased &&other) noexcept
    requires movable
      : m_dynamic{other.m_dynamic},
        m_ptr{other.m_ptr->move(m_dynamic, m_array.data())} {}

  constexpr basic_erased &operator=(basic_erased &&other) noexcept
    requires movable
  {
    destroy();
    m_dynamic = other.m_dynamic;
    m_ptr = other.m_ptr->move(m_dynamic, m_array.data());
    return *this;
  }

  constexpr basic_erased(const basic_erased &other)
    requires copyable
      : m_dynamic(other.m_dynamic),
        m_ptr(other.m_ptr->clone(m_dynamic, m_array.data())) {}

  constexpr basic_erased &operator=(const basic_erased &other)
    requires copyable
  {
    destroy();
    m_dynamic = other.m_dynamic;
    m_ptr = other.m_ptr->move(m_dynamic, m_array.data());
    return *this;
  }

  constexpr void destroy() {
    if (m_dynamic)
      delete m_ptr;
    else
      m_ptr->~Base();
  }

  constexpr ~basic_erased() { destroy(); }
};

template <typename... Methods> using erased = basic_erased<32, Methods...>;
} // namespace erased
