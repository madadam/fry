#ifndef __ASIO_H__
#define __ASIO_H__

// Collection of glue code to allow convenient use of futures with boost::asio.

#include <boost/system/error_code.hpp>
#include "future.h"
#include "result.h"


namespace asio {

////////////////////////////////////////////////////////////////////////////////
template<typename T>
using Result = ::Result<T, boost::system::error_code>;

template<typename T>
Result<typename std::decay<T>::type> success(T&& value) {
  return Result<typename std::decay<T>::type>(value);
}

template<typename T>
Result<T> failure(const boost::system::error_code& code) {
  return Result<T>(code);
}

////////////////////////////////////////////////////////////////////////////////
namespace detail {

  template<typename T>
  Result<T> to_result(const boost::system::error_code& error, const T& value) {
    if (error) {
      return Result<T>{ error };
    } else {
      return Result<T>{ value };
    }
  };

  Result<void> to_result(const boost::system::error_code& error) {
    if (error) {
      return Result<void>{ error };
    } else {
      return Result<void>{};
    }
  };

  //----------------------------------------------------------------------------
  // HACK: wrapper for movable-only types that make them look copyable. Used to
  // allow passing PackagedTasks (which are not copyable) as asio handlers.
  //
  // Inspired by this: http://stackoverflow.com/a/22891509/170073
  template<typename T>
  struct MoveWrapper : T {
    template<typename U>
    explicit MoveWrapper(U&& u) : T(std::move(u)) {}

    MoveWrapper(MoveWrapper&&) = default;
    MoveWrapper& operator=(MoveWrapper&&) = default;

    // Horrible evil sorcery.
    MoveWrapper(const MoveWrapper& other)
      : T(std::move(const_cast<T&>(static_cast<const T&>(other))))
    {}
  };

  template< typename T
          , typename R = MoveWrapper<
                           PackagedTask<
                             Result<T>( const boost::system::error_code&
                                      , const T&)
                           >
                         >>
  R make_handler() {
    return R(to_result<T>);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Futurized wrapper for Socket::async_receive_from.
template<typename Socket, typename Buffers>
Future<Result<std::size_t>> receive_from(
    Socket&                         socket
  , const Buffers&                  buffers
  , typename Socket::endpoint_type& sender_endpoint)
{
  auto handler = detail::make_handler<std::size_t>();
  auto future  = handler.get_future();

  socket.async_receive_from(buffers, sender_endpoint, std::move(handler));
  return future;
}

////////////////////////////////////////////////////////////////////////////////
// Futurized wrapper for Socket::async_send_to.
template<typename Socket, typename Buffers>
Future<Result<std::size_t>> send_to(
    Socket&                               socket
  , const Buffers&                        buffers
  , const typename Socket::endpoint_type& destination)
{
  auto handler = detail::make_handler<std::size_t>();
  auto future  = handler.get_future();

  socket.async_send_to(buffers, destination, std::move(handler));
  return future;
}


} // namespace asio

#endif // __ASIO_H__