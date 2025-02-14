#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <boost/asio.hpp>
#include <array>
#include <iostream> 
#include <string>    

using namespace boost::asio;
using boost::asio::ip::udp;
using namespace std;  

class UDPServer {
public:
    UDPServer(io_context& io_context, short port);
    ~UDPServer();

    void start();
    void stop();

private:
    io_context& io_context_; 
    short port_;                          
    udp::socket socket_;                  
    udp::endpoint remote_endpoint_;      
    array<char, 1024> recv_buffer_;

    // Begin asynchronous receive operations
    void do_receive();

    // Process incoming data
    void handle_receive(const boost::system::error_code& error, size_t bytes_transferred);

    // Send data to clients
    void do_send(const string& message, const udp::endpoint& endpoint);
};

#endif 
