#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <winsock2.h>
#include "Util.h"
#include <optional>

enum class Operation : uint8_t {
    QUERY = 1,
    BOOK = 2,
    CHANGE = 3,
    MONITOR = 4,
    STATUS = 5,
    CANCEL = 6
};

/*
Example of Requests:
Query:
[RequestID][OpCode=1][FacilityNameLength][FacilityName][Day=0(Monday)][StartTime=1000][EndTime=1200]
Book:
[RequestID][OpCode=2][FacilityNameLength][FacilityName][Day=0(Monday)][StartTime=1000][EndTime=1200]
Monitor:
[RequestID][OpCode=4][FacilityNameLength][FacilityName][Day=0(Monday)][StartTime=1000][EndTime=1400][extraMessage=300(300s
monitor interval)]
*/

struct RequestMessage {
    uint32_t requestId;
    Operation operation;
    std::string facilityName;
    Util::Day day;
    uint16_t startTime;
    uint16_t endTime;
    std::optional<uint32_t> extraMessage;  // Optional Booking ID

    // Marshal: Convert RequestMessage to byte array
    std::vector<uint8_t> marshal() const {
        std::vector<uint8_t> buffer;

        // Request ID (32 bits)
        uint32_t netRequestId = htonl(requestId);
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&netRequestId),
                      reinterpret_cast<const uint8_t*>(&netRequestId) + sizeof(netRequestId));

        // Operation (1 byte)
        buffer.push_back(static_cast<uint8_t>(operation));

        // Facility Name Length (16 bits) and Facility Name
        uint16_t nameLength = htons(static_cast<uint16_t>(facilityName.size()));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&nameLength),
                      reinterpret_cast<const uint8_t*>(&nameLength) + sizeof(nameLength));
        buffer.insert(buffer.end(), facilityName.begin(), facilityName.end());

        // Day (1 byte)
        buffer.push_back(static_cast<uint8_t>(day));

        // Start Time (16 bits)
        uint16_t netStartTime = htons(startTime);
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&netStartTime),
                      reinterpret_cast<const uint8_t*>(&netStartTime) + sizeof(netStartTime));

        // End Time (16 bits)
        uint16_t netEndTime = htons(endTime);
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&netEndTime),
                      reinterpret_cast<const uint8_t*>(&netEndTime) + sizeof(netEndTime));

        // Optional extra message (if present)
        if (extraMessage.has_value()) {
            uint32_t netExtraMessage = htonl(extraMessage.value());
            buffer.insert(
                buffer.end(), reinterpret_cast<const uint8_t*>(&netExtraMessage),
                reinterpret_cast<const uint8_t*>(&netExtraMessage) + sizeof(netExtraMessage));
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

        // Facility Name Length and Name
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

        // Start Time
        std::memcpy(&msg.startTime, buffer.data() + offset, sizeof(msg.startTime));
        msg.startTime = ntohs(msg.startTime);
        offset += sizeof(msg.startTime);

        // End Time
        std::memcpy(&msg.endTime, buffer.data() + offset, sizeof(msg.endTime));
        msg.endTime = ntohs(msg.endTime);
        offset += sizeof(msg.endTime);

        // Optional extra message (if available)
        if (offset + sizeof(uint32_t) <= buffer.size()) {
            uint32_t netExtraMessage;
            std::memcpy(&netExtraMessage, buffer.data() + offset, sizeof(netExtraMessage));
            msg.extraMessage = ntohl(netExtraMessage);
        } else {
            msg.extraMessage.reset();  // No extra messaage provided
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
