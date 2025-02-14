#include "udp_server.h"

UDPServer::UDPServer(io_context& io_context, short port)
    : io_context_(io_context), 
      port_(port),
      socket_(io_context, ip::udp::endpoint(ip::udp::v4(), port)) {
    cout << "Server started on port " << port << endl;
}



