#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <array>
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include "Facility.h"

using namespace boost::asio;
using boost::asio::ip::udp;
using namespace std;

class UDPServer {
  public:
    UDPServer(io_context &io_context, short portNumber,
              std::unordered_map<std::string, Facility> facilities, bool atLeastOnce);
    ~UDPServer();

    void start();  // Start the server
    void stop();   // stop the server

  private:
    struct MonitorInfo {
        udp::endpoint clientEndpoint;  // Client's IP and port
        Util::Day day;
        uint16_t startTime;
        uint16_t endTime;
        uint32_t monitorInterval;  // Monitor duration in seconds
        std::shared_ptr<boost::asio::steady_timer> timer;
    };

    // Boost Asio context and socket
    io_context &io_context_;
    short port_;
    udp::socket socket_;
    udp::endpoint remote_endpoint_;
    array<char, 1024> recv_buffer_;
    bool atLeastOnce_;  // True for At-Least-Once, False for At-Most-Once

    const float SEND_SUCCESS_RATE =
        0;  // 100% success rate for send message (the smaller , the higher rate)

    // Facility and client management
    unordered_map<string, Facility> facilities;

    // Store processed request keys
    unordered_set<string> processedRequests;

    // for monitoring clients
    using TimeRangeMap = multimap<uint16_t, MonitorInfo>;  // map startTime
    using DayMap = unordered_map<Util::Day, TimeRangeMap>;
    using FacilityMonitorMap = unordered_map<string, DayMap>;
    FacilityMonitorMap monitoringClients;

    void do_receive();  // Async receive function
    void handle_receive(const boost::system::error_code &error,
                        size_t bytes_transferred);                       // Handle incoming request
    void do_send(const string &message, const udp::endpoint &endpoint);  // Send response

    // Facility operations
    string queryAvailability(const string &facility, const Util::Day &day, uint16_t startTime,
                             uint16_t endTime);

    string bookFacility(const string &facility, const Util::Day &day, uint16_t startTime,
                        uint16_t endTime);

    string cancelBookFacility(const string &facility, uint32_t bookingId);

    string registerMonitorClient(const std::string &facility, const Util::Day &day,
                                 uint16_t startTime, uint16_t endTime, uint32_t interval,
                                 const udp::endpoint &clientEndpoint);

    void removeMonitorClient(const std::string &facility, const udp::endpoint &clientEndpoint);

    void notifyMonitorClients(
        const std::string &facility, uint16_t changedStartTime,
        uint16_t changedEndTime);  // Notify monitoring clients when availability changes
};

#endif  // UDP_SERVER_H
