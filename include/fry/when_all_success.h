//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __FRY__WHEN_ALL_SUCCESS_H__
#define __FRY__WHEN_ALL_SUCCESS_H__

// when_all_success - returns a future that becomes ready when ALL of the input
//                    futures become success or when ANY of the input futures
//                    becomes failure.

#include <tuple>
#include "future_result.h"

namespace fry {

namespace detail { namespace all_success {
  //----------------------------------------------------------------------------
  template<typename Error, typename... Values>
  struct State {
    typedef std::tuple<Values...>               InputTuple;
    typedef std::tuple<replace_void<Values>...> OutputTuple;

    typedef Result<OutputTuple, Error>          OutputResult;

    template<std::size_t Index>
    using InputValue = tuple_element<Index, InputTuple>;

    template<std::size_t Index>
    using InputResult = Result<InputValue<Index>, Error>;

    //--------------------------------------------------------------------------
    template<std::size_t Index, typename Enable = void>
    struct Setter;

    template<std::size_t Index>
    struct Setter<Index, enable_if<!is_void<InputValue<Index>>{}>> {
      State& state;

      void operator () (const InputValue<Index>& value) {
        std::get<Index>(state.values) = value;
        state.on_success();
      }
    };

    template<std::size_t Index>
    struct Setter<Index, enable_if<is_void<InputValue<Index>>{}>> {
      State& state;

      void operator () () {
        std::get<Index>(state.values) = Void();
        state.on_success();
      }
    };

    //--------------------------------------------------------------------------
    std::mutex            mutex;
    OutputTuple           values;
    std::size_t           num_success;
    Promise<OutputResult> promise;

    //--------------------------------------------------------------------------
    State() : num_success(0) {}

    Future<OutputResult> get_future() {
      return promise.get_future();
    }

    template<std::size_t Index>
    void
    set(const InputResult<Index>& result)
    {
      std::lock_guard<std::mutex> guard(mutex);

      result.match(
          Setter<Index>{ *this }
        , [=](const Error& error) { promise.set_value(OutputResult(error)); }
      );
    }

    void on_success() {
      ++num_success;

      if (num_success == std::tuple_size<OutputTuple>::value) {
        promise.set_value(OutputResult(std::move(values)));
      }
    }
  };

  //----------------------------------------------------------------------------
  template<std::size_t Index, typename Error, typename... Values>
  struct Continuation {
    typedef Result<tuple_element<Index, std::tuple<Values...>>, Error>
            InputResult;

    std::shared_ptr<State<Error, Values...>> state;

    Continuation(std::shared_ptr<State<Error, Values...>> state)
      : state(state)
    {}

    void operator () (const InputResult& result) {
      state->template set<Index>(result);
    }
  };

  //----------------------------------------------------------------------------
  template<std::size_t Index, typename Error, typename... Values>
  enable_if<Index < sizeof...(Values)>
  assign( std::shared_ptr<State<Error, Values...>>       state
        , std::tuple<Future<Result<Values, Error>>...>&& fs)
  {
    std::get<Index>(fs).then(Continuation<Index, Error, Values...>(state));
    assign<Index + 1>(state, std::move(fs));
  }

  template<std::size_t Index, typename Error, typename... Values>
  enable_if<Index >= sizeof...(Values)>
  assign( std::shared_ptr<State<Error, Values...>>
        , std::tuple<Future<Result<Values, Error>>...>&&)
  {}

}} // namespace detail::all_success

template<typename Error, typename... Values>
Future<Result<std::tuple<replace_void<Values>...>, Error>>
when_all_success(Future<Result<Values, Error>>&&... fs) {
  return when_all_success(std::make_tuple(std::move(fs)...));
}

template<typename Error, typename... Values>
Future<Result<std::tuple<replace_void<Values>...>, Error> >
when_all_success(std::tuple<Future<Result<Values, Error>>...>&& fs) {
  auto state = std::make_shared<detail::all_success::State<Error, Values...>>();
  detail::all_success::assign<0>(state, std::move(fs));

  return state->get_future();
}

} // namespace fry

#endif // __FRY__WHEN_ALL_SUCCESS_H__
