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
    typedef std::tuple<Values...> Tuple;
    typedef Result<Tuple, Error>  TupleResult;

    template<std::size_t Index>
    using ElementResult = Result<tuple_element<Index, Tuple>, Error>;

    std::mutex            mutex;
    Tuple                 values;
    std::size_t           num_resolved;
    Promise<TupleResult>  promise;

    State() : num_resolved(0) {}

    Future<TupleResult> get_future() {
      return promise.get_future();
    }

    template<std::size_t Index>
    void
    set(const ElementResult<Index>& result)
    {
      std::lock_guard<std::mutex> guard(mutex);

      result.match(
          [=](const tuple_element<Index, Tuple>& value) {
            std::get<Index>(values) = value;
            ++num_resolved;

            if (num_resolved == std::tuple_size<Tuple>::value) {
              promise.set_value(TupleResult(std::move(values)));
            }
          }
        , [=](const Error& error) {
            promise.set_value(TupleResult(error));
          }
      );
    }
  };

  //----------------------------------------------------------------------------
  template<std::size_t Index, typename Error, typename... Values>
  struct Continuation {
    typedef Result<tuple_element<Index, std::tuple<Values...>>, Error>
            ElementResult;

    std::shared_ptr<State<Error, Values...>> state;

    Continuation(std::shared_ptr<State<Error, Values...>> state)
      : state(state)
    {}

    void operator () (const ElementResult& result) {
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
Future<Result<std::tuple<Values...>, Error>>
when_all_success(Future<Result<Values, Error>>&&... fs) {
  return when_all_success(std::make_tuple(std::move(fs)...));
}

template<typename Error, typename... Values>
Future<Result<std::tuple<Values...>, Error> >
when_all_success(std::tuple<Future<Result<Values, Error>>...>&& fs) {
  auto state = std::make_shared<detail::all_success::State<Error, Values...>>();
  detail::all_success::assign<0>(state, std::move(fs));

  return state->get_future();
}

} // namespace fry

#endif // __FRY__WHEN_ALL_SUCCESS_H__
