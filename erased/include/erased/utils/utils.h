#pragma once

#include <array>
#include <tuple>
#include <type_traits>

namespace erased::details {
struct erased_type_t {};

template <bool condition> struct fast_conditional {
  template <typename T, typename F> using apply = T;
};

template <> struct fast_conditional<false> {
  template <typename T, typename F> using apply = F;
};

template <typename Method, typename F> struct method_to_trait;

template <typename Method, typename ErasedType, typename ReturnType,
          typename... Args>
struct method_to_trait<Method, ReturnType (*)(ErasedType &, Args...)> {
  static constexpr bool is_const = std::is_const_v<ErasedType>;

  using first_argument =
      typename fast_conditional<is_const>::template apply<const void *, void *>;

  using type = ReturnType (*)(first_argument, Args...);

  template <typename T> static constexpr auto create_invoker_for() {
    return +[](first_argument first, Args... args) {
      if constexpr (is_const) {
        return Method::invoker(*static_cast<const T *>(first),
                               static_cast<Args &&>(args)...);
      } else {
        return Method::invoker(*static_cast<T *>(first),
                               static_cast<Args &&>(args)...);
      }
    };
  }
};

template <typename Method>
using method_to_trait_t =
    method_to_trait<Method, decltype(&Method::template invoker<erased_type_t>)>;

template <typename Method>
using method_ptr = typename method_to_trait_t<Method>::type;

template <typename Method, typename... Methods> constexpr int index_in_list() {
  std::array array = {std::is_same_v<Method, Methods>...};
  for (int i = 0; i < static_cast<int>(array.size()); ++i)
    if (array[i])
      return i;
  return -1;
}

template <typename... Methods> struct vtable {
  constexpr vtable(method_ptr<Methods>... ptrs) : m_functions{ptrs...} {}

  template <typename T>
  static constexpr const vtable *construct_for() noexcept {
    static constexpr vtable vtable(
        method_to_trait_t<Methods>::template create_invoker_for<T>()...);
    return &vtable;
  }

  template <typename Method> constexpr auto *get() const noexcept {
    constexpr int index = index_in_list<Method, Methods...>();
    return std::get<index>(m_functions);
  }

  std::tuple<method_ptr<Methods>...> m_functions;
};
} // namespace erased::details
