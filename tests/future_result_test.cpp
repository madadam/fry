//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/test/unit_test.hpp>
#include "fry/future.h"
#include "fry/future_result.h"
#include "test_helpers.h"

using namespace fry;

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_success_continuation_returning_any_value_on_success) {
  Locked<int> probe{0};

  Promise<Result<int, TestError>> promise;
  auto future = promise.get_future();

  future.then([&](int value) {
    ++probe;
    return 2 * value;
  }).then([&](const Result<int, TestError>& result) {
    ++probe;
    BOOST_CHECK_EQUAL(make_result<TestError>(2000), result);
  });

  promise.set_value(make_result<TestError>(1000));

  BOOST_CHECK_EQUAL(2, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_success_continuation_returning_any_value_on_failure) {
  Locked<int> probe{0};

  Promise<Result<int, TestError>> promise;
  auto future = promise.get_future();

  Result<int, TestError> failure(error1);

  future.then([&](int value) {
    ++probe;
    return 2 * value;
  }).then([&](const Result<int, TestError>& result) {
    ++probe;
    BOOST_CHECK_EQUAL(failure, result);
  });

  promise.set_value(failure);

  BOOST_CHECK_EQUAL(1, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_success_continuation_returning_void_on_success) {
  Locked<int> probe{0};

  Promise<Result<int, TestError>> promise;
  auto future = promise.get_future();

  future.then([&](int value) {
    ++probe;
  }).then([&](const Result<void, TestError>& result) {
    ++probe;
    BOOST_CHECK_EQUAL(make_result<TestError>(), result);
  });

  promise.set_value(make_result<TestError>(1000));

  BOOST_CHECK_EQUAL(2, probe);
}

BOOST_AUTO_TEST_CASE(test_success_continuation_returning_void_on_failure) {
  Locked<int> probe{0};

  Promise<Result<int, TestError>> promise;
  auto future = promise.get_future();

  future.then([&](int value) {
    ++probe;
  }).then([&](const Result<void, TestError>& result) {
    ++probe;
    Result<void, TestError> failure(error1);
    BOOST_CHECK_EQUAL(failure, result);
  });

  promise.set_value(Result<int, TestError>(error1));

  BOOST_CHECK_EQUAL(1, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_success_continuation_returning_result_on_success) {
  Locked<int> probe{0};

  Promise<Result<int, TestError>> promise;
  auto future = promise.get_future();

  future.then([&](int value) {
    ++probe;
    return make_result<TestError>(2 * value);
  }).then([&](const Result<int, TestError>& result) {
    ++probe;
    BOOST_CHECK_EQUAL(make_result<TestError>(2000), result);
  });

  promise.set_value(make_result<TestError>(1000));

  BOOST_CHECK_EQUAL(2, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_success_continuation_returning_result_on_failure) {
  Locked<int> probe{0};
  Result<int, TestError> failure(error1);

  Promise<Result<int, TestError>> promise;
  auto future = promise.get_future();

  future.then([&](int value) {
    ++probe;
    return make_result<TestError>(2 * value);
  }).then([&](const Result<int, TestError>& result) {
    ++probe;
    BOOST_CHECK_EQUAL(failure, result);
  });

  promise.set_value(failure);

  BOOST_CHECK_EQUAL(1, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_success_continuation_returning_non_void_future_on_success) {
  Locked<int> probe{0};

  Promise<Result<int, TestError>> promise;
  auto future = promise.get_future();

  future.then([&](int value) {
    ++probe;
    return make_ready_future(2 * value);
  }).then([&](const Result<int, TestError>& result) {
    ++probe;
    BOOST_CHECK_EQUAL(make_result<TestError>(2000), result);
  });

  promise.set_value(make_result<TestError>(1000));

  BOOST_CHECK_EQUAL(2, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_success_continuation_returning_non_void_future_on_failure) {
  Locked<int> probe{0};
  auto failure = Result<int, TestError>(error1);

  Promise<Result<int, TestError>> promise;
  auto future = promise.get_future();

  future.then([&](int value) {
    ++probe;
    return make_ready_future(2 * value);
  }).then([&](const Result<int, TestError>& result) {
    ++probe;
    BOOST_CHECK_EQUAL(failure, result);
  });

  promise.set_value(failure);

  BOOST_CHECK_EQUAL(1, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_success_continuation_returning_void_future) {
  Locked<int> probe{0};

  Promise<Result<void, TestError>> promise;
  auto future = promise.get_future();

  future.then([&]() {
    ++probe;
    return make_ready_future();
  }).then([&](const Result<void, TestError>& result) {
    ++probe;
    BOOST_CHECK_EQUAL(make_result<TestError>(), result);
  });

  promise.set_value(make_result<TestError>());

  BOOST_CHECK_EQUAL(2, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_success_continuation_returning_result_future) {
  Locked<int> probe{0};

  Promise<Result<int, TestError>> promise;
  auto future = promise.get_future();

  future.then([&](int value) {
    ++probe;
    return make_ready_future(make_result<TestError>(2 * value));
  }).then([&](const Result<int, TestError>& result) {
    ++probe;
    BOOST_CHECK_EQUAL(make_result<TestError>(2000), result);
  });

  promise.set_value(make_result<TestError>(1000));

  BOOST_CHECK_EQUAL(2, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_failure_continuation_returning_any_value_on_success) {
  Locked<int> probe{0};

  Promise<Result<int, TestError>> promise;
  auto future = promise.get_future();

  future.then([&](TestError error) {
    ++probe;
    return 3000;
  }).then([&](const Result<int, TestError>& result) {
    ++probe;
    BOOST_CHECK_EQUAL(make_result<TestError>(1000), result);
  });

  promise.set_value(make_result<TestError>(1000));

  BOOST_CHECK_EQUAL(1, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_failure_continuation_returning_any_value_on_failure) {
  Locked<int> probe{0};

  Promise<Result<int, TestError>> promise;
  auto future = promise.get_future();

  future.then([&](TestError error) {
    ++probe;
    return 3000;
  }).then([&](const Result<int, TestError>& result) {
    ++probe;
    BOOST_CHECK_EQUAL(make_result<TestError>(3000), result);
  });

  promise.set_value(Result<int, TestError>(error1));

  BOOST_CHECK_EQUAL(2, probe);
}

////////////////////////////////////////////////////////////////////////////////

// TODO: failure_continuation_returning_future