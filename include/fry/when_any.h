//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __FRY__WHEN_ANY_H__
#define __FRY__WHEN_ANY_H__

#include <atomic>
#include "helpers.h"

// when_any - returns a future that becomes ready when any of the input futures
//            becomes ready.

namespace fry {

namespace detail {
namespace any {

//------------------------------------------------------------------------------
template<typename T>
struct Continuation {
  struct State {
    Promise<T>        promise;
    std::atomic_flag  resolved;

    State() {
      resolved.clear();
    }
  };

  std::shared_ptr<State> state;

  Continuation() : state(std::make_shared<State>()) {}

  void operator () (const T& value) {
    // TODO: learn about the various memory order flags and use the most
    // appropriate one.
    if (!state->resolved.test_and_set()) {
      state->promise.set_value(value);
    }
  }

  Future<T> get_future() {
    return state->promise.get_future();
  }
};

//------------------------------------------------------------------------------
template<typename T, typename F, typename... Fs>
void assign(Continuation<T> handler, F&& first, Fs&&... rest)
{
  first.then(handler);
  assign(handler, std::move(rest)...);
}

template<typename T>
void assign(Continuation<T>) {};

} // namespace any

//------------------------------------------------------------------------------
template<typename R>
using value_type = typename std::iterator_traits<
                     decltype(std::begin(std::declval<R>()))
                   >::value_type;

} // namespace detail

////////////////////////////////////////////////////////////////////////////////
template<typename Range>
detail::value_type<Range> when_any(Range& futures) {
  using T = future_type<detail::value_type<Range>>;

  detail::any::Continuation<T> handler;

  for (auto&& future : futures) {
    future.then(handler);
  }

  return handler.get_future();
}

template<typename Future, typename... Futures>
typename std::common_type<Future, Futures...>::type
when_any(Future&& f0, Future&& f1, Futures&&... fs)
{
  using T = future_type<typename std::common_type<Future, Futures...>::type>;

  detail::any::Continuation<T> handler;
  detail::any::assign(handler, std::move(f0), std::move(f1), std::move(fs)...);

  return handler.get_future();
}

} // namespace fry

#endif // __FRY__WHEN_ANY_H__