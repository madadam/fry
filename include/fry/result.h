//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __FRY__RESULT_H__
#define __FRY__RESULT_H__

#include <boost/optional.hpp>
#include "helpers.h"

namespace fry {

////////////////////////////////////////////////////////////////////////////////
template<typename, typename> class Result;

template<typename E, typename T>
Result<typename std::decay<T>::type, E> make_result(T&&);

template<typename E> Result<void, E> make_result();

template<typename E, typename T> Result<T, E>& make_result(Result<T, E>&);
template<typename E, typename T> const Result<T, E>& make_result(const Result<T, E>&);
template<typename E, typename T> Result<T, E> make_result(Result<T, E>&&);

////////////////////////////////////////////////////////////////////////////////
// Traits
namespace internal {
  template<typename T, typename E>
  struct add_result { typedef Result<T, E> type; };

  template<typename T, typename E>
  struct add_result<Result<T, E>, E> { typedef Result<T, E> type; };
}

template<typename T, typename E>
using add_result = typename internal::add_result<T, E>::type;



////////////////////////////////////////////////////////////////////////////////
namespace detail {
  template<typename E, typename F, typename... Args>
  enable_if< !is_void<result_of<F, Args...>>{}
           , add_result<result_of<F, Args...>, E>>
  make_result(F&& fun, Args&&... args) {
    return ::fry::make_result<E>(fun(std::forward<Args>(args)...));
  }

  template<typename E, typename F, typename... Args>
  enable_if< is_void<result_of<F, Args...>>{}, Result<void, E>>
  make_result(F&& fun, Args&&... args) {
    fun(std::forward<Args>(args)...);
    return {};
  }

  //----------------------------------------------------------------------------
  template<typename T, typename E, typename F, typename R = result_of<F, E>>
  enable_if<!is_void<R>{}, Result<T, E>>
  make_result_from_error(F&& fun, const E& error) {
    return ::fry::make_result<E>(fun(error));
  }

  template<typename T, typename E, typename F, typename R = result_of<F, E>>
  enable_if<is_void<R>{}, Result<T, E>>
  make_result_from_error(F&& fun, const E& error) {
    fun(error);
    return Result<T, E>(error);
  }

  //----------------------------------------------------------------------------
  template<typename T>
  constexpr T max(T a, T b) {
    return a > b ? a : b;
  }
}



////////////////////////////////////////////////////////////////////////////////
template<typename T, typename E>
class Result {
  static_assert(!std::is_same<T, E>{}, "value type and error type must be different");

public:

  typedef T value_type;
  typedef E error_type;

  //----------------------------------------------------------------------------
  Result() : _success(true) {
    assign(T());
  }

  //----------------------------------------------------------------------------
  explicit Result(const T& value) : _success(true) { assign(value);            }
  explicit Result(T&& value)      : _success(true) { assign(std::move(value)); }

  explicit Result(const E& error) : _success(false) { assign(error); }
  explicit Result(E&& error)      : _success(false) { assign(std::move(error)); }

  //----------------------------------------------------------------------------
  ~Result() {
    destroy();
  }

  //----------------------------------------------------------------------------
  Result(const Result<T, E>& other) : _success(other._success) {
    if (other._success) {
      assign(other.access<T>());
    } else {
      assign(other.access<E>());
    }
  }

  Result<T, E>& operator = (const Result<T, E>& other) {
    if (&other != this) {
      if (_success) {
        if (other._success) {
          access<T>() = other.access<T>();
        } else {
          destroy();
          assign(other.access<E>());
          _success = false;
        }
      } else {
        if (other._success) {
          destroy();
          assign(other.access<T>());
          _success = true;
        } else {
          access<E>() = other.access<E>();
        }
      }
    }

    return *this;
  }

  //----------------------------------------------------------------------------
  Result(Result<T, E>&& other) : _success(other._success) {
    if (other._success) {
      assign(std::move(other.access<T>()));
    } else {
      assign(std::move(other.access<E>()));
    }
  }

  Result<T, E>& operator = (Result<T, E>&& other) {
    if (&other != this) {
      if (_success) {
        if (other._success) {
          access<T>() = std::move(other.access<T>());
        } else {
          destroy();
          assign(std::move(other.access<E>()));
          _success = false;
        }
      } else {
        if (other._success) {
          destroy();
          assign(std::move(other.access<T>()));
          _success = true;
        } else {
          access<E>() = other.access<E>();
        }
      }
    }

    return *this;
  }

  //----------------------------------------------------------------------------
  template<typename SF, typename FF>
  typename std::common_type<result_of<SF, T>, result_of<FF, E>>::type
  match(SF&& on_success, FF&& on_failure) const {
    if (_success) {
      return on_success(access<T>());
    } else {
      return on_failure(access<E>());
    }
  }

  //----------------------------------------------------------------------------
  explicit operator bool () const {
    return _success;
  }

  //----------------------------------------------------------------------------
  template<typename F>
  add_result<result_of<F, const T&>, E>
  if_success(F&& fun) const {
    typedef add_result<result_of<F, const T&>, E> R;

    return match(
        [=](const T& value) { return detail::make_result<E>(fun, value); }
      , [] (const E& error) { return R(error); }
    );
  }

  //----------------------------------------------------------------------------
  template<typename F>
  Result<T, E> if_failure(F&& fun) const {
    typedef result_of<F, E> R;
    static_assert(    std::is_same<R, Result<T, E>>{}
                   || std::is_same<R, T>{}
                   || std::is_same<R, void>{}
                 , "the callbacks return type must be either the type of *this"
                   ", the value_type of *this or void");

    return match(
        [] (const T& value) { return Result<T, E>(value); }
      , [=](const E& error) {
          return detail::make_result_from_error<T, E>(fun, error);
        }
    );
  }

  //----------------------------------------------------------------------------
  T value_or(const T& a) const {
    return match( []  (const T& value) { return value; }
                , [&a](const E&)       { return a;     });
  }

private:

  template<typename U>
  U& access() {
    return *static_cast<U*>(static_cast<void*>(&_storage));
  }

  template<typename U>
  const U& access() const {
    return *static_cast<const U*>(static_cast<const void*>(&_storage));
  }

  template<typename U>
  void assign(U&& stuff) {
    new (&_storage) remove_reference<U>(std::forward<U>(stuff));
  }

  void destroy() {
    if (_success) {
      access<T>().~T();
    } else {
      access<E>().~E();
    }
  }

private:
  typedef typename std::aligned_storage<
              detail::max(sizeof(T), sizeof(E))
            , detail::max(alignof(T), alignof(E))
          >::type Storage;

  bool    _success;
  Storage _storage;
};



////////////////////////////////////////////////////////////////////////////////
// Specialization for void
template<typename E>
class Result<void, E> {
public:
  typedef void value_type;
  typedef E    error_type;

  Result() {}
  Result(const E& error) : _error(error) {}
  Result(E&& error) : _error(std::move(error)) {}

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
      return add_result<result_of<F>, E>{ *_error };
    } else {
      return detail::make_result<E>(fun);
    }
  }

  //----------------------------------------------------------------------------
  template<typename F>
  Result<void, E> if_failure(F&& fun) const {
    typedef result_of<F, E> R;
    static_assert(    std::is_same<R, Result<void, E>>{}
                   || std::is_same<R, void>{}
                 , "the callback return type must be either the type of *this"
                   " or void");

    if (_error) {
      return detail::make_result<E>(fun, *_error);
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
Result<T, E>& make_result(Result<T, E>& result) {
  return result;
}

template<typename E, typename T>
const Result<T, E>& make_result(const Result<T, E>& result) {
  return result;
}

template<typename E, typename T>
Result<T, E> make_result(Result<T, E>&& result) {
  return std::move(result);
}

} // namespace fry

#endif // __FRY__RESULT_H__