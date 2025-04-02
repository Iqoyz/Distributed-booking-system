#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <stdexcept>

// Platform-specific headers
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "Util.h"
#include <optional>
#include <boost/asio.hpp>

enum class Operation : uint8_t {
    QUERY = 1,
    BOOK = 2,
    CHANGE = 3,
    MONITOR = 4,
    EXTEND = 5,
    CANCEL = 6
};

/*
Example of Requests:
Query:
[RequestID][OpCode=1][FacilityNameLength][FacilityName][Day=0(Monday)][StartTime=1000][EndTime=1200]

Book:
[RequestID][OpCode=2][FacilityNameLength][FacilityName][Day=0(Monday)][StartTime=1000][EndTime=1200]

Change (Modify):
[RequestID][OpCode=3][FacilityNameLength][FacilityName][Day=0(Monday)][StartTime=1000][EndTime=1200]
[extraMessage=1000 and 30 (Booking ID=1000)(OffsetMinutes=30)]

Monitor:
[RequestID][OpCode=4][FacilityNameLength][FacilityName][Day=0(Monday)][StartTime=1000][EndTime=1400]
[extraMessage=300 (300s monitor interval)]

Extend:
[RequestID][OpCode=5][FacilityNameLength][FacilityName][Day=0(Monday)][StartTime=1000][EndTime=1200]
[extraMessage=1000 and 30 (Booking ID=1000)(ExtendMinutes=30)]

Cancel:
[RequestID][OpCode=6][FacilityNameLength][FacilityName][Day=0(Monday)][StartTime=1000][EndTime=1200]
[extraMessage=1000 (Booking ID=1000)]
*/

struct RequestMessage {
    uint32_t requestId;
    boost::asio::ip::udp::endpoint clientEndpoint;
    Operation operation;
    std::string facilityName;
    Util::Day day;
    uint16_t startTime;
    uint16_t endTime;

    std::optional<uint32_t> bookingId;        // Cancel & Modify
    std::optional<int> offsetMinutes;         // Modify only
    std::optional<uint32_t> monitorInterval;  // Monitor only

    // Generate a unique key combining request ID and client address
    std::string getUniqueRequestKey() const {
        return std::to_string(requestId) + "-" + clientEndpoint.address().to_string() + ":" +
               std::to_string(clientEndpoint.port());
    }

    // Marshal: Convert RequestMessage to byte array
    std::vector<uint8_t> marshal() const {
        std::vector<uint8_t> buffer;

        // Request ID
        uint32_t netRequestId = htonl(requestId);
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&netRequestId),
                      reinterpret_cast<const uint8_t*>(&netRequestId) + sizeof(netRequestId));

        // Operation
        buffer.push_back(static_cast<uint8_t>(operation));

        // Facility Name
        uint16_t nameLength = htons(static_cast<uint16_t>(facilityName.size()));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&nameLength),
                      reinterpret_cast<const uint8_t*>(&nameLength) + sizeof(nameLength));
        buffer.insert(buffer.end(), facilityName.begin(), facilityName.end());

        // Day
        buffer.push_back(static_cast<uint8_t>(day));

        // Start and End Times
        uint16_t netStart = htons(startTime);
        uint16_t netEnd = htons(endTime);
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&netStart),
                      reinterpret_cast<const uint8_t*>(&netStart) + sizeof(netStart));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&netEnd),
                      reinterpret_cast<const uint8_t*>(&netEnd) + sizeof(netEnd));

        // Operation-specific extra fields
        switch (operation) {
            case Operation::CANCEL:
                if (bookingId.has_value()) {
                    uint32_t netId = htonl(bookingId.value());
                    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&netId),
                                  reinterpret_cast<const uint8_t*>(&netId) + sizeof(netId));
                }
                break;

            case Operation::EXTEND:
                if (bookingId.has_value() && offsetMinutes.has_value()) {
                    uint32_t netId = htonl(bookingId.value());
                    int32_t netOffset = htonl(offsetMinutes.value());
                    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&netId),
                                  reinterpret_cast<const uint8_t*>(&netId) + sizeof(netId));
                    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&netOffset),
                                  reinterpret_cast<const uint8_t*>(&netOffset) + sizeof(netOffset));
                }
                break;

            case Operation::CHANGE:
                if (bookingId.has_value() && offsetMinutes.has_value()) {
                    uint32_t netId = htonl(bookingId.value());
                    int32_t netOffset = htonl(offsetMinutes.value());
                    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&netId),
                                  reinterpret_cast<const uint8_t*>(&netId) + sizeof(netId));
                    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&netOffset),
                                  reinterpret_cast<const uint8_t*>(&netOffset) + sizeof(netOffset));
                }
                break;

            case Operation::MONITOR:
                if (monitorInterval.has_value()) {
                    uint32_t netInterval = htonl(monitorInterval.value());
                    buffer.insert(
                        buffer.end(), reinterpret_cast<const uint8_t*>(&netInterval),
                        reinterpret_cast<const uint8_t*>(&netInterval) + sizeof(netInterval));
                }
                break;

            default:
                break;
        }

        return buffer;
    }

    // Unmarshal: Convert byte array to RequestMessage
    static RequestMessage unmarshal(const std::vector<uint8_t>& buffer) {
        if (buffer.size() < 15) {
            throw std::runtime_error("Buffer too small to unmarshal RequestMessage.");
        }

        RequestMessage msg;
        size_t offset = 0;

        // Request ID
        uint32_t netRequestId;
        std::memcpy(&netRequestId, buffer.data() + offset, sizeof(netRequestId));
        msg.requestId = ntohl(netRequestId);
        offset += sizeof(netRequestId);

        // Operation
        msg.operation = static_cast<Operation>(buffer[offset++]);

        // Facility Name
        uint16_t nameLength;
        std::memcpy(&nameLength, buffer.data() + offset, sizeof(nameLength));
        nameLength = ntohs(nameLength);
        offset += sizeof(nameLength);

        if (offset + nameLength > buffer.size()) {
            throw std::runtime_error("Buffer overflow while reading facility name.");
        }

        msg.facilityName.assign(buffer.begin() + offset, buffer.begin() + offset + nameLength);
        offset += nameLength;

        // Day
        msg.day = static_cast<Util::Day>(buffer[offset++]);

        // Start & End Times
        std::memcpy(&msg.startTime, buffer.data() + offset, sizeof(msg.startTime));
        msg.startTime = ntohs(msg.startTime);
        offset += sizeof(msg.startTime);

        std::memcpy(&msg.endTime, buffer.data() + offset, sizeof(msg.endTime));
        msg.endTime = ntohs(msg.endTime);
        offset += sizeof(msg.endTime);

        // Extract operation-specific extras
        switch (msg.operation) {
            case Operation::CANCEL:
                if (offset + sizeof(uint32_t) <= buffer.size()) {
                    uint32_t netBookingId;
                    std::memcpy(&netBookingId, buffer.data() + offset, sizeof(netBookingId));
                    msg.bookingId = ntohl(netBookingId);
                    offset += sizeof(netBookingId);
                }
                break;

            case Operation::CHANGE:
                if (offset + sizeof(uint32_t) + sizeof(int32_t) <= buffer.size()) {
                    uint32_t netBookingId;
                    int32_t netOffset;
                    std::memcpy(&netBookingId, buffer.data() + offset, sizeof(netBookingId));
                    offset += sizeof(netBookingId);
                    std::memcpy(&netOffset, buffer.data() + offset, sizeof(netOffset));
                    offset += sizeof(netOffset);

                    msg.bookingId = ntohl(netBookingId);
                    msg.offsetMinutes = ntohl(netOffset);
                }
                break;

            case Operation::EXTEND:
                if (offset + sizeof(uint32_t) + sizeof(int32_t) <= buffer.size()) {
                    uint32_t netBookingId;
                    int32_t netOffset;
                    std::memcpy(&netBookingId, buffer.data() + offset, sizeof(netBookingId));
                    offset += sizeof(netBookingId);
                    std::memcpy(&netOffset, buffer.data() + offset, sizeof(netOffset));
                    offset += sizeof(netOffset);

                    msg.bookingId = ntohl(netBookingId);
                    msg.offsetMinutes = ntohl(netOffset);
                }
                break;

            case Operation::MONITOR:
                if (offset + sizeof(uint32_t) <= buffer.size()) {
                    uint32_t netInterval;
                    std::memcpy(&netInterval, buffer.data() + offset, sizeof(netInterval));
                    msg.monitorInterval = ntohl(netInterval);
                    offset += sizeof(netInterval);
                }
                break;

            default:
                break;
        }

        return msg;
    }
};

/*
Example of respone:
Success: [RequestID][Status=0][MsgLen=30][Booking confirmed: ID 12345]
Error: [RequestID][Status=1][MsgLen=20][Error: Slot not available]
*/

struct ResponseMessage {
    uint32_t requestId;
    uint8_t status;       // 0 = success, 1 = error
    std::string message;  // Human-readable message

    // Marshal: Convert ResponseMessage to byte array
    std::vector<uint8_t> marshal() const {
        std::vector<uint8_t> buffer;
        uint32_t netRequestId = htonl(requestId);

        // Write Request ID
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&netRequestId),
                      reinterpret_cast<const uint8_t*>(&netRequestId) + sizeof(netRequestId));

        // Write Status
        buffer.push_back(status);

        // Write Message Length
        uint16_t messageLength = htons(static_cast<uint16_t>(message.size()));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&messageLength),
                      reinterpret_cast<const uint8_t*>(&messageLength) + sizeof(messageLength));

        // Write Message Content
        buffer.insert(buffer.end(), message.begin(), message.end());

        return buffer;
    }

    // Unmarshal: Convert byte array to ResponseMessage
    static ResponseMessage unmarshal(const std::vector<uint8_t>& buffer) {
        if (buffer.size() < 7) {  // Minimum size check
            throw std::runtime_error("Buffer too small to unmarshal ResponseMessage.");
        }

        ResponseMessage msg;
        size_t offset = 0;

        // Read Request ID
        uint32_t netRequestId;
        std::memcpy(&netRequestId, buffer.data() + offset, sizeof(netRequestId));
        msg.requestId = ntohl(netRequestId);
        offset += sizeof(netRequestId);

        // Read Status
        msg.status = buffer[offset++];

        // Read Message Length
        uint16_t messageLength;
        std::memcpy(&messageLength, buffer.data() + offset, sizeof(messageLength));
        messageLength = ntohs(messageLength);
        offset += sizeof(messageLength);

        // Read Message Content
        if (offset + messageLength > buffer.size()) {
            throw std::runtime_error("Buffer overflow while reading response message.");
        }

        msg.message.assign(buffer.begin() + offset, buffer.begin() + offset + messageLength);
        return msg;
    }
};

#endif
