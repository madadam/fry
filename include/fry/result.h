//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __FRY__RESULT_H__
#define __FRY__RESULT_H__

#include <boost/optional.hpp>
#include "either.h"

namespace fry {

////////////////////////////////////////////////////////////////////////////////
template<typename, typename> class Result;

template<typename E, typename T>
Result<typename std::decay<T>::type, E> make_result(T&&);

template<typename E> Result<void, E> make_result();
template<typename E, typename T> Result<T, E> make_result(Result<T, E>);

////////////////////////////////////////////////////////////////////////////////
// Traits
namespace internal {
  template<typename T, typename E, typename D>
  struct add_result { typedef Result<T, E> type; };

  template<typename T, typename E, typename D>
  struct add_result<Result<T, E>, E, D> { typedef Result<T, E> type; };

  template<typename E, typename D>
  struct add_result<E, E, D> { typedef Result<D, E> type; };
}

template<typename T, typename E, typename D = void>
using add_result = typename internal::add_result<T, E, D>::type;



////////////////////////////////////////////////////////////////////////////////
namespace detail {
  template<typename T, typename E, typename F, typename... Args>
  enable_if<    !is_void<result_of<F, Args...>>{}
             && !std::is_same<result_of<F, Args...>, E>{}
           , add_result<result_of<F, Args...>, E>>
  make_result(F&& fun, Args&&... args) {
    return ::fry::make_result<E>(fun(std::forward<Args>(args)...));
  }

  template<typename T, typename E, typename F, typename... Args>
  enable_if<std::is_same<result_of<F, Args...>, E>{}, Result<T, E>>
  make_result(F&& fun, Args&&... args) {
    return Result<T, E>(fun(std::forward<Args>(args)...));
  }

  template<typename T, typename E, typename F, typename... Args>
  enable_if<is_void<result_of<F, Args...>>{}, Result<void, E>>
  make_result(F&& fun, Args&&... args) {
    fun(std::forward<Args>(args)...);
    return {};
  }

  //----------------------------------------------------------------------------
  template<typename T, typename E, typename F>
  enable_if<    !is_void<result_of<F, E>>{}
             && !std::is_same<result_of<F, E>, E>{}
           , Result<T, E>
           >
  make_result(F&& fun, const E& error) {
    return ::fry::make_result<E>(fun(error));
  }

  template<typename T, typename E, typename F>
  enable_if<is_void<result_of<F, E>>{}, Result<T, E>>
  make_result(F&& fun, const E& error) {
    fun(error);
    return Result<T, E>(error);
  }

  template<typename T, typename E, typename F>
  enable_if<std::is_same<result_of<F, E>, E>{}, Result<T, E>>
  make_result(F&& fun, const E& error) {
    return Result<T, E>(fun(error));
  }
}



////////////////////////////////////////////////////////////////////////////////
template<typename T, typename E>
class Result : public Either<T, E> {
  static_assert(!std::is_same<T, E>{}, "value type and error type must be different");

public:

  typedef T value_type;
  typedef E error_type;

  using Either<T, E>::Either;
  using Either<T, E>::match;

  //----------------------------------------------------------------------------
  explicit operator bool () const {
    return this->type() == Either<T, E>::Type::first;
  }

  //----------------------------------------------------------------------------
  template<typename F>
  add_result<result_of<F, T>, E, T>
  if_success(F&& fun) const {
    typedef add_result<result_of<F, T>, E, T> R;

    return match(
        [=](const T& value) { return detail::make_result<T, E>(fun, value); }
      , [] (const E& error) { return R(error); }
    );
  }

  //----------------------------------------------------------------------------
  template<typename F>
  Result<T, E> if_failure(F&& fun) const {
    typedef result_of<F, E> R;
    static_assert(    std::is_same<R, Result<T, E>>{}
                   || std::is_same<R, T>{}
                   || std::is_same<R, E>{}
                   || std::is_same<R, void>{}
                 , "the callbacks return type must be either the type of *this"
                   ", the value_type of *this, the error_type of *this or void");

    return match(
        [] (const T& value) { return Result<T, E>(value); }
      , [=](const E& error) {
          return detail::make_result<T, E>(fun, error);
        }
    );
  }

  //----------------------------------------------------------------------------
  T value_or(const T& a) const {
    return match( []  (const T& value) { return value; }
                , [&a](const E&)       { return a;     });
  }
};



////////////////////////////////////////////////////////////////////////////////
// Specialization for void
template<typename E>
class Result<void, E> {
public:
  typedef void value_type;
  typedef E    error_type;

  Result() {}
  explicit Result(const E& error) : _error(error) {}
  explicit Result(E&& error) : _error(std::move(error)) {}

  //----------------------------------------------------------------------------
  explicit operator bool () const {
    return !_error;
  }

  //----------------------------------------------------------------------------
  template<typename SF, typename FF>
  typename std::common_type<result_of<SF>, result_of<FF, E>>::type
  match(SF&& on_success, FF&& on_failure) const {
    if (_error) {
      return on_failure(*_error);
    } else {
      return on_success();
    }
  }

  //----------------------------------------------------------------------------
  template<typename F>
  add_result<result_of<F>, E> if_success(F&& fun) const {
    if (_error) {
      return add_result<result_of<F>, E, void>{ *_error };
    } else {
      return detail::make_result<void, E>(fun);
    }
  }

  //----------------------------------------------------------------------------
  template<typename F>
  Result<void, E> if_failure(F&& fun) const {
    typedef result_of<F, E> R;
    static_assert(    std::is_same<R, Result<void, E>>{}
                   || std::is_same<R, E>{}
                   || std::is_same<R, void>{}
                 , "the callback return type must be either the type of *this"
                   ", error_type of *this or void");

    if (_error) {
      return detail::make_result<void, E>(fun, *_error);
    } else {
      return {};
    }
  }

private:
  boost::optional<E> _error;
};



////////////////////////////////////////////////////////////////////////////////
template<typename T, typename E>
bool operator == (const Result<T, E>& r1, const Result<T, E>& r2) {
  return r1.match(
    [&r2](const T& value1) {
      return r2.match( [&value1](const T& value2) { return value1 == value2; }
                     , [](const E&)               { return false; });
    },
    [&r2](const E& error1) {
      return r2.match( [](const T&)               { return false; }
                     , [&error1](const E& error2) { return error1 == error2; });
    }
  );
}

template<typename E>
bool operator == (const Result<void, E>& r1, const Result<void, E>& r2) {
  return r1.match(
    [&r2]() {
      return r2.match( []()         { return true;  }
                     , [](const E&) { return false; });
    },
    [&r2](const E& error1) {
      return r2.match( []()                       { return false; }
                     , [&error1](const E& error2) { return error1 == error2; });
    }
  );
}

template<typename T, typename E>
bool operator != (const Result<T, E>& r1, const Result<T, E>& r2) {
  return !(r1 == r2);
}



////////////////////////////////////////////////////////////////////////////////
template<typename E, typename T>
Result<typename std::decay<T>::type, E>
make_result(T&& value) {
  return Result<typename std::decay<T>::type, E>(std::forward<T>(value));
}

template<typename E>
Result<void, E> make_result() {
  return Result<void, E>();
}

template<typename E, typename T>
Result<T, E> make_result(Result<T, E> result) {
  return std::move(result);
}

////////////////////////////////////////////////////////////////////////////////
// Function object that calls make_result<E>(value).
template<typename E>
struct ResultMaker {
  template<typename T, typename = enable_if<!std::is_same<T, E>()>>
  auto operator () (T&& value) const
  -> decltype(make_result<E>(std::forward<T>(value)))
  {
    return make_result<E>(std::forward<T>(value));
  }

  Result<void, E> operator () () const {
    return make_result<E>();
  }
};

} // namespace fry

#endif // __FRY__RESULT_H__