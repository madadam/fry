#include <boost/test/unit_test.hpp>
#include "future.h"
#include "result_future.h"
#include "test_helpers.h"

// on_success returns: - void
//                     - Future<Result<T, E>>

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
BOOST_AUTO_TEST_CASE(test_on_success_with_future) {
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

