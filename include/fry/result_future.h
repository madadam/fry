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
    return input.if_success(fun).match(
      [](const Future<R>& future) {
        // HACK: Result::match currently does not have a non-const overload.
        // As a temporary workaroud, we cast away the constness. It is nasty,
        // but should be safe in this case.
        return const_cast<Future<R>&>(future).then(ResultMaker<E>());
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
template<typename F>
struct OnFailure {
  F fun;

  //----------------------------------------------------------------------------
  template< typename T, typename E
          , typename = enable_if<!is_future<result_of<F, E>>{}>>
  auto operator () (const Result<T, E>& input) const
  -> decltype(input.if_failure(fun))
  {
    return input.if_failure(fun);
  }

};

template<typename F>
OnFailure<F> on_failure(F&& fun) {
  return { std::forward<F>(fun) };
}

} // namespace fry

#endif // __FRY__RESULT_FUTURE_H__