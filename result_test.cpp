#include <boost/test/unit_test.hpp>
#include "result.h"
#include "test_helpers.h"

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_conversion_to_bool_of_non_void_result) {
  Result<int, TestError> success(1);
  Result<int, TestError> failure(error1);

  BOOST_CHECK( success);
  BOOST_CHECK(!failure);
}
////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_conversion_to_bool_of_void_result) {
  Result<void, TestError> success;
  Result<void, TestError> failure(error1);

  BOOST_CHECK( success);
  BOOST_CHECK(!failure);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_equality_and_inequality) {
  Result<int, TestError> success1(1);
  Result<int, TestError> success2(2);
  Result<int, TestError> failure1(error1);
  Result<int, TestError> failure2(error2);

  BOOST_CHECK(success1 == success1);
  BOOST_CHECK(success1 != success2);
  BOOST_CHECK(success1 != failure1);
  BOOST_CHECK(failure1 != failure2);
  BOOST_CHECK(failure1 == failure1);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_if_success) {
  int probe = 0;
  Result<int, TestError> result1(1);

  auto result2 = result1.if_success([&](int value) {
    probe = value;
  });

  BOOST_CHECK_EQUAL(1, probe);
  BOOST_CHECK(result2);

  auto result3 = result2.if_success([&]() {
    probe = 2;
    return 3;
  });

  BOOST_CHECK_EQUAL(2, probe);
  BOOST_CHECK(result3);

  auto result4 = result3.if_success([&](int value) {
    probe = value;
    return make_result<TestError>(4);
  });

  BOOST_CHECK_EQUAL(3, probe);
  BOOST_CHECK(result4);

  auto result5 = result4.if_success([&](int value) {
    probe = value;
    return Result<void, TestError>(error1);
  });

  BOOST_CHECK_EQUAL(4, probe);
  BOOST_CHECK(!result5);

  auto result6 = result5.if_success([&]() {
    BOOST_CHECK(false);
  });

  BOOST_CHECK(!result6);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_if_failure) {
  int probe = 0;
  Result<int, TestError> result1(error1);

  auto result2 = result1.if_failure([&](TestError error) {
    BOOST_CHECK_EQUAL(error1, error);
    probe = 1;
  });

  BOOST_CHECK_EQUAL(1, probe);
  BOOST_CHECK(!result2);

  auto result3 = result2.if_failure([&](TestError error) {
    BOOST_CHECK_EQUAL(error1, error);
    probe = 2;
  });

  BOOST_CHECK_EQUAL(2, probe);

  auto result4 = result3.if_failure([](TestError) {
    return Result<int, TestError>(error2);
  });

  BOOST_CHECK(!result4);

  auto result5 = result4.if_failure([&](TestError error) {
    BOOST_CHECK_EQUAL(error2, error);
    probe = 3;

    return 42;
  });

  BOOST_CHECK_EQUAL(3, probe);
  BOOST_CHECK(result5);

  result5.if_failure([&](TestError) {
    probe = 4;
  });

  BOOST_CHECK_EQUAL(3, probe);
}
