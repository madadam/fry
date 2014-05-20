//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __FRY__WHEN_ALL_H__
#define __FRY__WHEN_ALL_H__

// when_all - returns a future that becomes ready when all of the input futures
//            become ready.

#include "helpers.h"

namespace fry {

namespace detail { namespace all {
  //----------------------------------------------------------------------------
  template<typename... Ts>
  struct State {
    typedef std::tuple<Ts...> Tuple;

    std::mutex        mutex;
    Tuple             values;
    std::size_t       num_resolved;
    Promise<Tuple>    promise;

    State() : num_resolved(0) {}

    Future<Tuple> get_future() {
      return promise.get_future();
    }

    template<std::size_t Index>
    void set(const tuple_element<Index, Tuple>& value) {
      std::lock_guard<std::mutex> guard(mutex);

      std::get<Index>(values) = value;
      ++num_resolved;

      if (num_resolved == std::tuple_size<Tuple>::value) {
        promise.set_value(std::move(values));
      }
    }
  };

  template<std::size_t Index, typename... Ts>
  struct Continuation {
    typedef tuple_element<Index, std::tuple<Ts...>> Value;

    std::shared_ptr<State<Ts...>> state;

    Continuation(std::shared_ptr<State<Ts...>> state)
      : state(state)
    {}

    void operator () (const Value& value) {
      state->template set<Index>(value);
    }
  };

  //----------------------------------------------------------------------------
  template<std::size_t Index, typename... Ts>
  enable_if<Index < sizeof...(Ts)>
  assign( std::shared_ptr<State<Ts...>> state
        , std::tuple<Future<Ts>...>&&   fs)
  {
    std::get<Index>(fs).then(Continuation<Index, Ts...>(state));
    assign<Index + 1>(state, std::move(fs));
  }

  template<std::size_t Index, typename... Ts>
  enable_if<Index >= sizeof...(Ts)>
  assign(std::shared_ptr<State<Ts...>>, std::tuple<Future<Ts>...>&&)
  {}
}} // namespace detail::all

////////////////////////////////////////////////////////////////////////////////
template<typename... Ts>
Future<std::tuple<Ts...>> when_all(Future<Ts>&&... fs) {
  return when_all(std::make_tuple(std::move(fs)...));
}

template<typename... Ts>
Future<std::tuple<Ts...>> when_all(std::tuple<Future<Ts>...>&& fs) {
  auto state = std::make_shared<detail::all::State<Ts...>>();
  detail::all::assign<0>(state, std::move(fs));

  return state->get_future();
}

} // namespace fry

#endif // __FRY__WHEN_ALL_H__