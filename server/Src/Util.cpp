#include "Util.h"
#include <unordered_map>
#include <stdexcept>

std::string Util::dayToString(Util::Day day) {
    switch (day) {
        case Day::Monday:
            return "Monday";
        case Day::Tuesday:
            return "Tuesday";
        case Day::Wednesday:
            return "Wednesday";
        case Day::Thursday:
            return "Thursday";
        case Day::Friday:
            return "Friday";
        case Day::Saturday:
            return "Saturday";
        case Day::Sunday:
            return "Sunday";
        default:
            return "Unknown";
    }
}

Util::Day Util::stringToDay(const std::string& dayStr) {
    static const std::unordered_map<std::string, Day> dayMap = {
        {"Monday", Day::Monday},     {"Tuesday", Day::Tuesday}, {"Wednesday", Day::Wednesday},
        {"Thursday", Day::Thursday}, {"Friday", Day::Friday},   {"Saturday", Day::Saturday},
        {"Sunday", Day::Sunday}};

    auto it = dayMap.find(dayStr);
    if (it != dayMap.end()) {
        return it->second;
    }
    throw std::invalid_argument("Invalid day: " + dayStr);
}
