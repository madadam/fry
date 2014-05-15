#ifndef __FRY__COMBINATORS_H__
#define __FRY__COMBINATORS_H__

// Future combinators:
//
// when_any - returns a future that becomes ready when any of the input futures
//            becomes ready.
//
// when_all - returns a future that becomes ready when all of the input futures
//            become ready.
//
// repeat_until - repeatedly calls the given future-returing action until the
//                future resolves to a value for which the given predicate
//                returns true.

#include <atomic>
#include <tuple>
#include "helpers.h"

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

  //----------------------------------------------------------------------------
  template<typename Action, typename Predicate>
  struct Repeater {
    typedef result_of<Action>   Future;
    typedef future_type<Future> Value;

    Action          action;
    Predicate       predicate;
    Promise<Value>  promise;

    Repeater(Action action, Predicate predicate)
      : action(std::move(action))
      , predicate(std::move(predicate))
    {}

    Future start() {
      loop();
      return promise.get_future();
    }

    void loop() {
      action().then([=](const Value& value) {
        if (predicate(value)) {
          promise.set_value(value);
        } else {
          loop();
        }
      });
    }
  };
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

////////////////////////////////////////////////////////////////////////////////
template< typename Action, typename Predicate
        , typename = enable_if<is_future<result_of<Action>>{}>>
result_of<Action> repeat_until(Action&& action, Predicate&& predicate) {
  using Value = future_type<result_of<Action>>;
  using Repeater = detail::Repeater< typename std::decay<Action>::type
                                   , typename std::decay<Predicate>::type>;

  auto repeater = std::make_shared<Repeater>(
      std::forward<Action>(action)
    , std::forward<Predicate>(predicate));

  return repeater->start().then([repeater](const Value& value) {
    return value;
  });
}

} // namespace fry

#endif // __FRY__COMBINATORS_H__