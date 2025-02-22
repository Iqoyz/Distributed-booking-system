#include "Facility.h"

Facility::Facility(const std::string& name) : name(name) {}

std::string Facility::getName() const { return name; }

void Facility::addAvailability(const TimeSlot& slot) { availability.push_back(slot); }

bool Facility::isAvailable(const TimeSlot& slot) const {
    for (const auto& s : availability) {
        if (s.day == slot.day && s.hour == slot.hour && s.minute == slot.minute) {
            return true;
        }
    }
    return false;
}

bool Facility::bookSlot(const TimeSlot& slot) {
    for (size_t i = 0; i < availability.size(); ++i) {
        if (availability[i].day == slot.day && availability[i].hour == slot.hour &&
            availability[i].minute == slot.minute) {
            availability.erase(availability.begin() + i);
            return true;
        }
    }
    return false;
}

void Facility::displayAvailability() const {
    std::cout << "Availability for " << name << ":\n";
    for (const auto& slot : availability) {
        std::cout << "  - " << slot.toString() << "\n";
    }
}