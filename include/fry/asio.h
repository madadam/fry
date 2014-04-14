//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __FRY__ASIO_H__
#define __FRY__ASIO_H__

// Glue code between this library and boost::asio

#include <boost/asio/async_result.hpp>
#include <boost/asio/handler_type.hpp>
#include <boost/system/error_code.hpp>

#include "future.h"
#include "result.h"


namespace fry { namespace asio {

////////////////////////////////////////////////////////////////////////////////
template<typename T>
using Result = ::fry::Result<T, boost::system::error_code>;

template<typename T>
Result<typename std::decay<T>::type> success(T&& value) {
  return Result<typename std::decay<T>::type>(value);
}

template<typename T>
Result<T> failure(const boost::system::error_code& code) {
  return Result<T>(code);
}

////////////////////////////////////////////////////////////////////////////////
struct UseFuture {};

constexpr UseFuture use_future;

////////////////////////////////////////////////////////////////////////////////
namespace detail {
  template<typename T>
  struct PromiseHandler {
    PromiseHandler(UseFuture)
      : promise(std::make_shared<Promise<Result<T>>>())
    {}

    void operator () (const boost::system::error_code& error, T value) {
      if (error) {
        promise->set_value(Result<T>(error));
      } else {
        promise->set_value(Result<T>(value));
      }
    }

    std::shared_ptr<Promise<Result<T>>> promise;
  };

  //----------------------------------------------------------------------------
  template<>
  struct PromiseHandler<void> {
    PromiseHandler(UseFuture)
      : promise(std::make_shared<Promise<Result<void>>>())
    {}

    void operator () (const boost::system::error_code& error) {
      if (error) {
        promise->set_value(Result<void>(error));
      } else {
        promise->set_value(Result<void>());
      }
    }

    std::shared_ptr<Promise<Result<void>>> promise;
  };
} // namespace detail
}} // namespace fry::asio

////////////////////////////////////////////////////////////////////////////////
namespace boost { namespace asio {

template <typename T>
class async_result<::fry::asio::detail::PromiseHandler<T>> {
public:
  typedef ::fry::Future<::fry::asio::Result<T>> type;

  explicit async_result(::fry::asio::detail::PromiseHandler<T>& h)
    : _future(h.promise->get_future())
  {}

  type get() {
    return std::move(_future);
  }

private:

  type _future;
};

template<typename R, typename T>
struct handler_type<::fry::asio::UseFuture, R(boost::system::error_code, T)> {
  typedef ::fry::asio::detail::PromiseHandler<T> type;
};

template<typename R>
struct handler_type<::fry::asio::UseFuture, R(boost::system::error_code)> {
  typedef ::fry::asio::detail::PromiseHandler<void> type;
};

}} // namespace boost::asio


#endif // __FRY__ASIO_H__