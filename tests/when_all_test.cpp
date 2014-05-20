//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/test/unit_test.hpp>

#include "test_helpers.h"
#include "fry/future.h"
#include "fry/when_all.h"

using namespace std;
using namespace fry;

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_when_all) {
  Locked<bool> called{false};

  Promise<int> p1;
  Promise<int> p2;

  auto f1 = p1.get_future();
  auto f2 = p2.get_future();

  when_all(std::move(f1), std::move(f2)).then([&](const tuple<int, int>& values) {
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
