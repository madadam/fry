#ifndef __FUTURE_H__
#define __FUTURE_H__

#include <cassert>
#include <functional>
#include <memory>
#include <thread>
#include <type_traits>
#include <boost/optional.hpp>

#include "helpers.h"

////////////////////////////////////////////////////////////////////////////////
template<typename> class Future;
template<typename> class Promise;
template<typename> class PackagedTask;

template<typename T> Future<T> make_ready_future(T&& value);
Future<void> make_ready_future();



////////////////////////////////////////////////////////////////////////////////
namespace detail {
  //----------------------------------------------------------------------------
  template<typename F, typename... Args> struct _result_of {
    typedef typename std::result_of<F(Args...)>::type type;
  };

  template<typename F> struct _result_of<F, void> {
    typedef typename std::result_of<F()>::type type;
  };

  template<typename F, typename... Args> struct _checked_result_of {
    static_assert(::can_call<F, Args...>{}, "F cannot be called with arguments of types Args...");
    typedef typename _result_of<F, Args...>::type type;
  };

  //----------------------------------------------------------------------------
  // result_of<T, A> is the same as result_of_t<T(A0)>, but it works also when
  // A is void, in which case it is the same as result_of_t<T()>.
  template<typename... T>
  using result_of = typename _checked_result_of<T...>::type;

  //----------------------------------------------------------------------------
  template<typename T> struct is_void : std::is_same<T, void> {};

  //----------------------------------------------------------------------------
  template<typename T> struct _unwrapped            { typedef T type; };
  template<typename T> struct _unwrapped<Future<T>> { typedef T type; };

  //----------------------------------------------------------------------------
  // If T is a Future, returns its underlying value type. otherwise returns T.
  template<typename T>
  using unwrapped_type = typename _unwrapped<T>::type;

  template<typename... T>
  using unwrapped_result_of = unwrapped_type<result_of<T...>>;

  //----------------------------------------------------------------------------
  using lock_guard = std::lock_guard<std::mutex>;

} // namespace detail



////////////////////////////////////////////////////////////////////////////////
//
// Future
//
////////////////////////////////////////////////////////////////////////////////
template<typename T>
class Future {
public:
  struct State;

  Future(const Future<T>&) = delete;
  Future(Future<T>&& other)
    : _state(other._state)
  {
    other._state = nullptr;
  }

  // TODO: is possible, add a static_assert that fun is callable with T
  template<typename F>
  Future<detail::unwrapped_result_of<F, T>>
  then(F&& fun) {
    assert(_state);
    detail::lock_guard lock(_state->mutex);

    return _state->set_continuation(fun);
  }

private:

  Future(std::shared_ptr<State> state)
    : _state(state)
  {}

private:

  std::shared_ptr<State> _state;

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
    : _state(std::make_shared<typename Future<T>::State>())
  {}

  Promise(Promise<T>&& other)
    : _state(other._state)
  {
    other._state = nullptr;
  }

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

  std::shared_ptr<typename Future<T>::State> _state;
};



////////////////////////////////////////////////////////////////////////////////
//
// is_future - test if the given type is a future.
//
////////////////////////////////////////////////////////////////////////////////
template<typename U> struct is_future : std::false_type {};
template<typename T> struct is_future<Future<T>> : std::true_type {};



////////////////////////////////////////////////////////////////////////////////
//
// make_ready_future - Create a future that already holds a value.
//
////////////////////////////////////////////////////////////////////////////////
template<typename T>
Future<T> make_ready_future(T&& value) {
  static_assert( !is_future<typename std::remove_reference<T>::type>{}
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
  // Set value of a promise to the result of calling the given function with
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
  // Make a ready future from the result of calling the given function with the
  // given arguments.
  template<typename F, typename... Args>
  enable_if_t<!is_void<result_of<F, Args...>>{}, Future<unwrapped_result_of<F, Args...>>>
  make_ready_future(F&& fun, Args&&... args) {
    return ::make_ready_future(fun(std::forward<Args>(args)...));
  }

  template<typename F, typename... Args>
  enable_if_t<is_void<result_of<F, Args...>>{}, Future<void>>
  make_ready_future(F&& fun, Args&&... args) {
    fun(std::forward<Args>(args)...);
    return ::make_ready_future();
  }

  //----------------------------------------------------------------------------
  template<typename... Args>
  class ContinuationBase {
  public:
    virtual ~ContinuationBase() {}
    virtual void operator () (Args&&... args) = 0;
  };

  template<typename R, typename... Args>
  class Continuation : public ContinuationBase<Args...> {
  public:

    template<typename F>
    Continuation(F&& fun) : _fun(fun) {}

    Continuation(const Continuation<R, Args...>&) = delete;
    Continuation<R, Args...>& operator = (const Continuation<R, Args...>&) = delete;

    void operator () (Args&&... args) override {
      detail::set_value(_promise, _fun, std::forward<Args>(args)...);
    }

    Future<unwrapped_type<R>> get_future() const {
      return _promise.get_future();
    }

  private:

    std::function<R(Args...)>   _fun;
    Promise<unwrapped_type<R>>  _promise;
  };
} // namespace detail



////////////////////////////////////////////////////////////////////////////////
//
// Internal shared state of the Future
//
////////////////////////////////////////////////////////////////////////////////
template<typename T>
struct Future<T>::State {
  std::mutex                                    mutex;
  boost::optional<T>                            value;
  std::unique_ptr<detail::ContinuationBase<T&>> continuation;

  void swap(State& other) {
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
  Future<detail::unwrapped_result_of<F, T>> set_continuation(F&& fun) {
    if (value) {
      return detail::make_ready_future(fun, *value);
    } else {
      using CB = detail::ContinuationBase<T&>;
      using C  = detail::Continuation<decltype(fun(*value)), T&>;

      continuation.reset(static_cast<CB*>(new C(fun)));
      return static_cast<C&>(*continuation).get_future();
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
struct Future<void>::State {
  std::mutex                                  mutex;
  bool                                        ready;
  std::unique_ptr<detail::ContinuationBase<>> continuation;

  State() : ready(false) {}

  void swap(State& other) {
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
  Future<detail::unwrapped_result_of<F, void>> set_continuation(F&& fun) {
    if (ready) {
      return detail::make_ready_future(fun);
    } else {
      typedef detail::ContinuationBase<>            CB;
      typedef detail::Continuation<decltype(fun())> C;

      continuation.reset(static_cast<CB*>(new C(fun)));
      return static_cast<C&>(*continuation).get_future();
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

#endif // __FUTURE_H__