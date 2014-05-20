//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/test/unit_test.hpp>

#include "test_helpers.h"
#include "fry/future.h"
#include "fry/repeat_until.h"

using namespace std;
using namespace fry;

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_repeat_until) {
  Locked<int>  counter{0};
  Locked<bool> called{false};

  auto action = [&]() {
    ++counter;
    return make_ready_future((int) counter);
  };

  repeat_until(action, [](int value) {
    return value > 10;
  }).then([&](int value) {
    called = true;
    BOOST_CHECK_EQUAL(value, 11);
  });

  BOOST_CHECK(called);
}


