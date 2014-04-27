//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __FRY__FUTURE_H__
#define __FRY__FUTURE_H__

#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <boost/optional.hpp>

#include "helpers.h"

namespace fry {

////////////////////////////////////////////////////////////////////////////////
template<typename> class Future;
template<typename> class Promise;
template<typename> class PackagedTask;

template<typename T> Future<T> make_ready_future(T&& value);
Future<void> make_ready_future();


////////////////////////////////////////////////////////////////////////////////
//
// Traits
//
////////////////////////////////////////////////////////////////////////////////
namespace internal {
  template<typename T> struct add_future            { typedef Future<T> type; };
  template<typename T> struct add_future<Future<T>> { typedef Future<T> type; };

  template<typename T> struct remove_future            { typedef T type; };
  template<typename T> struct remove_future<Future<T>> { typedef T type; };
}

//------------------------------------------------------------------------------
template<typename T>
using add_future = typename internal::add_future<T>::type;

//------------------------------------------------------------------------------
template<typename T>
using remove_future = typename internal::remove_future<T>::type;

//------------------------------------------------------------------------------
template<typename U> struct is_future : std::false_type {};
template<typename T> struct is_future<Future<T>> : std::true_type {};



////////////////////////////////////////////////////////////////////////////////
namespace detail {
  using lock_guard = std::lock_guard<std::mutex>;

  template<typename> struct State;
}

////////////////////////////////////////////////////////////////////////////////
//
// Future
//
////////////////////////////////////////////////////////////////////////////////
template<typename T>
class Future {
public:
  Future(const Future<T>&) = delete;
  Future(Future<T>&& other) = default;

  template<typename F>
  add_future<result_of<F, T>> then(F&& fun) {
    assert(_state);
    return _state->set_continuation(fun);
  }

private:

  Future(std::shared_ptr<detail::State<T>> state)
    : _state(state)
  {}

private:

  std::shared_ptr<detail::State<T>> _state;

  friend class Promise<T>;
};



////////////////////////////////////////////////////////////////////////////////
//
// Promise
//
////////////////////////////////////////////////////////////////////////////////
template<typename T>
class Promise {
public:

  typedef T value_type;

  Promise()
    : _state(std::make_shared<typename detail::State<T>>())
  {}

  Promise(Promise<T>&& other) = default;

  Promise(const Promise<T>&) = delete;
  Promise<T>& operator = (const Promise<T>&) = delete;

  Future<T> get_future() const {
    assert(_state);
    return { _state };
  }

  template<typename... U>
  void set_value(U&&... values) {
    assert(_state);
    detail::lock_guard lock(_state->mutex);
    _state->set_value(std::forward<U>(values)...);
  }

  void set_value(Future<T>&& future) {
    detail::lock_guard lock1(_state->mutex);
    detail::lock_guard lock2(future._state->mutex);

    if (future._state->is_ready()) {
      _state->move_value_from(*future._state);
    } else {
      _state->swap(*future._state);
    }
  }

private:

  std::shared_ptr<typename detail::State<T>> _state;
};



////////////////////////////////////////////////////////////////////////////////
//
// make_ready_future - Create a future that already holds a value.
//
////////////////////////////////////////////////////////////////////////////////
template<typename T>
Future<T> make_ready_future(T&& value) {
  static_assert( !is_future<remove_reference<T>>{}
               , "l-value Future not allowed as argument to make_ready_future");

  Promise<T> promise;
  promise.set_value(std::forward<T>(value));

  return promise.get_future();
}

template<typename T>
Future<T> make_ready_future(Future<T>&& future) {
  return std::move(future);
}

inline Future<void> make_ready_future() {
  Promise<void> promise;
  promise.set_value();

  return promise.get_future();
}



////////////////////////////////////////////////////////////////////////////////
namespace detail {
  //----------------------------------------------------------------------------
  // Set value of a promise to the result of calling the given callable with
  // the given arguments.
  template<typename R, typename F, typename... Args>
  void set_value(Promise<R>& promise, F&& fun, Args&&... args) {
    promise.set_value(fun(std::forward<Args>(args)...));
  }

  template<typename F, typename... Args>
  void set_value(Promise<void>& promise, F&& fun, Args&&... args) {
    fun(std::forward<Args>(args)...);
    promise.set_value();
  }

  //----------------------------------------------------------------------------
  // Make a ready future from the result of calling the given callable with the
  // given arguments.
  template<typename F, typename... Args>
  enable_if< !is_void<result_of<F, Args...>>{}
           , add_future<result_of<F, Args...>>>
  make_ready_future(F&& fun, Args&&... args) {
    return ::fry::make_ready_future(fun(std::forward<Args>(args)...));
  }

  template<typename F, typename... Args>
  enable_if<is_void<result_of<F, Args...>>{}, Future<void>>
  make_ready_future(F&& fun, Args&&... args) {
    fun(std::forward<Args>(args)...);
    return ::fry::make_ready_future();
  }

  //----------------------------------------------------------------------------
  template<typename... Args>
  class ContinuationBase {
  public:
    virtual ~ContinuationBase() {}
    virtual void operator () (Args&&... args) = 0;
  };

  template<typename F, typename... Args>
  class Continuation : public ContinuationBase<Args...> {
    static_assert(!std::is_reference<F>{}, "F cannot be a reference");

  public:

    typedef result_of<F, Args...> result_type;

    Continuation(const F& fun)
      : _fun(fun) {}

    Continuation(F&& fun)
      : _fun(std::move(fun)) {}

    Continuation(const Continuation<F, Args...>&) = delete;
    Continuation<F, Args...>& operator = (const Continuation<F, Args...>&) = delete;

    void operator () (Args&&... args) override {
      detail::set_value(_promise, _fun, std::forward<Args>(args)...);
    }

    add_future<result_type> get_future() const {
      return _promise.get_future();
    }

  private:

    F                                   _fun;
    Promise<remove_future<result_type>> _promise;
  };

  //----------------------------------------------------------------------------
  template<typename F, typename... T>
  add_future<result_of<F, T...>> build_continuation(
      std::unique_ptr<ContinuationBase<T&...>>& storage
    , F&&                                       fun
  )
  {
    using CB = ContinuationBase<T&...>;
    using C  = Continuation<remove_reference<F>, T&...>;

    storage.reset(static_cast<CB*>(new C(std::forward<F>(fun))));
    return static_cast<C&>(*storage).get_future();
  }
} // namespace detail



////////////////////////////////////////////////////////////////////////////////
namespace detail {

  template<typename T>
  struct State {
    std::mutex                                    mutex;
    boost::optional<T>                            value;
    std::unique_ptr<detail::ContinuationBase<T&>> continuation;

    void swap(State<T>& other) {
      std::swap(value,        other.value);
      std::swap(continuation, other.continuation);
    }

    bool is_ready() const {
      return value;
    }

    void set_value(const T& v) {
      value = v;
      run_continuation();
    }

    void set_value(T&& v) {
      value = std::move(v);
      run_continuation();
    }

    void move_value_from(State& other) {
      assert(other.value);

      value = std::move(other.value);
      run_continuation();
    }

    template<typename F>
    add_future<result_of<F, T>> set_continuation(F&& fun) {
      detail::lock_guard lock(mutex);

      if (value) {
        return detail::make_ready_future(fun, *value);
      } else {
        return detail::build_continuation(continuation, std::forward<F>(fun));
      }
    }

  private:
    void run_continuation() {
      if (continuation) {
        (*continuation)(*value);
        continuation = nullptr;
      }
    }
  };

  ////////////////////////////////////////////////////////////////////////////////
  template<>
  struct State<void> {
    std::mutex                                  mutex;
    bool                                        ready;
    std::unique_ptr<detail::ContinuationBase<>> continuation;

    State() : ready(false) {}

    void swap(State<void>& other) {
      std::swap(ready,        other.ready);
      std::swap(continuation, other.continuation);
    }

    bool is_ready() const {
      return ready;
    }

    void set_value() {
      ready = true;
      run_continuation();
    }

    void move_value_from(State& other) {
      assert(other.ready);

      ready = other.ready;
      run_continuation();
    }

    template<typename F>
    add_future<result_of<F>> set_continuation(F&& fun) {
      detail::lock_guard lock(mutex);

      if (ready) {
        return detail::make_ready_future(fun);
      } else {
        return detail::build_continuation(continuation, std::forward<F>(fun));
      }
    }

  private:
    void run_continuation() {
      if (continuation) {
        (*continuation)();
        continuation = nullptr;
      }
    }
  };

} // namespace detail


////////////////////////////////////////////////////////////////////////////////
//
// PackagedTask
//
////////////////////////////////////////////////////////////////////////////////
template<typename F> class PackagedTask;

template<typename R, typename... Args>
class PackagedTask<R(Args...)> {
public:

  template<typename F>
  explicit PackagedTask(F&& fun) : _fun(std::forward<F>(fun)) {}

  PackagedTask(const PackagedTask<R(Args...)>&) = delete;
  PackagedTask<R(Args...)>& operator = (const PackagedTask<R(Args...)>&) = delete;

  PackagedTask(PackagedTask<R(Args...)>&&) = default;
  PackagedTask<R(Args...)>& operator = (PackagedTask<R(Args...)>&&) = default;

  Future<R> get_future() const {
    return _promise.get_future();
  }

  void operator () (Args&&... args) {
    detail::set_value(_promise, _fun, std::forward<Args>(args)...);
  }

private:

  std::function<R(Args...)> _fun;
  Promise<R>                _promise;
};

} // namespace fry

#endif // __FRY__FUTURE_H__