#include "UDPServer.h"
#include "Facility.h"
#include "Util.h"
#include <iostream>
#include <sstream>
#include <unordered_map>

using namespace std;
using namespace boost::asio;

UDPServer::UDPServer(io_context &io_context, short portNumber)
    : io_context_(io_context),
      port_(portNumber),
      socket_(io_context, udp::endpoint(udp::v4(), portNumber)) {
    cout << "Server started on port " << portNumber << endl;

    // Sample facility for testing
    // Correct way to initialize with a name
    facilities.emplace("Gym", Facility("Gym"));
    facilities.at("Gym").addAvailability(Facility::TimeSlot(Util::Day::Monday, 10, 0));
    facilities.at("Gym").addAvailability(Facility::TimeSlot(Util::Day::Tuesday, 14, 0));

    do_receive();
}

UDPServer::~UDPServer() { stop(); }

void UDPServer::start() { io_context_.run(); }

void UDPServer::stop() {
    socket_.close();
    cout << "Server stopped." << endl;
}

void UDPServer::do_receive() {
    socket_.async_receive_from(buffer(recv_buffer_), remote_endpoint_,
                               [this](boost::system::error_code ec, std::size_t bytes_recvd) {
                                   if (!ec && bytes_recvd > 0) {
                                       string request(recv_buffer_.data(), bytes_recvd);
                                       handle_receive(ec, bytes_recvd);
                                   }
                                   do_receive();  // Continue listening
                               });
}

void UDPServer::handle_receive(const boost::system::error_code &error, size_t bytes_transferred) {
    if (!error) {
        string message(recv_buffer_.data(), bytes_transferred);
        cout << "Received: " << message << " from " << remote_endpoint_ << endl;

        stringstream ss(message);
        string command, facilityName, dayStr;
        int hour, minute;
        ss >> command >> facilityName >> dayStr >> hour >> minute;

        string response;
        try {
            if (command == "QUERY") {
                response = queryAvailability(facilityName, dayStr, hour, minute);
            } else if (command == "BOOK") {
                response = bookFacility(facilityName, dayStr, hour, minute);
            } else if (command == "MONITOR") {
                monitoringClients[facilityName] = remote_endpoint_;
                response = "Monitoring started for " + facilityName;
            } else {
                response = "Error: Invalid command.";
            }
        } catch (const invalid_argument &e) {
            response = e.what();
        }

        do_send(response, remote_endpoint_);
    }
}

void UDPServer::do_send(const string &message, const udp::endpoint &endpoint) {
    socket_.async_send_to(buffer(message), endpoint,
                          [this](boost::system::error_code ec, std::size_t /*bytes_sent*/) {
                              if (ec) {
                                  cerr << "Error sending response: " << ec.message() << endl;
                              }
                          });
}

string UDPServer::queryAvailability(const string &facility, const string &dayStr, int hour,
                                    int minute) {
    auto it = facilities.find(facility);
    if (it == facilities.end()) {
        return "Error: Facility not found.";
    }

    Util::Day day = Util::stringToDay(dayStr);
    Facility::TimeSlot slot(day, hour, minute);
    it->second.displayAvailability();
    return it->second.isAvailable(slot)
               ? "Available: " + dayStr + " at " + to_string(hour) + ":" + to_string(minute)
               : "Not available: " + dayStr + " at " + to_string(hour) + ":" + to_string(minute);
}

string UDPServer::bookFacility(const string &facility, const string &dayStr, int hour, int minute) {
    auto it = facilities.find(facility);
    if (it == facilities.end()) {
        return "Error: Facility not found.";
    }

    Util::Day day = Util::stringToDay(dayStr);
    Facility::TimeSlot slot(day, hour, minute);
    if (it->second.bookSlot(slot)) {
        notifyClients(facility);
        return "Booking confirmed for " + facility + " on " + dayStr + " at " + to_string(hour) +
               ":" + to_string(minute);
    } else {
        return "Error: Slot not available.";
    }
}

void UDPServer::notifyClients(const string &facility) {
    auto it = monitoringClients.find(facility);
    if (it != monitoringClients.end()) {
        string update = "Update: Availability changed for " + facility;
        do_send(update, it->second);
    }
}
