#pragma once

#include <new>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include <memory>

#define fwd(x) static_cast<decltype(x) &&>(x)

namespace erased {

struct erased_type_t {};

template <bool condition> struct fast_conditional {
  template <typename T, typename F> using apply = T;
};

template <> struct fast_conditional<false> {
  template <typename T, typename F> using apply = F;
};

template <typename Method, typename Signature> struct MethodTraitImpl;

template <typename Method, typename ReturnType, typename ErasedType,
          typename... Args>
struct MethodTraitImpl<Method, ReturnType (*)(ErasedType &, Args...)> {
  using function =
      fast_conditional<std::is_const_v<ErasedType>>::template apply<
          ReturnType(Args...) const, ReturnType(Args...)>;
};

template <typename Method>
using MethodTrait =
    MethodTraitImpl<Method,
                    decltype(&Method::template invoker<erased::erased_type_t>)>;

template <typename Method>
using MethodPtr = typename MethodTrait<Method>::function;

template <typename Method, typename F> struct Signature;

template <typename Method>
using CreateSignature = Signature<Method, MethodPtr<Method>>;

struct Copy {
  constexpr static void invoker(const auto &) {}
};
struct Move {
  constexpr static void invoker(const auto &) {}
};

namespace details::base {
template <typename...> struct base_with_methods;

template <> struct base_with_methods<> {
  using type = void;
  constexpr void invoker() {}
  constexpr virtual ~base_with_methods() = default;
};

template <typename Method, typename R, typename... Args>
struct base_with_methods<Signature<Method, R(Args...) const>> {
  using type = Method;
  constexpr virtual R invoker(Method, Args...) const = 0;
};

template <typename Method, typename R, typename... Args>
struct base_with_methods<Signature<Method, R(Args...)>> {
  using type = Method;
  constexpr virtual R invoker(Method, Args...) = 0;
};

} // namespace details::base

namespace details::concrete {
template <typename Base, typename Type, typename... Ts> struct concrete_method;

template <typename Base, typename Type>
struct concrete_method<Base, Type> : Base {
  Type& m_object;

  constexpr concrete_method(Type& object) : m_object{object} {}
};

template <typename Base, typename Type, typename Method, typename R,
          typename... Args>
struct concrete_method<Base, Type, Signature<Method, R(Args...) const>>
    : concrete_method<Base, Type> {
  using concrete_method<Base, Type>::concrete_method;
  constexpr R invoker(Method, Args... args) const override {
    return Method::invoker(this->m_object, fwd(args)...);
  }
};

template <typename Base, typename Type, typename Method, typename R,
          typename... Args>
struct concrete_method<Base, Type, Signature<Method, R(Args...)>>
    : concrete_method<Base, Type> {
  using concrete_method<Base, Type>::concrete_method;
  constexpr R invoker(Method, Args... args) override {
    return Method::invoker(this->m_object, fwd(args)...);
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

template <typename T, std::size_t N>
constexpr auto *construct(char (&array)[N], auto &&...args) {
  if constexpr (sizeof(T) <= N) {
    if (std::is_constant_evaluated())
      return new T{fwd(args)...};
    return new (array) T{fwd(args)...};
  } else {
    return new T{fwd(args)...};
  }
}

template <typename T, typename... List> constexpr bool contains() {
  return (std::is_same_v<T, List> || ...);
}

template <int Size, typename... Methods>
struct alignas(Size) basic_erased : public Methods... {
  static constexpr bool copyable = contains<Copy, Methods...>();
  static constexpr bool movable = contains<Move, Methods...>();

  struct Base {
    using MethodStorage = std::tuple<std::unique_ptr<details::base::base_with_methods<CreateSignature<Methods>>>...>;
    MethodStorage m_methods;

    constexpr virtual ~Base() = default;

    constexpr virtual Base *clone(bool is_dynamic, char *buffer) const = 0;
    constexpr virtual Base *move(bool is_dynamic, char *buffer) noexcept = 0;

    template <typename Method>
    constexpr decltype(auto) invoker(Method method, auto &&...xs) const {
      return invoke_helper(method, fwd(xs)...);
    }

  private:
    template <typename Method, std::size_t I = 0>
    constexpr decltype(auto) invoke_helper(Method method, auto &&...xs) const {
      if constexpr (I < sizeof...(Methods)) {
        const auto& storage_method = std::get<I>(m_methods);
        if constexpr (typeid(typename std::remove_cvref_t<decltype(*storage_method)>::type) == typeid(Method)) {
          return storage_method->invoker(method, fwd(xs)...);
        } else {
          return invoke_helper<Method, I + 1>(method, fwd(xs)...);
        }
      } else {
        static_assert(false);
      }
    }
  };

  static constexpr auto buffer_size = Size - sizeof(bool) - sizeof(Base *);

  template <typename Type>
  struct Concrete : Base {
    Type m_object;

    constexpr Concrete(auto &&...args) : m_object{fwd(args)...} {
      this->m_methods = std::make_tuple(std::make_unique<details::concrete::concrete_method<
            details::base::base_with_methods<CreateSignature<Methods>>, Type, CreateSignature<Methods>
          >>(this->m_object)...);
    }

    constexpr Base *clone(bool is_dynamic, char *soo_buffer) const override {
      if constexpr (copyable) {
        if (is_dynamic)
          return new Concrete{this->m_object};
        return new (soo_buffer) Concrete{this->m_object};
      } else {
        return nullptr;
      }
    }

    constexpr Base *move(bool is_dynamic, char *soo_buffer) noexcept override {
      if constexpr (movable) {
        if (is_dynamic)
          return new Concrete{static_cast<Type &&>(this->m_object)};
        return new (soo_buffer) Concrete{static_cast<Type &&>(this->m_object)};
      } else {
        return nullptr;
      }
    }
  };

  char m_array[buffer_size];
  bool m_dynamic;
  Base *m_ptr;

  template <typename T>
  constexpr basic_erased(std::in_place_type_t<T>, auto &&...args) noexcept
      : m_dynamic{is_dynamic<Concrete<T>, buffer_size>()},
        m_ptr{construct<Concrete<T>>(m_array, fwd(args)...)} {}

  template <typename T>
  constexpr basic_erased(T x) noexcept
      : basic_erased{std::in_place_type<T>, static_cast<T &&>(x)} {}

  constexpr decltype(auto) invoke(auto method, auto &&...xs) const {
    return m_ptr->invoker(method, fwd(xs)...);
  }

  constexpr decltype(auto) invoke(auto method, auto &&...xs) {
    return m_ptr->invoker(method, fwd(xs)...);
  }

  constexpr basic_erased(basic_erased &&other) noexcept
    requires movable
      : m_dynamic(other.m_dynamic),
        m_ptr{other.m_ptr->move(m_dynamic, m_array)} {}

  constexpr basic_erased(const basic_erased &other)
    requires copyable
      : m_dynamic(other.m_dynamic),
        m_ptr{other.m_ptr->clone(m_dynamic, m_array)} {}

  constexpr basic_erased &operator=(basic_erased &&other) noexcept
    requires movable
  {
    destroy();
    m_dynamic = other.m_dynamic;
    m_ptr = other.m_ptr->move(m_dynamic, m_array);
    return *this;
  }

  constexpr basic_erased &operator=(const basic_erased &other)
    requires copyable
  {
    destroy();
    m_dynamic = other.m_dynamic;
    m_ptr = other.m_ptr->clone(m_dynamic, m_array);
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

#undef fwd

#define ERASED_HEAD(a, ...) a
#define ERASED_TAIL(a, ...) __VA_ARGS__
#define ERASED_EAT(...)
#define ERASED_EXPAND(...) __VA_ARGS__

#define ERASED_REMOVE_PARENTHESIS_IMPL(...) __VA_ARGS__
#define ERASED_REMOVE_PARENTHESIS(...)                                         \
  ERASED_EXPAND(ERASED_REMOVE_PARENTHESIS_IMPL __VA_ARGS__)

#define ERASED_ADD_COMA_AFTER_PARENTHESIS_IMPL(...) (__VA_ARGS__),
#define ERASED_ADD_COMA_AFTER_PARENTHESIS(...)                                 \
  (ERASED_EXPAND(ERASED_ADD_COMA_AFTER_PARENTHESIS_IMPL __VA_ARGS__))

#define ERASED_REMOVE_AFTER_PARENTHESIS(...)                                   \
  ERASED_REMOVE_PARENTHESIS(                                                   \
      ERASED_HEAD ERASED_ADD_COMA_AFTER_PARENTHESIS(__VA_ARGS__))

#define ERASED_CAT_IMPL(a, b) a##b
#define ERASED_CAT(...) ERASED_CAT_IMPL(__VA_ARGS__)
#define ERASED_GET_AFTER_requires(...) (__VA_ARGS__)
#define ERASED_GET_INSIDE_REQUIRES(...)                                        \
  ERASED_REMOVE_AFTER_PARENTHESIS(                                             \
      ERASED_CAT(ERASED_GET_AFTER_, ERASED_EAT __VA_ARGS__))

#define ERASED_GET_TRAILING_RETURN(...)                                        \
  ERASED_EXPAND(ERASED_TAIL ERASED_ADD_COMA_AFTER_PARENTHESIS(                 \
      ERASED_CAT(ERASED_GET_AFTER_, ERASED_EAT __VA_ARGS__)))

#define ERASED_MAKE_BEHAVIOR(Name, name, signature)                            \
  struct Name {                                                                \
    static constexpr auto                                                      \
    invoker(auto ERASED_REMOVE_AFTER_PARENTHESIS(signature))                   \
        ERASED_GET_TRAILING_RETURN(signature) {                                \
      return ERASED_GET_INSIDE_REQUIRES(signature);                            \
    }                                                                          \
                                                                               \
    constexpr decltype(auto) name(this auto &&self, auto &&...args) {          \
      return self.invoke(Name{}, static_cast<decltype(args) &&>(args)...);     \
    }                                                                          \
  }

#undef fwd
