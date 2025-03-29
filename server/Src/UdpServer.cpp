#include "UdpServer.h"
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

    auto now = std::chrono::steady_clock::now();
    auto it = processedRequests.find(requestKey);
    // how long we want to retain old requests (e.g., 30s)for demo purposes
    int expirySeconds = 30;

    bool isDuplicate = false;

    if (!atLeastOnce_ && it != processedRequests.end()) {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count();
        if (age <= expirySeconds) {
            isDuplicate = true;
        }
    }

    // **At-Most-Once Handling**: Ignore duplicate requests
    if (isDuplicate) {
        response.message = "Ignoring duplicate request: " + requestKey;
        response.status = 1;
    } else {
        if (!atLeastOnce_) {
            // Clean up if over capacity
            if (processedRequests.size() >= MAX_PROCESSED_REQUESTS) {
                const std::string &oldest = requestOrder.front();
                processedRequests.erase(oldest);
                requestOrder.pop();
            }
            // Insert new request, (insert or update timestamp)
            processedRequests[requestKey] = now;
            requestOrder.push(requestKey);
        }
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

                case Operation::CHANGE:
                    if (!request.bookingId.has_value() || !request.offsetMinutes.has_value()) {
                        throw std::runtime_error(
                            "Booking ID and offset are required for modification.");
                    }
                    response.status = 0;
                    response.message =
                        modifyBookFacility(request.facilityName, request.bookingId.value(),
                                           request.offsetMinutes.value());
                    break;
                case Operation::EXTEND:
                    if (!request.bookingId.has_value() || !request.offsetMinutes.has_value()) {
                        throw std::runtime_error("Booking ID and extension duration required.");
                    }
                    response.status = 0;
                    response.message =
                        extendBookFacility(request.facilityName, request.bookingId.value(),
                                           request.offsetMinutes.value());
                    break;

                case Operation::CANCEL:
                    if (!request.bookingId.has_value()) {
                        throw std::runtime_error("Booking ID required for cancellation.");
                    }
                    response.status = 0;
                    response.message =
                        cancelBookFacility(request.facilityName, request.bookingId.value());
                    break;

                case Operation::MONITOR:
                    if (!request.monitorInterval.has_value()) {
                        throw std::runtime_error("Monitor interval is required.");
                    }
                    response.status = 0;
                    response.message = registerMonitorClient(
                        request.facilityName, request.day, request.startTime, request.endTime,
                        request.monitorInterval.value(), remote_endpoint_);
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

    // Send response
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

void UDPServer::do_send_reliable(const std::string &message, const udp::endpoint &endpoint) {
    socket_.async_send_to(buffer(message), endpoint,
                          [this](boost::system::error_code ec, std::size_t /*bytes_sent*/) {
                              if (ec) {
                                  std::cerr << "Error sending reliable response: " << ec.message()
                                            << std::endl;
                              }
                          });
}

std::string UDPServer::queryAvailability(const std::string &facility, const Util::Day &day,
                                         uint16_t startTime, uint16_t endTime) {
    Facility &f = getFacilityOrThrow(facility);  // throws if not found

    Facility::TimeSlot slot(day, startTime, endTime);

    f.displayAllSlots(day);
    f.displayAvailability(day);

    return f.isAvailable(slot) ? "Slot available: " + slot.toString()
                               : "Slot not available: " + slot.toString();
}

std::string UDPServer::bookFacility(const std::string &facility, const Util::Day &day,
                                    uint16_t startTime, uint16_t endTime) {
    Facility &f = getFacilityOrThrow(facility);  // throws if not found

    Facility::TimeSlot slot(day, startTime, endTime);
    uint32_t bookingId;

    if (f.bookSlot(slot, bookingId)) {
        notifyMonitorClients(facility, startTime, endTime);
        return "Booking confirmed for " + facility + " on " + slot.toString() +
               ". Booking ID: " + std::to_string(bookingId);
    } else {
        return "Slot not available.";
    }
}

std::string UDPServer::modifyBookFacility(std::string &facility, uint32_t bookingId,
                                          int offsetMinutes) {
    Facility &f = getFacilityOrThrow(facility);  // Throws if facility doesn't exist

    std::string errorMessage;

    // Get original booking slot before modification
    Facility::BookingInfo oldBooking = f.getBookingInfo(bookingId);
    Facility::TimeSlot oldSlot = oldBooking.slot;

    // Attempt modification
    if (!f.modifyBooking(bookingId, offsetMinutes, errorMessage)) {
        throw std::runtime_error("Failed to modify booking: " + errorMessage);
    }

    Facility::BookingInfo newBooking = f.getBookingInfo(bookingId);
    Facility::TimeSlot newSlot = newBooking.slot;

    // Combine overlapping or adjacent time slots
    uint16_t combinedStart = oldSlot.startTime;
    uint16_t combinedEnd = oldSlot.endTime;

    if ((oldSlot.endTime >= newSlot.startTime && oldSlot.startTime <= newSlot.endTime) ||
        oldSlot.endTime == newSlot.startTime || newSlot.endTime == oldSlot.startTime) {
        combinedStart = std::min(oldSlot.startTime, newSlot.startTime);
        combinedEnd = std::max(oldSlot.endTime, newSlot.endTime);

        std::cout << "[Server] Combined timeslot: " << combinedStart << " to " << combinedEnd
                  << "\n";

        // call notifyMonitorClients with this combined range
        notifyMonitorClients(facility, combinedStart, combinedEnd);
    } else {
        // No overlap, notify both separately
        notifyMonitorClients(facility, oldSlot.startTime, oldSlot.endTime);
        notifyMonitorClients(facility, newSlot.startTime, newSlot.endTime);
    }

    return "Booking with ID " + std::to_string(bookingId) + " modified successfully to " +
           newSlot.toString() + ".";
}

std::string UDPServer::extendBookFacility(std::string &facility, uint32_t bookingId,
                                          int extensionMinutes) {
    Facility &f = getFacilityOrThrow(facility);  // throws if not found

    // Get original slot before extension
    Facility::BookingInfo oldBooking = f.getBookingInfo(bookingId);
    Facility::TimeSlot oldSlot = oldBooking.slot;

    std::string errorMessage;
    if (!f.extendBooking(bookingId, extensionMinutes, errorMessage)) {
        throw std::runtime_error("Failed to extend booking: " + errorMessage);
    }

    // Get the updated slot after extension
    Facility::BookingInfo newBooking = f.getBookingInfo(bookingId);
    Facility::TimeSlot newSlot = newBooking.slot;

    // Notify only for the newly added portion
    if (newSlot.endTime > oldSlot.endTime) {
        notifyMonitorClients(facility, oldSlot.endTime, newSlot.endTime);
    } else if (newSlot.startTime < oldSlot.startTime) {
        notifyMonitorClients(facility, newSlot.startTime, oldSlot.startTime);
    }

    return "Booking with ID " + std::to_string(bookingId) + " extended successfully to " +
           newSlot.toString() + ".";
}

std::string UDPServer::cancelBookFacility(const std::string &facility, uint32_t bookingId) {
    Facility &f = getFacilityOrThrow(facility);  // Throws if facility doesn't exist

    auto cancelledSlot = f.cancelBooking(bookingId);
    if (cancelledSlot.has_value()) {
        notifyMonitorClients(facility, cancelledSlot->startTime, cancelledSlot->endTime);
        return "Booking with ID " + std::to_string(bookingId) + " canceled successfully.";
    } else {
        return "Invalid booking ID.";
    }
}

std::string UDPServer::registerMonitorClient(const std::string &facility, const Util::Day &day,
                                             uint16_t startTime, uint16_t endTime,
                                             uint32_t interval,
                                             const udp::endpoint &clientEndpoint) {
    Facility &f = getFacilityOrThrow(facility);  // Throws if facility doesn't exist

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

    auto monitorInfoCopy = monitorInfo;
    // Timer to auto-expire after the interval ends
    monitorInfo.timer->async_wait(
        [this, facility, monitorInfoCopy, clientEndpoint](const boost::system::error_code &ec) {
            if (!ec) {
                std::cout << "[Server] Monitoring expired for client: " << clientEndpoint
                          << " on facility: " << facility << " for " << monitorInfoCopy.startTime
                          << " to " << monitorInfoCopy.endTime << std::endl;
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

    auto facilityPtr = facilities.find(facility);
    if (facilityPtr == facilities.end()) return;

    const Facility &fac = facilityPtr->second;

    ResponseMessage response;

    for (auto &[day, monitorMap] : facilityIt->second) {
        for (uint16_t t = changedStartTime; t < changedEndTime;
             t = Util::toHHMM(Util::toMinutes(t) + 30)) {
            uint16_t subStart = t;
            uint16_t subEnd = Util::toHHMM(Util::toMinutes(t) + 30);

            Facility::TimeSlot slot(day, subStart, subEnd);
            bool isAvailable = fac.isAvailable(slot);
            std::string availabilityStatus = isAvailable ? "available" : "not available";

            std::set<std::pair<uint16_t, uint16_t>> notified;

            for (auto &[monitorStart, info] : monitorMap) {
                if (info.endTime > subStart && monitorStart < subEnd) {
                    // Build response
                    response.message = "Update: Availability for " + facility + " from " +
                                       std::to_string(subStart) + " to " + std::to_string(subEnd) +
                                       " changed to " + availabilityStatus + ".";
                    response.status = 0;

                    auto responseData = response.marshal();
                    do_send_reliable(std::string(responseData.begin(), responseData.end()),
                                     info.clientEndpoint);
                }
            }
        }
    }
}

Facility &UDPServer::getFacilityOrThrow(const std::string &facilityName) {
    auto it = facilities.find(facilityName);
    if (it == facilities.end()) {
        throw std::runtime_error("Facility '" + facilityName + "' not found!");
    }
    return it->second;
}
