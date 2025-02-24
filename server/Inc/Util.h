#ifndef UTIL_H
#define UTIL_H

#include <string>

class Util {
  public:
    enum class Day { Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday };
    static Day stringToDay(const std::string &dayStr);
    static std::string dayToString(Day day);
    static std::pair<int, int> parseTime(uint16_t time);
};

#endif
