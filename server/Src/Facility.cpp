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

bool Facility::isAvailable(const TimeSlot& requested) const {
    uint16_t currentStart = requested.startTime;
    while (currentStart < requested.endTime) {
        bool found = false;
        for (const auto& slot : availableSlots) {
            if (slot.day == requested.day && slot.startTime == currentStart &&
                slot.endTime <= requested.endTime) {
                currentStart = slot.endTime;
                found = true;
                break;
            }
        }
        if (!found) return false;  // Could not find a matching segment
    }
    return true;
}

std::string Facility::getAvailability(Util::Day day) const {
    std::ostringstream oss;
    oss << "All slots for " << name << " on " << Util::dayToString(day) << ":\n";

    for (int mins = 480; mins < 1080; mins += 30) {  // 480 = 800, 1080 = 1800
        int startTime = Util::toHHMM(mins);
        int endTime = Util::toHHMM(mins + 30);

        TimeSlot halfHourSlot(day, startTime, endTime);
        bool available = isAvailable(halfHourSlot);

        oss << "\t" << (startTime < 1000 ? "0" : "") << startTime << " - "
            << (endTime < 1000 ? "0" : "") << endTime << " -> " << (available ? "yes" : "no")
            << "\n";
    }

    return oss.str();
}

bool Facility::bookSlot(const TimeSlot& requested, uint32_t& bookingId) {
    std::vector<size_t> matchedIndices;
    uint16_t currentStart = requested.startTime;

    // Find all matching slots that make up the requested time
    for (size_t i = 0; i < availableSlots.size(); ++i) {
        const auto& slot = availableSlots[i];
        if (slot.day == requested.day && slot.startTime == currentStart &&
            slot.endTime <= requested.endTime) {
            matchedIndices.push_back(i);
            currentStart = slot.endTime;

            if (currentStart == requested.endTime) break;  // Done
        }
    }

    if (currentStart != requested.endTime) {
        return false;  // Could not fulfill full requested duration
    }

    // All required slots found — proceed to book
    bookingId = generateBookingId();
    bookings.emplace(bookingId, requested);

    // Remove matched slots from availableSlots (in reverse to avoid shifting)
    for (auto it = matchedIndices.rbegin(); it != matchedIndices.rend(); ++it) {
        availableSlots.erase(availableSlots.begin() + *it);
    }

    return true;
}

bool Facility::modifyBooking(uint32_t bookingId, int offsetMinutes, std::string& errorMessage) {
    auto it = bookings.find(bookingId);
    if (it == bookings.end()) {
        errorMessage = "Invalid Booking ID.";
        return false;
    }

    BookingInfo& booking = it->second;
    TimeSlot oldSlot = booking.slot;

    int newStartMins = Util::toMinutes(oldSlot.startTime) + offsetMinutes;
    int newEndMins = Util::toMinutes(oldSlot.endTime) + offsetMinutes;

    int newStart = Util::toHHMM(newStartMins);
    int newEnd = Util::toHHMM(newEndMins);

    if (newStart < 800 || newEnd > 1800 || newStart >= newEnd) {
        std::cerr << "[Server] Exceed time range.\n";
        errorMessage = "Invalid time range after applying offset.";
        return false;
    }

    TimeSlot newSlot = oldSlot;
    newSlot.startTime = newStart;
    newSlot.endTime = newEnd;

    // Add back all sub-slots of the old booking
    auto originalParts = splitIntoThirtyMinSlots(oldSlot);
    for (const auto& part : originalParts) {
        availableSlots.push_back(part);
    }
    sortAvailableSlots();

    if (!isAvailable(newSlot)) {
        // Remove added parts again (rollback)
        for (const auto& part : originalParts) {
            availableSlots.erase(std::remove(availableSlots.begin(), availableSlots.end(), part),
                                 availableSlots.end());
        }
        sortAvailableSlots();
        std::cerr << "[Server] New slot is unavailable.\n";
        errorMessage = "Requested new time slot is unavailable.";
        return false;
    }

    // Remove new parts from availability
    auto newParts = splitIntoThirtyMinSlots(newSlot);
    for (const auto& part : newParts) {
        availableSlots.erase(std::remove(availableSlots.begin(), availableSlots.end(), part),
                             availableSlots.end());
    }
    sortAvailableSlots();

    booking.slot = newSlot;

    return true;
}

std::optional<Facility::TimeSlot> Facility::cancelBooking(uint32_t bookingId) {
    auto it = bookings.find(bookingId);
    if (it != bookings.end()) {
        TimeSlot fullSlot = it->second.slot;

        // Split and return all sub-slots to availability
        auto parts = splitIntoThirtyMinSlots(fullSlot);
        for (const auto& part : parts) {
            availableSlots.push_back(part);
        }
        sortAvailableSlots();

        bookings.erase(it);
        return fullSlot;
    }
    return std::nullopt;
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

std::vector<Facility::TimeSlot> Facility::splitIntoThirtyMinSlots(
    const Facility::TimeSlot& slot) const {
    std::vector<TimeSlot> result;
    int start = slot.startTime;
    int end = slot.endTime;

    while (start < end) {
        int next = Util::toHHMM(Util::toMinutes(start) + 30);
        result.emplace_back(slot.day, start, next);
        start = next;
    }
    return result;
}

bool Facility::extendBooking(uint32_t bookingId, int extensionMinutes, std::string& errorMessage) {
    auto it = bookings.find(bookingId);
    if (it == bookings.end()) {
        errorMessage = "Invalid Booking ID.";
        return false;
    }

    BookingInfo& booking = it->second;
    TimeSlot oldSlot = booking.slot;

    int oldEndMins = Util::toMinutes(oldSlot.endTime);
    int newEndMins = oldEndMins + extensionMinutes;
    int newEnd = Util::toHHMM(newEndMins);

    if (newEnd > 1800 || newEnd <= oldSlot.endTime) {
        std::cerr << "[Server] Invalid extension range.\n";
        errorMessage = "Extension goes beyond allowed time range or is non-positive.";
        return false;
    }

    TimeSlot extendedSlot = oldSlot;
    extendedSlot.endTime = newEnd;

    // Check availability for the extension period
    TimeSlot extensionOnlySlot(oldSlot.day, oldSlot.endTime, newEnd);
    if (!isAvailable(extensionOnlySlot)) {
        errorMessage = "Extension time slot is not available.";
        return false;
    }

    // Reserve the extended portion by removing it from availability
    auto extensionParts = splitIntoThirtyMinSlots(extensionOnlySlot);
    for (const auto& part : extensionParts) {
        availableSlots.erase(std::remove(availableSlots.begin(), availableSlots.end(), part),
                             availableSlots.end());
    }
    sortAvailableSlots();

    // Update the booking
    booking.slot = extendedSlot;

    return true;
}
