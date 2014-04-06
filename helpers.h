#ifndef __HELPERS_H__
#define __HELPERS_H__

////////////////////////////////////////////////////////////////////////////////
// Shortcut for std::enable_if<B, T>::type
template<bool B, typename T = void>
using enable_if_t = typename std::enable_if<B, T>::type;

////////////////////////////////////////////////////////////////////////////////
namespace detail {
  template<typename F, typename... A>
  decltype(std::declval<F>()(std::declval<A>()...), std::true_type())
  can_call(int);

  template<typename F, typename... A>
  std::false_type
  can_call(...);
}

template<typename F, typename... A>
struct can_call : decltype(detail::can_call<F, A...>(0)) {};

template<typename F>
struct can_call<F, void> : decltype(detail::can_call<F>(0)) {};

#endif // __HELPERS_H__