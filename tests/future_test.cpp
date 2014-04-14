//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/test/unit_test.hpp>
#include <thread>

#include "test_helpers.h"
#include "fry/future.h"

using namespace std;
using namespace fry;

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_pending_future_calls_the_continuation_when_made_ready) {
  int probe = 1;

  Promise<void> promise;
  auto future = promise.get_future();

  BOOST_CHECK_EQUAL(1, probe);

  future.then([&]() {
    probe = 2;
  });

  BOOST_CHECK_EQUAL(1, probe);

  promise.set_value();

  BOOST_CHECK_EQUAL(2, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_ready_future_calls_the_continuation_immeditately) {
  int probe = 1;

  Promise<void> promise;
  auto future = promise.get_future();

  promise.set_value();

  BOOST_CHECK_EQUAL(1, probe);

  future.then([&]() {
    probe = 2;
  });

  BOOST_CHECK_EQUAL(2, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_setting_a_continuation_overwrites_previous_continuation) {
  int probe = 1;

  Promise<void> promise;
  auto future = promise.get_future();

  future.then([&]() {
    probe = 2;
  });

  future.then([&]() {
    probe = 3;
  });

  promise.set_value();

  BOOST_CHECK_EQUAL(3, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_setting_a_promise_value_more_than_once_has_no_effect) {
  int probe = 1;

  Promise<void> promise;
  auto future = promise.get_future();

  future.then([&]() {
    ++probe;
  });

  BOOST_CHECK_EQUAL(1, probe);

  promise.set_value();
  BOOST_CHECK_EQUAL(2, probe);

  promise.set_value();
  BOOST_CHECK_EQUAL(2, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_non_void_future) {
  int probe = 1;

  Promise<int> promise;
  auto future = promise.get_future();

  future.then([&](int i) {
    probe = i;
  });

  promise.set_value(66);

  BOOST_CHECK_EQUAL(66, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_chaining_futures) {
  int probe = 1;

  Promise<int> promise;
  auto future = promise.get_future();

  future.then([&](int i) {
    probe += i;
    return 2;
  }).then([&](int i) {
    probe += 2 * i;
    return 4;
  }).then([&](int i) {
    probe += 4 * i;
  }).then([&]() {
    probe += 1000;
  });

  promise.set_value(1);
  BOOST_CHECK_EQUAL(1022, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_future_returned_from_a_continuation_is_unwrapped) {
  int probe = 1;

  Promise<int> promise;
  auto future = promise.get_future();

  future.then([&](int i) {
    Promise<int> inner_promise;
    auto inner_future = inner_promise.get_future();

    inner_promise.set_value(i * 2);
    return inner_future;
  }).then([&](int i) {
    probe = i;
  });

  promise.set_value(2);

  BOOST_CHECK_EQUAL(4, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_make_ready_future) {
  int probe = 1;

  auto future = make_ready_future(2);

  future.then([&](int i) {
    probe = i;
  });

  BOOST_CHECK_EQUAL(2, probe);
}

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE(test_make_ready_future_unwraps_futures_passed_to_it) {
  int probe = 1;
  auto future1 = make_ready_future(2);
  auto future2 = make_ready_future(std::move(future1));

  future2.then([&](int i) {
    probe = i;
  });

  BOOST_CHECK_EQUAL(2, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_async_evaluation) {
  Locked<int> probe{1};

  Promise<void> promise;
  auto future = promise.get_future();

  future.then([&]() {
    probe = 2;
  });

  thread t([](Promise<void>&& promise) {
    promise.set_value();
  }, std::move(promise));

  BOOST_CHECK_EQUAL(1, probe);

  t.join();
  BOOST_CHECK_EQUAL(2, probe);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_continuation_is_called_from_the_same_thread_that_the_promise_value_is_set_in) {
  Locked<thread::id> id1;
  Locked<thread::id> id2;

  Promise<void> promise1;
  Promise<void> promise2;

  promise1.get_future().then([&]() {
    id1 = this_thread::get_id();
  });

  promise2.get_future().then([&]() {
    id2 = this_thread::get_id();
  });

  // thread 1
  promise1.set_value();

  // thread 2
  thread thread2([](Promise<void>&& promise) {
    promise.set_value();
  }, std::move(promise2));

  auto thread2_id = thread2.get_id();

  thread2.join();

  BOOST_CHECK_EQUAL(this_thread::get_id(), id1);
  BOOST_CHECK_EQUAL(thread2_id,            id2);
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(test_packaged_task) {
  Locked<int> probe{1};

  PackagedTask<int()> task([]() {
    return 2;
  });

  auto future = task.get_future();

  future.then([&](int i) {
    probe = i;
  });

  thread t(std::move(task));

  BOOST_CHECK_EQUAL(1, probe);

  t.join();
  BOOST_CHECK_EQUAL(2, probe);
}
