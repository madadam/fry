#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::udp;

enum { max_length = 1024 };

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: echo_client <host> <port>\n";
    return 1;
  }

  boost::asio::io_service io_service;

  udp::socket s(io_service, udp::endpoint(udp::v4(), 0));

  udp::resolver resolver(io_service);
  udp::endpoint endpoint = *resolver.resolve({udp::v4(), argv[1], argv[2]});

  std::cout << "Enter message: ";
  char request[max_length];
  std::cin.getline(request, max_length);
  size_t request_length = std::strlen(request);
  s.send_to(boost::asio::buffer(request, request_length), endpoint);

  char reply[max_length];
  udp::endpoint sender_endpoint;
  size_t reply_length = s.receive_from(
      boost::asio::buffer(reply, max_length), sender_endpoint);
  std::cout << "Reply is: ";
  std::cout.write(reply, reply_length);
  std::cout << "\n";

  return 0;
}