#ifndef __RESULT_H__
#define __RESULT_H__

#include <boost/variant.hpp>
#include "helpers.h"

////////////////////////////////////////////////////////////////////////////////
namespace detail {
  template<typename T, typename E>
  struct TestVisitor : public boost::static_visitor<bool> {
    bool operator () (const T&) const { return true;  }
    bool operator () (const E&) const { return false; }
  };

  template<typename T, typename E, typename F>
  struct ConstSuccessVisitor : public boost::static_visitor<> {
    F fun;

    ConstSuccessVisitor(F&& fun) : fun(fun) {}

    void operator () (const T& value) const { fun(value); }
    void operator () (const E&) const {}
  };

  template<typename T, typename E, typename F>
  struct MutableSuccessVisitor : public boost::static_visitor<> {
    F fun;

    MutableSuccessVisitor(F&& fun) : fun(fun) {}

    void operator () (T& value) const { fun(value); }
    void operator () (const E&) const {}
  };

  template<typename T, typename E, typename F>
  struct FailureVisitor : public boost::static_visitor<> {
    F fun;

    FailureVisitor(F&& fun) : fun(fun) {}

    void operator () (const T&) const {}
    void operator () (const E& error) const { fun(error); }
  };
}

////////////////////////////////////////////////////////////////////////////////
template<typename T, typename E>
class Result {
  static_assert(!std::is_same<T, E>{}, "value and error must have different types");

public:

  Result(const T& value) : _storage(value) {}
  Result(T&& value) : _storage(std::move(value)) {}

  Result(const E& error) : _storage(error) {}
  Result(E&& error) : _storage(std::move(error)) {}

  //----------------------------------------------------------------------------
  Result(Result<T, E>&& other) = default;
  Result<T, E>& operator = (Result<T, E>&& other) = default;

  //----------------------------------------------------------------------------
  explicit operator bool () const {
    return boost::apply_visitor(detail::TestVisitor<T, E>(), _storage);
  }

  //----------------------------------------------------------------------------
  template<typename F>
  typename std::enable_if<can_call<F, const T&>{}, const Result<T, E>&>::type
  if_success(F&& fun) const {
    boost::apply_visitor(
      detail::ConstSuccessVisitor<T, E, F>(std::forward<F>(fun)), _storage
    );
  }

  //----------------------------------------------------------------------------
  template<typename F>
  typename std::enable_if<can_call<F, T&>{}, Result<T, E>&>::type
  if_success(F&& fun) {
    boost::apply_visitor(
      detail::MutableSuccessVisitor<T, E, F>(std::forward<F>(fun)), _storage
    );

    return *this;
  }

  //----------------------------------------------------------------------------
  template<typename F>
  const Result<T, E>& if_failure(F&& fun) const {
    static_assert(can_call<F, E>{}, "F cannot be called with the error type");

    boost::apply_visitor(
      detail::FailureVisitor<T, E, F>(std::forward<F>(fun)), _storage
    );

    return *this;
  }

private:
  boost::variant<T, E> _storage;
};

#endif // __RESULT_H__