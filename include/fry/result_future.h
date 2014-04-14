//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __FRY__RESULT_FUTURE_H__
#define __FRY__RESULT_FUTURE_H__

// Utilities to allow convenient use of Futures and Results together.

#include "helpers.h"
#include "result.h"

namespace fry {

////////////////////////////////////////////////////////////////////////////////
// Function adaptor that takes a callable with no arguments and produces a
// function object that takes a Result, but ignores it and always returns the
// result of the original callable.
template<typename F>
struct Always {
  F fun;

  template<typename T, typename E>
  result_of<F> operator () (const Result<T, E>&) const {
    return fun();
  }
};

template<typename F>
Always<F> always(F&& fun) {
  return { std::forward<F>(fun) };
}

////////////////////////////////////////////////////////////////////////////////
template<typename F>
struct OnSuccess {
  F fun;

  //----------------------------------------------------------------------------
  template< typename T, typename E
          , typename = enable_if<!is_future<result_of<F, T>>{}>>
  auto operator () (const Result<T, E>& input) const
  -> decltype(input.if_success(fun))
  {
    return input.if_success(fun);
  }

  //----------------------------------------------------------------------------
  template< typename T, typename E
          , typename R = remove_reference<remove_future<result_of<F, T>>>
          , typename = enable_if<is_future<result_of<F, T>>{}>>
  add_future<add_result<R, E>> operator () (const Result<T, E>& input) const {
    return input.match(
      [this](const T& value) {
        return fun(value).then([](const R& result) {
          return make_result<E>(result);
        });
      },
      [](const E& error) {
        return make_ready_future(add_result<R, E>{ error });
      }
    );
  }
};

template<typename F>
OnSuccess<F> on_success(F&& fun) {
  return { std::forward<F>(fun) };
}

////////////////////////////////////////////////////////////////////////////////
// TODO: OnFailure

} // namespace fry

#endif // __FRY__RESULT_FUTURE_H__