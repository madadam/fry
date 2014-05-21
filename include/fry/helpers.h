//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __FRY__HELPERS_H__
#define __FRY__HELPERS_H__

#include <tuple>
#include <type_traits>

namespace fry {

////////////////////////////////////////////////////////////////////////////////
// Shortcut for std::enable_if<B, T>::type
template<bool B, typename T = void>
using enable_if = typename std::enable_if<B, T>::type;

// Shortcut for std::remove_reference<T>::type
template<typename T>
using remove_reference = typename std::remove_reference<T>::type;

// Shortcut for std::tuple_element<I, T>::type
template<std::size_t Index, typename Tuple>
using tuple_element = typename std::tuple_element<Index, Tuple>::type;

////////////////////////////////////////////////////////////////////////////////
namespace internal {
  template<typename F, typename... A>
  decltype(std::declval<F>()(std::declval<A>()...), std::true_type())
  can_call(int);

  template<typename F, typename... A>
  std::false_type
  can_call(...);
}

template<typename F, typename... A>
struct can_call : decltype(internal::can_call<F, A...>(0)) {};

template<typename F>
struct can_call<F, void> : decltype(internal::can_call<F>(0)) {};

////////////////////////////////////////////////////////////////////////////////
namespace internal {
  template<typename F, typename... Args> struct result_of {
    typedef typename std::result_of<F(Args...)>::type type;
  };

  template<typename F> struct result_of<F, void> {
    typedef typename std::result_of<F()>::type type;
  };
}

// result_of<T, A> is the same as std::result_of<T(A0)>::type, but it works
// also when A is void, in which case it is the same as
// std::result_of<T()>::type.
template<typename... T>
using result_of = typename internal::result_of<T...>::type;

////////////////////////////////////////////////////////////////////////////////
template<typename T> struct is_void : std::is_same<T, void> {};

// Dummy type used as a placeholder for void where void cannot be used.
struct Void {};

namespace internal {
  template<typename T> struct replace_void       { typedef T    type; };
  template<>           struct replace_void<void> { typedef Void type; };
}

// If T is void, evaluates to the Void dummy type, otherwise evaluates to T.
template<typename T>
using replace_void = typename internal::replace_void<T>::type;

} // namespace fry

#endif // __FRY__HELPERS_H__