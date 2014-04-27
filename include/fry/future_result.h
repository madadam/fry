//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __FRY__FUTURE_RESULT_H__
#define __FRY__FUTURE_RESULT_H__

// Future<Result<T, E>> specialization.

#include "future.h"
#include "result.h"

namespace fry {

////////////////////////////////////////////////////////////////////////////////
namespace detail {

  //----------------------------------------------------------------------------
  // Turns Result<Future<T>, E> into Future<Result<T, E>>.
  template<typename T, typename E>
  Future<add_result<T, E>> flip(const Result<Future<T>, E>& result) {
    return result.match(
      [](const Future<T>& future) {
        // HACK: Result::match currently does not have a non-const overload.
        // As a temporary workaroud, we cast away the constness. It is nasty,
        // but should be safe in this case.
        return const_cast<Future<T>&>(future).then(ResultMaker<E>());
      },
      [](const E& error) {
        return ::fry::make_ready_future(add_result<T, E>{ error });
      }
    );
  }

  //----------------------------------------------------------------------------
  template<typename E>
  struct ReadyFutureResultMaker {
    template<typename T>
    auto operator () (T&& value) const
    -> decltype(
      ::fry::make_ready_future(::fry::make_result<E>(std::forward<T>(value)))
    )
    {
      return ::fry::make_ready_future(
        ::fry::make_result<E>(std::forward<T>(value))
      );
    }

    Future<Result<void, E>> operator () () const {
      return ::fry::make_ready_future(::fry::make_result<E>());
    }
  };

  //----------------------------------------------------------------------------
  template<typename F>
  struct OnSuccess {
    F fun;

    template< typename T, typename E
            , typename = enable_if<!is_future<result_of<F, T>>{}>>
    auto operator () (const Result<T, E>& input) const
    -> decltype(input.if_success(fun))
    {
      return input.if_success(fun);
    }

    template< typename T, typename E
            , typename = enable_if<is_future<result_of<F, T>>{}>>
    auto operator () (const Result<T, E>& input) const
    -> decltype(flip(input.if_success(fun)))
    {
      return flip(input.if_success(fun));
    }
  };

  //----------------------------------------------------------------------------
  template<typename F>
  struct OnFailure {
    F fun;

    template< typename T, typename E
            , typename = enable_if<!is_future<result_of<F, E>>{}>>
    auto operator () (const Result<T, E>& input) const
    -> decltype(input.if_failure(fun))
    {
      return input.if_failure(fun);
    }

    template< typename T, typename E
            , typename = enable_if<is_future<result_of<F, E>>{}>>
    Future<Result<T, E>> operator () (const Result<T, E>& input) const
    {
      return input.match(
        ReadyFutureResultMaker<E>(),
        [=](const E& error) { return fun(error).then(ResultMaker<E>()); }
      );
    }
  };

  //----------------------------------------------------------------------------
  template<typename F>
  struct Always {
    F fun;

    template<typename T>
    result_of<F> operator () (T&&) const {
      return fun();
    }
  };

}

////////////////////////////////////////////////////////////////////////////////
template<typename T, typename E>
class Future<Result<T, E>> {
private:
  typedef Future<Result<T, E>>        This;
  typedef detail::State<Result<T, E>> State;

  template<typename F>
  add_future<result_of<F, Result<T, E>>> _then(F&& fun) {
    assert(_state);
    return _state->set_continuation(std::forward<F>(fun));
  }

public:
  Future(const Future<Result<T, E>>&) = delete;
  Future(Future<Result<T, E>>&& other) = default;

  // set continuation that accepts Result<T, E>
  template< typename F
          , typename = enable_if<!can_call<F, T>{} && !can_call<F, E>{}>>
  auto then(F&& fun)
  ->decltype(std::declval<This>()._then(fun))
  {
    return _then(std::forward<F>(fun));
  }

  // set continuation that accepts T (value)
  template<typename F, typename = enable_if<can_call<F, T>{}>>
  auto then(F&& fun)
  -> decltype(std::declval<This>()._then(detail::OnSuccess<F>{ fun }))
  {
    return _then(detail::OnSuccess<F>{ std::forward<F>(fun) });
  }

  // set continuation that accepts E (error)
  template<typename F, typename = enable_if<can_call<F, E>{}>>
  auto then(F&& fun)
  -> decltype(std::declval<This>()._then(detail::OnFailure<F>{ fun }))
  {
    return _then(detail::OnFailure<F>{ std::forward<F>(fun) });
  }

  // sets a continuation that is called no matter if the result is success
  // or failure. The continuation does not take any arguments.
  template<typename F>
  auto always(F&& fun)
  -> decltype(std::declval<This>()._then(detail::Always<F>{ fun }))
  {
    return _then(detail::Always<F>{ std::forward<F>(fun) });
  }

private:

  Future(std::shared_ptr<State> state)
    : _state(state)
  {}

private:

  std::shared_ptr<State> _state;

  friend class Promise<Result<T, E>>;
};

} // namespace fry

#endif // __FRY__FUTURE_RESULT_H__