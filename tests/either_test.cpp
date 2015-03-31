//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/test/unit_test.hpp>
#include "fry/either.h"
#include "test_helpers.h"

using namespace fry;

struct Foo {};
struct Bar {};

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_match_returning_void) {
  int result = 0;

  Either<Foo, Bar> e{ Foo{} };

  e.match( [&](Foo) { result = 1; }
         , [&](Bar) { result = 2; });

  BOOST_CHECK_EQUAL(1, result);
}
