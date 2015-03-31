//
// Copyright (c) 2014 Adam Cigánek (adam.ciganek@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __FRY__EITHER_H__
#define __FRY__EITHER_H__

#include "helpers.h"

namespace fry {

////////////////////////////////////////////////////////////////////////////////
namespace detail {
  template<typename T>
  constexpr T max(T a, T b) {
    return a > b ? a : b;
  }

// Workaround for bug in libc++ where std::common_type does not work with void.
  template<typename T0, typename T1>
  struct common_type : std::common_type<T0, T1> {};

  template<>
  struct common_type<void, void> { typedef void type; };
}

////////////////////////////////////////////////////////////////////////////////
template<typename First, typename Second>
class Either {
public:

  enum class Type { first, second };

  //----------------------------------------------------------------------------
  Either() : _type(Type::first) {
    assign(First());
  }

  //----------------------------------------------------------------------------
  ~Either() {
    destroy();
  }

  //----------------------------------------------------------------------------
  explicit Either(const First& v) : _type(Type::first) {
    assign(v);
  }

  explicit Either(First&& v) : _type(Type::first) {
    assign(std::move(v));
  }

  explicit Either(const Second& v) : _type(Type::second) {
    assign(v);
  }

  explicit Either(Second&& v) : _type(Type::second) {
    assign(std::move(v));
  }

  //----------------------------------------------------------------------------
  Either(const Either<First, Second>& other) : _type(other._type) {
    switch (other._type) {
      case Type::first:  assign(other.access<First>());  break;
      case Type::second: assign(other.access<Second>()); break;
    }
  }

  Either<First, Second>& operator = (const Either<First, Second>& other) {
    if (&other != this) {
      if (_type == other._type) {
        switch (_type) {
          case Type::first:  access<First>()  = other.access<First>();  break;
          case Type::second: access<Second>() = other.access<Second>(); break;
        }
      } else {
        destroy();

        switch (other._type) {
          case Type::first:  assign(other.access<First>());  break;
          case Type::second: assign(other.access<Second>()); break;
        }

        _type = other._type;
      }
    }

    return *this;
  }

  //----------------------------------------------------------------------------
  Either(Either<First, Second>&& other) : _type(other._type) {
    switch (other._type) {
      case Type::first:  assign(std::move(other.access<First>()));  break;
      case Type::second: assign(std::move(other.access<Second>())); break;
    }
  }

  Either<First, Second>& operator = (Either<First, Second>&& other) {
    assert(this != &other);

    if (_type == other._type) {
      switch (_type) {
        case Type::first:
          access<First>() = std::move(other.access<First>());
          break;
        case Type::second:
          access<Second>() = std::move(other.access<Second>());
          break;
      }
    } else {
      destroy();

      switch (other._type) {
        case Type::first:  assign(std::move(other.access<First>()));  break;
        case Type::second: assign(std::move(other.access<Second>())); break;
      }

      _type = other._type;
    }

    return *this;
  }

  //----------------------------------------------------------------------------
  Either<First, Second>& operator = (const First& other) {
    if (_type == Type::first) {
      access<First>() = other;
    } else {
      destroy();
      assign(other);
      _type = Type::first;
    }

    return *this;
  }

  Either<First, Second>& operator = (First&& other) {
    if (_type == Type::first) {
      access<First>() = std::move(other);
    } else {
      destroy();
      assign(std::move(other));
      _type = Type::first;
    }

    return *this;
  }

  Either<First, Second>& operator = (const Second& other) {
    if (_type == Type::second) {
      access<Second>() = other;
    } else {
      destroy();
      assign(other);
      _type = Type::second;
    }

    return *this;
  }

  Either<First, Second>& operator = (Second&& other) {
    if (_type == Type::second) {
      access<Second>() = std::move(other);
    } else {
      destroy();
      assign(std::move(other));
      _type = Type::second;
    }

    return *this;
  }

  //----------------------------------------------------------------------------
  Type type() const {
    return _type;
  }

  //----------------------------------------------------------------------------
  template< typename F1, typename F2
          , typename = enable_if<   can_call<F1, const First&>()
                                 && can_call<F2, const Second&>()>>
  typename detail::common_type<result_of<F1, First>, result_of<F2, Second>>::type
  match(F1&& first, F2&& second) const {
    switch (_type) {
      case Type::first:
        return first(access<First>());
      default /*case Type::second*/:
        return second(access<Second>());
    }
  }

  template< typename F1, typename F2
          , typename = enable_if<    can_call<F1, First&>()
                                 && !can_call<F1, const First&>()
                                 &&  can_call<F2, Second&>()
                                 && !can_call<F2, const Second&>()>>
  typename detail::common_type<result_of<F1, First&>, result_of<F2, Second&>>::type
  match(F1&& first, F2&& second) {
    switch (_type) {
      case Type::first:
        return first(access<First>());
      default /*case Type::second*/:
        return second(access<Second>());
    }
  }

  //----------------------------------------------------------------------------
  template< typename V
          , typename = enable_if<   can_call<V, const First&>()
                                 && can_call<V, const Second&>()>>
  typename detail::common_type<result_of<V, First>, result_of<V, Second>>::type
  visit(V&& visitor) const {
    switch (_type) {
      case Type::first:
        return visitor(access<First>());
      default /*case Type::second*/:
        return visitor(access<Second>());
    }
  }

  template< typename V
          , typename = enable_if<    can_call<V, First&>()
                                 && !can_call<V, const First&>()
                                 &&  can_call<V, Second&>()
                                 && !can_call<V, const Second&>()>>
  typename detail::common_type<result_of<V, First&>, result_of<V, Second&>>::type
  visit(V&& visitor) {
    switch (_type) {
      case Type::first:
        return visitor(access<First>());
      default /*case Type::second*/:
        return visitor(access<Second>());
    }
  }

private:

  template<typename T>
  T& access() {
    return *static_cast<T*>(static_cast<void*>(&_storage));
  }

  template<typename T>
  const T& access() const {
    return *static_cast<const T*>(static_cast<const void*>(&_storage));
  }

  template<typename T>
  void assign(T&& stuff) {
    new (&_storage) remove_reference<T>(std::forward<T>(stuff));
  }

  void destroy() {
    if (_type == Type::first) {
      access<First>().~First();
    } else {
      access<Second>().~Second();
    }
  }

private:
  union Union {
    First  first;
    Second second;
  };

  typedef typename std::aligned_storage<sizeof(Union), alignof(Union)>::type
          Storage;

  Type    _type;
  Storage _storage;
};

}

#endif // __FRY__EITHER_H__
