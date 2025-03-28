#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <random>

class Util {
  public:
    enum class Day { Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday };
    static Day stringToDay(const std::string &dayStr);
    static std::string dayToString(Day day);
    static std::pair<int, int> parseTime(uint16_t time);
    static int toHHMM(int minutes);
    static int toMinutes(int hhmm);

    static float generateFpRandNumber();
};

#endif
