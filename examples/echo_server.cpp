#include <boost/asio.hpp>
#include "fry.h"
#include "fry/asio.h"

using namespace boost::asio;
using namespace fry;
using ip::udp;

////////////////////////////////////////////////////////////////////////////////

class Server {
public:
  Server(io_service& io_service, short port)
    : _socket(io_service, udp::endpoint(udp::v4(), port))
  {
    receive();
  }

  void receive() {
    _socket.async_receive_from(
      buffer(_data, max_length), _sender_endpoint, asio::use_future
    ).then([=](std::size_t length) {
      if (length > 0) {
        return _socket.async_send_to( buffer(_data, length)
                                    , _sender_endpoint
                                    , asio::use_future);
      } else {
        return asio::make_ready_future(std::size_t(0));
      }
    }).always([=]() {
      receive();
    });
  }

  // void do_receive()
  // {
  //   _socket.async_receive_from(
  //       boost::asio::buffer(_data, max_length), _sender_endpoint,
  //       [this](boost::system::error_code ec, std::size_t bytes_recvd)
  //       {
  //         if (!ec && bytes_recvd > 0)
  //         {
  //           do_send(bytes_recvd);
  //         }
  //         else
  //         {
  //           do_receive();
  //         }
  //       });
  // }

  // void do_send(std::size_t length)
  // {
  //   _socket.async_send_to(
  //       boost::asio::buffer(_data, length), _sender_endpoint,
  //       [this](boost::system::error_code, std::size_t)
  //       {
  //         do_receive();
  //       });
  // }

private:

  enum { max_length = 1024 };

  udp::socket   _socket;
  udp::endpoint _sender_endpoint;
  char          _data[max_length];
};

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage: echo_server <port>\n";
    return 1;
  }

  io_service io_service;
  Server server(io_service, std::atoi(argv[1]));

  io_service.run();
  return 0;
}
