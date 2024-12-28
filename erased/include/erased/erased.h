#pragma once

#include <array>
#include <type_traits>
#include <utility>

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

struct Copy {};
struct Move {};

namespace details::base {
template <typename...> struct base_method;

template <> struct base_method<> {
  constexpr void invoker() {}
  constexpr virtual ~base_method() = default;
};

template <typename Method, typename R, typename... Args, typename... Others>
struct base_method<Signature<Method, R (*)(Args...), constness::Const>,
                   Others...> : base_method<Others...> {
  using base_method<Others...>::invoker;
  constexpr virtual R invoker(Method, Args...) const = 0;
};

template <typename Method, typename R, typename... Args, typename... Others>
struct base_method<Signature<Method, R (*)(Args...), constness::Mutable>,
                   Others...> : base_method<Others...> {
  using base_method<Others...>::invoker;
  constexpr virtual R invoker(Method, Args...) = 0;
};

template <typename... Signatures>
struct base : public base_method<Signatures...> {
  constexpr virtual base *clone(bool is_dynamic,
                                std::byte *soo_buffer) const = 0;
  constexpr virtual base *move(bool is_dynamic, std::byte *soo_buffer) = 0;
};

} // namespace details::base

namespace details::concrete {
template <typename Base, typename Type, typename... Ts> struct concrete_method;

template <typename Base, typename Type>
struct concrete_method<Base, Type> : Base {
  using Base::invoker;
  Type m_object;

  template <typename... Args>
  constexpr concrete_method(Args &&...args)
      : m_object{static_cast<Args>(args)...} {}
};

template <typename Base, typename Type, typename Method, typename R,
          typename... Args, typename... Others>
struct concrete_method<
    Base, Type, Signature<Method, R (*)(Args...), constness::Const>, Others...>
    : concrete_method<Base, Type, Others...> {
  using concrete_method<Base, Type, Others...>::concrete_method;
  using concrete_method<Base, Type, Others...>::invoker;
  constexpr R invoker(Method, Args... args) const override {
    return Method::invoker(this->m_object, static_cast<Args &&>(args)...);
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
    return Method::invoker(this->m_object, static_cast<Args &&>(args)...);
  }
};

template <typename Base, typename Type, typename... Signatures>
struct concrete : concrete_method<Base, Type, Signatures...> {
  using concrete_method<Base, Type, Signatures...>::concrete_method;
  using concrete_method<Base, Type, Signatures...>::invoker;

  constexpr virtual Base *clone(bool is_dynamic,
                                std::byte *soo_buffer) const override {
    if (is_dynamic)
      return new concrete{this->m_object};
    return new (soo_buffer) concrete{this->m_object};
  }

  constexpr virtual Base *move(bool is_dynamic,
                               std::byte *soo_buffer) override {
    if (is_dynamic)
      return new concrete{std::move(this->m_object)};
    return new (soo_buffer) concrete{std::move(this->m_object)};
  }
};

} // namespace details::concrete
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
  using Base = details::base::base<CreateSignature<Methods>...>;
  static_assert(Size > sizeof(Base *));

  static constexpr auto buffer_size = Size - sizeof(bool) - sizeof(Base *);

  std::array<std::byte, buffer_size> m_array;
  bool m_dynamic = false;
  Base *m_ptr;

  template <typename T>
  constexpr basic_erased(T x)
      : m_dynamic{is_dynamic<
            details::concrete::concrete<Base, T, CreateSignature<Methods>...>,
            buffer_size>()},
        m_ptr{construct<
            details::concrete::concrete<Base, T, CreateSignature<Methods>...>>(
            m_array, std::move(x))} {}

  template <typename T, typename... Args>
  constexpr basic_erased(std::in_place_type_t<T>, Args &&...args)
      : m_dynamic{is_dynamic<
            details::concrete::concrete<Base, T, CreateSignature<Methods>...>,
            buffer_size>()},
        m_ptr{construct<
            details::concrete::concrete<Base, T, CreateSignature<Methods>...>>(
            m_array, static_cast<Args &&>(args)...)} {}

  template <typename Method> constexpr decltype(auto) invoke(Method) const {
    return std::as_const(*m_ptr).invoker(Method{});
  }

  template <typename Method> constexpr decltype(auto) invoke(Method) {
    return m_ptr->invoker(Method{});
  }

  constexpr basic_erased(basic_erased &&other) noexcept
      : m_dynamic{other.m_dynamic},
        m_ptr{other.m_ptr->move(m_dynamic, m_array.data())} {}

  constexpr basic_erased &operator=(basic_erased &&other) noexcept {
    destroy();
    m_dynamic = other.m_dynamic;
    m_ptr = other.m_ptr->move(m_dynamic, m_array.data());
    return *this;
  }

  constexpr basic_erased(const basic_erased &other)
      : m_dynamic(other.m_dynamic),
        m_ptr(other.m_ptr->clone(m_dynamic, m_array.data())) {}

  constexpr basic_erased &operator=(const basic_erased &other) {
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
