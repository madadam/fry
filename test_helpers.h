#ifndef __TEST_HELPERS_H__
#define __TEST_HELPERS_H__

#include <mutex>
#include <type_traits>

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

#endif // __TEST_HELPERS_H__