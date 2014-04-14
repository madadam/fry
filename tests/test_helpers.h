//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __TEST_HELPERS_H__
#define __TEST_HELPERS_H__

#include <mutex>
#include <type_traits>
#include "fry/result.h"

////////////////////////////////////////////////////////////////////////////////
// Simple wrapper that synchronizes access to the underlying value.
template<typename T>
class Locked {
public:
  Locked() = default;
  explicit Locked(const T& value) : _value(value) {}

  operator T () const {
    return use([=](const T& value) { return value; });
  }

  Locked<T>& operator = (const T& other) {
    use([=](T& value) { value = other; });
    return *this;
  }

  Locked<T>& operator ++ () {
    use([](T& value) { ++value; });
    return *this;
  }

  Locked<T> operator ++ (int) {
    Locked<T> tmp(*this);
    ++(*this);
    return tmp;
  }

  template<typename F>
  typename std::result_of<F(T&)>::type use(F&& fun) {
    std::lock_guard<std::mutex> lock(_mutex);
    return fun(_value);
  }

  template<typename F>
  typename std::result_of<F(const T&)>::type use(F&& fun) const {
    std::lock_guard<std::mutex> lock(_mutex);
    return fun(_value);
  }

private:

  mutable std::mutex _mutex;
  T                  _value;
};

////////////////////////////////////////////////////////////////////////////////
struct TestSuccess {};

struct TestError {
  int code;

  bool operator == (TestError other) const { return code == other.code; }
  bool operator != (TestError other) const { return code != other.code; }
};

TestError error1{1};
TestError error2{2};

////////////////////////////////////////////////////////////////////////////////
std::ostream& operator << (std::ostream& s, const TestError& error) {
  return s << "TestError(" << error.code << ")";
}

template<typename T, typename E>
std::ostream& operator << (std::ostream& s, const fry::Result<T, E>& r) {
  r.match( [&s](const T& value) { s << "Success(" << value << ")"; }
         , [&s](const E& error) { s << "Failure(" << error << ")"; });

  return s;
}

template<typename E>
std::ostream& operator << (std::ostream& s, const fry::Result<void, E>& r) {
  r.match( [&s]()               { s << "Success"; }
         , [&s](const E& error) { s << "Failure(" << error << ")"; });

  return s;
}



#endif // __TEST_HELPERS_H__