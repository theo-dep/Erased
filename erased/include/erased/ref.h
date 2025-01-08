#pragma once

#include "utils/utils.h"
#include <memory>
#include <tuple>

namespace erased {

template <typename... Methods> class ref : public Methods... {
  static constexpr auto all_const =
      (details::method_to_trait_t<Methods>::is_const && ...);

  using vtable = details::vtable<Methods...>;

public:
  template <typename T>
  constexpr ref(T &object) noexcept
      : m_ptr{std::addressof(object)},
        m_vtable{vtable::template construct_for<T>()} {}

  template <typename Method, typename... Args>
  constexpr decltype(auto) invoke(Method, Args &&...args) const {
    return m_vtable->template get<Method>()(m_ptr, static_cast<Args>(args)...);
  }

private:
  details::fast_conditional<all_const>::template apply<const void *, void *>
      m_ptr;
  const vtable *m_vtable;
};

} // namespace erased
