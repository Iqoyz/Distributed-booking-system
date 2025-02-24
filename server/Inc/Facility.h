#ifndef FACILITY_H
#define FACILITY_H

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "Util.h"
#include <unordered_map>

class Facility {
  public:
    struct TimeSlot {
        Util::Day day;
        uint16_t startTime;  // HHMM
        uint16_t endTime;    // HHMM

        TimeSlot(Util::Day d, uint16_t start, uint16_t end)
            : day(d), startTime(start), endTime(end) {}

        std::string toString() const {
            auto start = Util::parseTime(startTime);
            auto end = Util::parseTime(endTime);

            return Util::dayToString(day) + " " + std::to_string(start.first) + ":" +
                   (start.second < 10 ? "0" : "") + std::to_string(start.second) + " to " +
                   std::to_string(end.first) + ":" + (end.second < 10 ? "0" : "") +
                   std::to_string(end.second);
        }
    };

    struct BookingInfo {
        TimeSlot slot;
        uint32_t bookingId;

        BookingInfo(const TimeSlot &s, uint32_t id) : slot(s), bookingId(id) {}
    };
    BookingInfo getBookingInfo(uint32_t bookingId) const;

    explicit Facility(const std::string &name);
    std::string getName() const;

    void addAvailability(const TimeSlot &slot);
    bool isAvailable(const TimeSlot &slot) const;
    bool bookSlot(const TimeSlot &slot, uint32_t &bookingId);
    bool cancelBooking(uint32_t bookingId);
    void displayAvailability(Util::Day day) const;
    void displayAllSlots(Util::Day day) const;

  private:
    std::string name;
    std::vector<TimeSlot> availableSlots;
    std::unordered_map<uint32_t, BookingInfo> bookings;  // Map of booking ID to booking info

    uint32_t generateBookingId();  // Private method to generate unique booking IDs
    void sortAvailableSlots();
};

#endif
