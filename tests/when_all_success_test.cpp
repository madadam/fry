//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/test/unit_test.hpp>

#include "test_helpers.h"
#include "fry/future_result.h"
#include "fry/when_all_success.h"

using namespace std;
using namespace fry;

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_when_all_success_on_success) {
  Locked<bool> success_called{false};
  Locked<bool> failure_called{false};

  Promise<Result<int, TestError>> p1;
  Promise<Result<int, TestError>> p2;

  auto f1 = p1.get_future();
  auto f2 = p2.get_future();

  when_all_success(
    std::move(f1), std::move(f2)
  ).then([&](const tuple<int, int>& result) {
    success_called = true;
    BOOST_CHECK_EQUAL(1000, get<0>(result));
    BOOST_CHECK_EQUAL(2000, get<1>(result));
  }).then([&](TestError) {
    failure_called = true;
  });

  BOOST_CHECK(!success_called);
  BOOST_CHECK(!failure_called);

  p1.set_value(Result<int, TestError>(1000));
  BOOST_CHECK(!success_called);
  BOOST_CHECK(!failure_called);

  p2.set_value(Result<int, TestError>(2000));
  BOOST_CHECK(success_called);
  BOOST_CHECK(!failure_called);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_when_all_success_on_failure) {
  Locked<bool> success_called{false};
  Locked<bool> failure_called{false};

  Promise<Result<int, TestError>> p1;
  Promise<Result<int, TestError>> p2;

  auto f1 = p1.get_future();
  auto f2 = p2.get_future();

  when_all_success(
    std::move(f1), std::move(f2)
  ).then([&](const tuple<int, int>&) {
    success_called = true;
  }).then([&](TestError error) {
    failure_called = true;
    BOOST_CHECK_EQUAL(error1, error);
  });

  BOOST_CHECK(!success_called);
  BOOST_CHECK(!failure_called);

  p1.set_value(Result<int, TestError>(error1));
  BOOST_CHECK(!success_called);
  BOOST_CHECK(failure_called);
}
