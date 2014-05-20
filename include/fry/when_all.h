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

#include <tuple>
#include "helpers.h"

namespace fry {

namespace detail {
  //----------------------------------------------------------------------------
  template<std::size_t Index, typename Tuple>
  using tuple_element_t = typename std::tuple_element<Index, Tuple>::type;

  //----------------------------------------------------------------------------
  template<typename... Ts>
  struct AllContinuationState {
    typedef std::tuple<Ts...> Tuple;

    std::mutex        mutex;
    Tuple             values;
    std::size_t       num_resolved;
    Promise<Tuple>    promise;

    AllContinuationState() : num_resolved(0) {}

    Future<Tuple> get_future() {
      return promise.get_future();
    }

    template<std::size_t Index>
    void set_value(const tuple_element_t<Index, Tuple>& value) {
      std::lock_guard<std::mutex> guard(mutex);

      std::get<Index>(values) = value;
      ++num_resolved;

      if (num_resolved == std::tuple_size<Tuple>::value) {
        promise.set_value(std::move(values));
      }
    }
  };

  template<std::size_t Index, typename... Ts>
  struct AllContinuation {
    typedef tuple_element_t<Index, std::tuple<Ts...>> Value;

    std::shared_ptr<AllContinuationState<Ts...>> state;

    AllContinuation(std::shared_ptr<AllContinuationState<Ts...>> state)
      : state(state)
    {}

    void operator () (const Value& value) {
      state->set_value<Index>(value);
    }
  };

  //----------------------------------------------------------------------------
  template<std::size_t Index, typename... Ts>
  enable_if<Index < sizeof...(Ts)>
  assign_all_handler( std::shared_ptr<AllContinuationState<Ts...>> state
                    , const std::tuple<Future<Ts>&...>& fs)
  {
    std::get<Index>(fs).then(AllContinuation<Index, Ts...>(state));
    assign_all_handler<Index + 1, Ts...>(state, fs);
  }

  template<std::size_t Index, typename... Ts>
  enable_if<Index >= sizeof...(Ts)>
  assign_all_handler( std::shared_ptr<AllContinuationState<Ts...>>
                    , const std::tuple<Future<Ts>&...>&)
  {}
}

////////////////////////////////////////////////////////////////////////////////
template<typename... Ts>
Future<std::tuple<Ts...>> when_all(Future<Ts>&... fs) {
  return when_all(std::tie(fs...));
}

template<typename... Ts>
Future<std::tuple<Ts...>> when_all(const std::tuple<Future<Ts>&...>& fs) {
  auto state = std::make_shared<detail::AllContinuationState<Ts...>>();

  detail::assign_all_handler<0>(state, fs);

  return state->get_future();
}

} // namespace fry

#endif // __FRY__WHEN_ALL_H__