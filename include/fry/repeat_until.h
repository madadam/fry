//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __FRY__REPEAT_UNTIL_H__
#define __FRY__REPEAT_UNTIL_H__

// repeat_until - repeatedly calls the given future-returing action until the
//                future resolves to a value for which the given predicate
//                returns true.

#include "helpers.h"

namespace fry {

namespace detail {
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

#endif // __FRY__REPEAT_UNTIL_H__