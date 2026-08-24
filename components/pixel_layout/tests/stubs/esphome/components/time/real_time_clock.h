#pragma once

#include <ctime>
#include <cstdio>
#include <cstring>
#include <string>

namespace esphome {
namespace time {

struct ESPTime {
  int hour{0};
  int minute{0};
  int second{0};
  int day_of_month{1};
  int month{1};
  int year{2026};
  int day_of_week{0};
  bool valid{false};

  bool is_valid() const { return this->valid; }

  void strftime(char *buf, size_t size, const char *fmt) const {
    std::tm t{};
    t.tm_hour = this->hour;
    t.tm_min = this->minute;
    t.tm_sec = this->second;
    t.tm_mday = this->day_of_month;
    t.tm_mon = this->month - 1;
    t.tm_year = this->year - 1900;
    t.tm_wday = this->day_of_week;
    std::strftime(buf, size, fmt, &t);
  }
};

class RealTimeClock {
 public:
  ESPTime now() const { return this->now_; }
  void set_now(const ESPTime &now) { this->now_ = now; }

 protected:
  ESPTime now_{};
};

}  // namespace time
}  // namespace esphome
