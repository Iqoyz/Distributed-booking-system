#include "UDPServer.h"
#include "Facility.h"
#include "Util.h"
#include <iostream>
#include <sstream>
#include "Message.h"

using namespace std;
using namespace boost::asio;

UDPServer::UDPServer(io_context &io_context, short portNumber,
                     std::unordered_map<std::string, Facility> facilities, bool atLeastOnce)
    : io_context_(io_context),
      port_(portNumber),
      facilities(std::move(facilities)),  // Move the facilities into the member variable
      socket_(io_context, udp::endpoint(udp::v4(), portNumber)),
      atLeastOnce_(atLeastOnce) {
    cout << "[Server] Server started on port " << portNumber << " with "
         << (atLeastOnce ? "At-Least-Once" : "At-Most-Once") << " mode." << endl;
    do_receive();
}

UDPServer::~UDPServer() { stop(); }

void UDPServer::start() { io_context_.run(); }

void UDPServer::stop() {
    socket_.close();
    cout << "[Server] Server stopped." << endl;
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
    if (error) return;  // Early return on error

    std::vector<uint8_t> requestData(recv_buffer_.begin(),
                                     recv_buffer_.begin() + bytes_transferred);

    RequestMessage request = RequestMessage::unmarshal(requestData);

    request.clientEndpoint = remote_endpoint_;
    std::string requestKey = request.getUniqueRequestKey();

    ResponseMessage response;
    response.requestId = request.requestId;

    // **At-Most-Once Handling**: Ignore duplicate requests
    if (!atLeastOnce_ && processedRequests.find(requestKey) != processedRequests.end()) {
        response.message = "Ignoring duplicate request: " + requestKey;
        response.status = 1;
    } else {
        if (!atLeastOnce_) processedRequests.insert(requestKey);  // Insert only if unique

        // Process the request
        try {
            switch (request.operation) {
                case Operation::QUERY:
                    response.status = 0;
                    response.message = queryAvailability(request.facilityName, request.day,
                                                         request.startTime, request.endTime);
                    break;

                case Operation::BOOK:
                    response.status = 0;
                    response.message = bookFacility(request.facilityName, request.day,
                                                    request.startTime, request.endTime);
                    break;

                case Operation::CANCEL:
                    if (!request.extraMessage.has_value())
                        throw std::runtime_error("Booking ID required for cancellation.");

                    response.status = 0;
                    response.message =
                        cancelBookFacility(request.facilityName, request.extraMessage.value());
                    break;

                case Operation::MONITOR:
                    if (!request.extraMessage.has_value())
                        throw std::runtime_error(
                            "Monitor interval is missing for monitor request.");

                    response.status = 0;
                    response.message = registerMonitorClient(
                        request.facilityName, request.day, request.startTime, request.endTime,
                        request.extraMessage.value(), remote_endpoint_);
                    break;

                default:
                    response.status = 1;
                    response.message = "Invalid operation.";
                    break;
            }
        } catch (const std::exception &e) {
            response.status = 1;
            response.message = e.what();
        }
    }

    // Send Response
    auto responseData = response.marshal();
    do_send(std::string(responseData.begin(), responseData.end()), remote_endpoint_);
}

void UDPServer::do_send(const string &message, const udp::endpoint &endpoint) {
    if (Util::generateFpRandNumber() >= SEND_SUCCESS_RATE) {  // simulate the rate of loss
        socket_.async_send_to(buffer(message), endpoint,
                              [this](boost::system::error_code ec, std::size_t /*bytes_sent*/) {
                                  if (ec) {
                                      cerr << "Error sending response: " << ec.message() << endl;
                                  }
                              });
    } else {
        std::cout << "[Server] send failed. (simulated)" << std::endl;
    }
}

string UDPServer::queryAvailability(const string &facility, const Util::Day &day,
                                    uint16_t startTime, uint16_t endTime) {
    auto it = facilities.find(facility);
    if (it == facilities.end()) {
        throw std::runtime_error("Facility not found!");
    }

    Facility::TimeSlot slot(day, startTime, endTime);
    it->second.displayAllSlots(day);
    it->second.displayAvailability(day);
    return it->second.isAvailable(slot) ? "Slot available: " + slot.toString()
                                        : "Slot not available: " + slot.toString();
}

string UDPServer::bookFacility(const string &facility, const Util::Day &day, uint16_t startTime,
                               uint16_t endTime) {
    auto it = facilities.find(facility);
    if (it == facilities.end()) {
        throw std::runtime_error("Facility not found!");
    }

    Facility::TimeSlot slot(day, startTime, endTime);
    uint32_t bookingId;

    if (it->second.bookSlot(slot, bookingId)) {
        notifyMonitorClients(facility, startTime, endTime);
        return "Booking confirmed for " + facility + " on " + slot.toString() +
               ". Booking ID: " + std::to_string(bookingId);
    } else {
        return "Slot not available.";
    }
}

string UDPServer::cancelBookFacility(const string &facility, uint32_t bookingId) {
    auto it = facilities.find(facility);
    if (it == facilities.end()) {
        throw std::runtime_error("Facility not found!");
    }

    // Retrieve the booking details before canceling
    auto booking =
        it->second.getBookingInfo(bookingId);  // Ensure you have a way to get booking info

    if (it->second.cancelBooking(bookingId)) {
        // Notify monitoring clients about the cancellation
        notifyMonitorClients(facility, booking.slot.startTime, booking.slot.endTime);

        return "Booking with ID " + std::to_string(bookingId) + " canceled successfully.";
    } else {
        return "Invalid booking ID.";
    }
}

string UDPServer::registerMonitorClient(const std::string &facility, const Util::Day &day,
                                        uint16_t startTime, uint16_t endTime, uint32_t interval,
                                        const udp::endpoint &clientEndpoint) {
    auto it = facilities.find(facility);
    if (it == facilities.end()) {
        throw std::runtime_error("Facility not found!");
    }

    // Create MonitorInfo with the desired timeslot
    MonitorInfo monitorInfo = {
        clientEndpoint,
        day,
        startTime,
        endTime,
        interval,
        std::make_shared<boost::asio::steady_timer>(io_context_, std::chrono::seconds(interval))};

    // Store monitor info in the map
    monitoringClients[facility][day].emplace(startTime, monitorInfo);
    // Timer to auto-expire after the interval ends
    monitorInfo.timer->async_wait(
        [this, facility, &monitorInfo, clientEndpoint](const boost::system::error_code &ec) {
            if (!ec) {
                std::cout << "[Server] Monitoring expired for client: " << clientEndpoint
                          << " on facility: " << facility << " for " << monitorInfo.startTime
                          << " to " << monitorInfo.endTime << std::endl;
                removeMonitorClient(facility, clientEndpoint);
            }
        });

    return "Client registered to monitor " + facility + " from " + std::to_string(startTime) +
           " to " + std::to_string(endTime) + " for " + std::to_string(interval) + " seconds.\n";
}

void UDPServer::removeMonitorClient(const std::string &facility,
                                    const udp::endpoint &clientEndpoint) {
    auto facilityIt = monitoringClients.find(facility);
    if (facilityIt == monitoringClients.end()) return;

    for (auto &[day, monitorMap] : facilityIt->second) {
        for (auto it = monitorMap.begin(); it != monitorMap.end();) {
            if (it->second.clientEndpoint == clientEndpoint) {
                it = monitorMap.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void UDPServer::notifyMonitorClients(const std::string &facility, uint16_t changedStartTime,
                                     uint16_t changedEndTime) {
    auto facilityIt = monitoringClients.find(facility);
    if (facilityIt == monitoringClients.end()) return;
    ResponseMessage response;

    for (auto &[day, monitorMap] : facilityIt->second) {
        for (auto it = monitorMap.lower_bound(changedStartTime);
             it != monitorMap.end() && it->first < changedEndTime; it++) {
            const MonitorInfo &info = it->second;
            if (info.endTime > changedStartTime) {
                // Check availability
                auto facilityIt = facilities.find(facility);
                Facility::TimeSlot slot(day, changedStartTime, changedEndTime);
                bool isAvailable = facilityIt->second.isAvailable(slot);
                std::string availabilityStatus = isAvailable ? "available" : "not available";

                // Prepare the notification message
                response.message = "Update: Availability for " + facility + " from " +
                                   std::to_string(changedStartTime) + " to " +
                                   std::to_string(changedEndTime) + " changed to " +
                                   availabilityStatus + ".";
                response.status = 0;

                // Send the response to the monitoring client
                auto responseData = response.marshal();
                do_send(std::string(responseData.begin(), responseData.end()), info.clientEndpoint);
            }
        }
    }
}
