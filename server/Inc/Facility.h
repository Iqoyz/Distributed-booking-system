#ifndef FACILITY_H
#define FACILITY_H

#include <iostream>
#include <string>
#include <vector>
#include "Util.h"
class Facility {
  public:
    struct TimeSlot {
        Util::Day day;
        int hour;
        int minute;

        TimeSlot(Util::Day d, int h, int m) : day(d), hour(h), minute(m) {}

        std::string toString() const {
            return Util::dayToString(day) + " " + std::to_string(hour) + ":" +
                   (minute < 10 ? "0" : "") + std::to_string(minute);
        }
    };
    Facility(const std::string &name);
    std::string getName() const;

    void addAvailability(const TimeSlot &slot);
    bool isAvailable(const TimeSlot &slot) const;
    bool bookSlot(const TimeSlot &slot);
    void displayAvailability() const;

  private:
    std::string name;
    std::vector<TimeSlot> availability;
};

#endif
