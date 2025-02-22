#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <array>
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
#include "Facility.h"

using namespace boost::asio;
using boost::asio::ip::udp;
using namespace std;

class UDPServer {
  public:
    UDPServer(io_context &io_context, short portNumber);  // Constructor: Initialize UDP server
    ~UDPServer();

    void start();  // Start the server
    void stop();   // stop the server

  private:
    // Boost Asio context and socket
    io_context &io_context_;
    short port_;
    udp::socket socket_;
    udp::endpoint remote_endpoint_;
    array<char, 1024> recv_buffer_;

    // Facility and client management
    unordered_map<string, Facility> facilities;
    unordered_map<string, udp::endpoint> monitoringClients;

    void do_receive();  // Async receive function
    void handle_receive(const boost::system::error_code &error,
                        size_t bytes_transferred);                       // Handle incoming request
    void do_send(const string &message, const udp::endpoint &endpoint);  // Send response

    // Facility operations
    string queryAvailability(const string &facility, const string &day, int hour, int minute);
    string bookFacility(const string &facility, const string &day, int hour, int minute);
    void notifyClients(
        const string &facility);  // Notify monitoring clients when availability changes
};

#endif  // UDP_SERVER_H
