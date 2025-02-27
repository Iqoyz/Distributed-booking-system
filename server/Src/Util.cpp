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

std::pair<int, int> Util::parseTime(uint16_t time) {
    int hour = time / 100;    // First two digits represent hours
    int minute = time % 100;  // Last two digits represent minutes

    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        throw std::invalid_argument("Invalid time format: " + std::to_string(time));
    }
    return {hour, minute};
}

float Util::generateFpRandNumber() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(0.0, 1.0);  // between 0 and 1.0
    return dist(gen);
}
