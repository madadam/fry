//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __FRY__HELPERS_H__
#define __FRY__HELPERS_H__

#include <type_traits>

namespace fry {

////////////////////////////////////////////////////////////////////////////////
// Shortcut for std::enable_if<B, T>::type
template<bool B, typename T = void>
using enable_if = typename std::enable_if<B, T>::type;

// Shortcut for std::remove_reference<T>::type
template<typename T>
using remove_reference = typename std::remove_reference<T>::type;

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
  //----------------------------------------------------------------------------
  template<typename F, typename... Args> struct result_of {
    typedef typename std::result_of<F(Args...)>::type type;
  };

  template<typename F> struct result_of<F, void> {
    typedef typename std::result_of<F()>::type type;
  };

  template<typename F, typename... Args> struct checked_result_of {
    static_assert(::fry::can_call<F, Args...>{}, "F cannot be called with arguments of types Args...");
    typedef typename result_of<F, Args...>::type type;
  };
}

//------------------------------------------------------------------------------
// result_of<T, A> is the same as std::result_of<T(A0)>::type, but it works
// also when A is void, in which case it is the same as
// std::result_of<T()>::type.
template<typename... T>
using result_of = typename internal::checked_result_of<T...>::type;

////////////////////////////////////////////////////////////////////////////////
template<typename T> struct is_void : std::is_same<T, void> {};

} // namespace fry

#endif // __FRY__HELPERS_H__