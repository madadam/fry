#ifndef __RESULT_FUTURE_H__
#define __RESULT_FUTURE_H__

// Utilities to allow convenient use of Futures and Results together.

#include "helpers.h"
#include "result.h"

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
          // return Result<R, E>(result);
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

#endif // __RESULT_FUTURE_H__