//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/test/unit_test.hpp>

#include "test_helpers.h"
#include "fry/future.h"
#include "fry/combinators.h"

using namespace std;
using namespace fry;

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_when_any) {
  Locked<bool> called{false};

  Promise<int> p1;
  Promise<int> p2;

  auto f1 = p1.get_future();
  auto f2 = p2.get_future();

  when_any(f1, f2).then([&](int value) {
    called = true;
    BOOST_CHECK_EQUAL(1000, value);
  });

  p1.set_value(1000);
  p2.set_value(2000);

  BOOST_CHECK(called);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_when_any_with_range) {
  Locked<bool> called{false};

  std::vector<Promise<int>> promises(10);
  std::vector<Future<int>> futures;

  for (auto& promise : promises) {
    futures.push_back(promise.get_future());
  }

  when_any(futures).then([&](int value) {
    called = true;
    BOOST_CHECK_EQUAL(1000, value);
  });

  int index = 0;

  for (auto& promise : promises) {
    promise.set_value(++index * 1000);
  }

  BOOST_CHECK(called);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_when_all) {
  Locked<bool> called{false};

  Promise<int> p1;
  Promise<int> p2;

  auto f1 = p1.get_future();
  auto f2 = p2.get_future();

  when_all(f1, f2).then([&](const tuple<int, int>& values) {
    called = true;
    BOOST_CHECK_EQUAL(1000, get<0>(values));
    BOOST_CHECK_EQUAL(2000, get<1>(values));
  });

  BOOST_CHECK(!called);

  p1.set_value(1000);
  BOOST_CHECK(!called);

  p2.set_value(2000);
  BOOST_CHECK(called);
}

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


