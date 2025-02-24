#include "Facility.h"

Facility::BookingInfo Facility::getBookingInfo(uint32_t bookingId) const {
    auto it = bookings.find(bookingId);
    if (it != bookings.end()) {
        return it->second;  // Return the booking info if found
    } else {
        throw std::runtime_error("Booking ID not found.");
    }
}

Facility::Facility(const std::string& name) : name(name) {}

std::string Facility::getName() const { return name; }

void Facility::addAvailability(const TimeSlot& slot) {
    availableSlots.push_back(slot);
    sortAvailableSlots();
}

bool Facility::isAvailable(const TimeSlot& slot) const {
    for (const auto& s : availableSlots) {
        if (s.day == slot.day && s.startTime == slot.startTime && s.endTime == slot.endTime) {
            return true;
        }
    }
    return false;
}

bool Facility::bookSlot(const TimeSlot& slot, uint32_t& bookingId) {
    for (size_t i = 0; i < availableSlots.size(); ++i) {
        if (availableSlots[i].day == slot.day && availableSlots[i].startTime == slot.startTime &&
            availableSlots[i].endTime == slot.endTime) {
            bookingId = generateBookingId();

            bookings.emplace(bookingId, BookingInfo(slot, bookingId));

            availableSlots.erase(availableSlots.begin() + i);
            return true;
        }
    }
    return false;
}

bool Facility::cancelBooking(uint32_t bookingId) {
    auto it = bookings.find(bookingId);
    if (it != bookings.end()) {
        availableSlots.push_back(it->second.slot);

        bookings.erase(it);
        return true;
    }
    return false;  // Invalid booking ID
}

void Facility::displayAvailability(Util::Day day) const {
    std::cout << "[Server] Available slots for " << name << " on " << Util::dayToString(day)
              << ":\n";
    bool found = false;

    for (const auto& slot : availableSlots) {
        if (slot.day == day) {
            std::cout << "  - " << slot.toString() << "\n";
            found = true;
        }
    }

    if (!found) {
        std::cout << "[Server] No available slots for " << Util::dayToString(day) << ".\n";
    }
}

void Facility::displayAllSlots(Util::Day day) const {
    std::cout << "[Server] All slots for " << name << " on " << Util::dayToString(day) << ":\n";

    for (int hour = 8; hour < 18; ++hour) {
        int startTime = hour * 100;
        int endTime = startTime + 100;

        // Check if the current slot is available
        bool isAvailable = false;
        for (const auto& slot : availableSlots) {
            if (slot.day == day && slot.startTime <= startTime && slot.endTime > startTime) {
                isAvailable = true;
                break;
            }
        }

        // Display each slot and availability
        std::cout << "\t" << (startTime < 1000 ? "0" : "") << startTime << " - "
                  << (endTime < 1000 ? "0" : "") << endTime << " -> "
                  << (isAvailable ? "yes" : "no") << "\n";
    }
}

void Facility::sortAvailableSlots() {
    std::sort(availableSlots.begin(), availableSlots.end(),
              [](const TimeSlot& a, const TimeSlot& b) {
                  if (a.day != b.day) return a.day < b.day;
                  return a.startTime < b.startTime;
              });
}

uint32_t Facility::generateBookingId() {
    static uint32_t currentId =
        1000;  // should include client endpoint, but for simplity, just use hardcoded value here
    return currentId++;
}
