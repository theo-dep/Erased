#pragma once

#include <any>
#include <array>
#include <tuple>
#include <type_traits>
#include <utility>

namespace erased {

struct erased_type_t {};

template <typename Method, typename Signature> struct MethodTraitImpl;

template <typename Method, typename ReturnType, typename ErasedType,
          typename... Args>
struct MethodTraitImpl<Method, ReturnType (*)(ErasedType &, Args...)> {
  using first_argument = std::conditional_t<std::is_const_v<ErasedType>,
                                            const std::any &, std::any &>;

  using function_pointer = ReturnType (*)(first_argument, Args...);

  template <typename T> static auto createInvoker() {
    return +[](first_argument value, Args... args) {
      return Method::invoker(*std::any_cast<T>(&value),
                             std::forward<Args>(args)...);
    };
  }
};

template <typename Method>
using MethodTrait =
    MethodTraitImpl<Method, decltype(&Method::template invoker<erased_type_t>)>;

template <typename Method>
using MethodPtr = typename MethodTrait<Method>::function_pointer;

template <typename T, typename... List> constexpr int index_in_list() {
  std::array array = {std::is_same_v<T, List>...};
  for (int i = 0; i < array.size(); ++i)
    if (array[i])
      return i;
  return -1;
}

template <typename... Methods> class erased : public Methods... {
private:
  std::any m_value;
  std::tuple<MethodPtr<Methods>...> m_pointers{};

public:
  template <typename T>
  erased(T x)
      : m_value(std::move(x)),
        m_pointers{MethodTrait<Methods>::template createInvoker<T>()...} {}

  template <typename Method, typename... Args>
  decltype(auto) invoke(Method, Args &&...args) const {
    return std::get<index_in_list<Method, Methods...>()>(m_pointers)(
        m_value, std::forward<Args>(args)...);
  }

  template <typename Method, typename... Args>
  decltype(auto) invoke(Method, Args &&...args) {
    return std::get<index_in_list<Method, Methods...>()>(m_pointers)(
        m_value, std::forward<Args>(args)...);
  }
};

} // namespace erased
