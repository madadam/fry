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
  //----------------------------------------------------------------------------
  template<typename T>
  struct AnyContinuation {
    struct State {
      Promise<T>        promise;
      std::atomic_flag  resolved;

      State() {
        resolved.clear();
      }
    };

    std::shared_ptr<State> state;

    AnyContinuation() : state(std::make_shared<State>()) {}

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

  //----------------------------------------------------------------------------
  template<typename T, typename F, typename... Fs>
  void assign_any_handler(AnyContinuation<T> handler, F& first, Fs&... rest)
  {
    first.then(handler);
    assign_any_handler(handler, rest...);
  }

  template<typename T>
  void assign_any_handler(AnyContinuation<T>) {};

  //----------------------------------------------------------------------------
  template<typename R>
  using value_type = typename std::iterator_traits<
                       decltype(std::begin(std::declval<R>()))
                     >::value_type;
}

////////////////////////////////////////////////////////////////////////////////
template<typename Range>
detail::value_type<Range> when_any(Range& futures) {
  using T = future_type<detail::value_type<Range>>;

  detail::AnyContinuation<T> handler;

  for (auto&& future : futures) {
    future.then(handler);
  }

  return handler.get_future();
}

template<typename Future, typename... Futures>
typename std::common_type<Future, Futures...>::type
when_any(Future& f0, Future& f1, Futures&... fs)
{
  using T = future_type<typename std::common_type<Future, Futures...>::type>;

  detail::AnyContinuation<T> handler;
  detail::assign_any_handler(handler, f0, f1, fs...);

  return handler.get_future();
}

} // namespace fry

#endif // __FRY__WHEN_ANY_H__