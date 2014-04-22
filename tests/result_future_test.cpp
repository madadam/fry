//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/test/unit_test.hpp>
#include "fry/future.h"
#include "fry/result_future.h"
#include "test_helpers.h"

using namespace fry;

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_on_success_with_any_value) {
  int probe = 0;

  auto f = on_success([&](int value) {
    ++probe;
    return 2 * value;
  });

  BOOST_CHECK_EQUAL(make_result<TestError>(4), f(make_result<TestError>(2)));
  BOOST_CHECK_EQUAL(1, probe);

  auto failure = Result<int, TestError>(error1);

  BOOST_CHECK_EQUAL(failure, f(failure));
  BOOST_CHECK_EQUAL(1, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_on_success_with_void) {
  int probe = 0;

  auto f = on_success([&]() {
    ++probe;
  });

  BOOST_CHECK_EQUAL(make_result<TestError>(), f(make_result<TestError>()));
  BOOST_CHECK_EQUAL(1, probe);

  auto failure = Result<void, TestError>(error1);

  BOOST_CHECK_EQUAL(failure, f(failure));
  BOOST_CHECK_EQUAL(1, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_on_success_with_result) {
  int probe = 0;
  auto failure = Result<int, TestError>(error1);

  auto f_success = on_success([&](int value) {
    probe += 1;
    return make_result<TestError>(2 * value);
  });

  auto f_failure = on_success([&](int value) {
    probe += 2;
    return failure;
  });


  BOOST_CHECK_EQUAL(make_result<TestError>(4), f_success(make_result<TestError>(2)));
  BOOST_CHECK_EQUAL(1, probe);

  BOOST_CHECK_EQUAL(failure, f_success(failure));
  BOOST_CHECK_EQUAL(1, probe);

  BOOST_CHECK_EQUAL(failure, f_failure(make_result<TestError>(2)));
  BOOST_CHECK_EQUAL(3, probe);

  BOOST_CHECK_EQUAL(failure, f_failure(failure));
  BOOST_CHECK_EQUAL(3, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_on_success_with_future_using_non_void_result) {
  Locked<int> probe{0};
  auto failure = Result<int, TestError>(error1);

  auto f = on_success([&](int value) {
    ++probe;
    return make_ready_future(2 * value);
  });

  f(make_result<TestError>(2)).then([&](const Result<int, TestError>& result) {
    ++probe;
    BOOST_CHECK_EQUAL(make_result<TestError>(4), result);
  });

  BOOST_CHECK_EQUAL(2, probe);

  f(failure).then([&](const Result<int, TestError>& result) {
    ++probe;
    BOOST_CHECK_EQUAL(failure, result);
  });

  BOOST_CHECK_EQUAL(3, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_on_success_with_future_using_void_result) {
  Locked<int> probe{0};

  auto f = on_success([&]() {
    ++probe;
    return make_ready_future();
  });

  f(make_result<TestError>()).then([&](const Result<void, TestError>& result) {
    ++probe;
    BOOST_CHECK_EQUAL(make_result<TestError>(), result);
  });

  BOOST_CHECK_EQUAL(2, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_on_success_with_future_of_result) {
  Locked<int> probe{0};

  auto f = on_success([&](int value) {
    ++probe;
    return make_ready_future(make_result<TestError>(2 * value));
  });

  f(make_result<TestError>(2)).then([&](const Result<int, TestError>& result) {
    ++probe;
    BOOST_CHECK_EQUAL(make_result<TestError>(4), result);
  });

  BOOST_CHECK_EQUAL(2, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_on_failure_with_any_value) {
  int probe = 0;

  auto f = on_failure([&](TestError error) {
    ++probe;
    return 3;
  });

  BOOST_CHECK_EQUAL(make_result<TestError>(2), f(make_result<TestError>(2)));
  BOOST_CHECK_EQUAL(0, probe);

  auto failure = Result<int, TestError>(error1);

  BOOST_CHECK_EQUAL(make_result<TestError>(3), f(failure));
  BOOST_CHECK_EQUAL(1, probe);
}
