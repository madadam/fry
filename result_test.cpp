#include <boost/test/unit_test.hpp>
#include "result.h"

using namespace std;

//------------------------------------------------------------------------------
struct TestSuccess {};

struct TestError {};
static TestError error;

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_conversion_to_bool) {
  Result<int, TestError> success(1);
  Result<int, TestError> failure(error);

  BOOST_CHECK( success);
  BOOST_CHECK(!failure);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_if_success) {
  int probe = 0;
  Result<int, TestError> result(1);

  result.if_success([&](int) {
    probe = 1;
  });

  BOOST_CHECK_EQUAL(1, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_if_failure) {
  int probe = 0;
  Result<int, TestError> result(error);

  result.if_failure([&](TestError) {
    probe = -1;
  });

  BOOST_CHECK_EQUAL(-1, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_modify_value_if_success) {
  Result<int, TestError> result(1);

  result.if_success([](int& i) {
    i = 2;
  });

  result.if_success([](int i) {
    BOOST_CHECK_EQUAL(2, i);
  });
}
