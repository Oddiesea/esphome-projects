#include "pixel_layout.h"
#include "test_helpers.h"

#include "esphome/components/time/real_time_clock.h"
#include "esphome/core/hal.h"

#include <gtest/gtest.h>

using esphome::Color;
using esphome::pixel_layout::CustomWidget;
using esphome::pixel_layout::PixelLayout;
using esphome::pixel_layout::test::rgb_at;
using esphome::time::ESPTime;
using esphome::time::RealTimeClock;

namespace {

void set_clock(RealTimeClock *rtc, int hour, int minute, bool valid = true) {
  ESPTime t;
  t.valid = valid;
  t.hour = hour;
  t.minute = minute;
  rtc->set_now(t);
}

void configure_night(PixelLayout *layout, RealTimeClock *rtc, int off_h = 23, int off_m = 0, int on_h = 7,
                     int on_m = 0) {
  layout->set_night_schedule_configured(true);
  layout->set_night_schedule_time(rtc);
  layout->set_night_schedule_enabled(true);
  layout->set_night_off_hour(static_cast<uint8_t>(off_h));
  layout->set_night_off_minute(static_cast<uint8_t>(off_m));
  layout->set_night_on_hour(static_cast<uint8_t>(on_h));
  layout->set_night_on_minute(static_cast<uint8_t>(on_m));
}

}  // namespace

TEST(NightSchedule, InWindowBlanks) {
  RealTimeClock rtc;
  PixelLayout layout;
  configure_night(&layout, &rtc);
  set_clock(&rtc, 0, 30);
  layout.evaluate_night_schedule();
  EXPECT_TRUE(layout.is_blanked());
}

TEST(NightSchedule, OutsideWindowShows) {
  RealTimeClock rtc;
  PixelLayout layout;
  configure_night(&layout, &rtc);
  set_clock(&rtc, 12, 0);
  layout.evaluate_night_schedule();
  EXPECT_FALSE(layout.is_blanked());
}

TEST(NightSchedule, DisabledNeverBlanksFromSchedule) {
  RealTimeClock rtc;
  PixelLayout layout;
  configure_night(&layout, &rtc);
  layout.set_night_schedule_enabled(false);
  set_clock(&rtc, 1, 0);
  layout.evaluate_night_schedule();
  EXPECT_FALSE(layout.is_blanked());
}

TEST(NightSchedule, InvalidTimeFailsOpen) {
  RealTimeClock rtc;
  PixelLayout layout;
  configure_night(&layout, &rtc);
  set_clock(&rtc, 1, 0, false);
  layout.evaluate_night_schedule();
  EXPECT_FALSE(layout.is_blanked());
}

TEST(NightSchedule, CrossMidnightWindow) {
  RealTimeClock rtc;
  PixelLayout layout;
  configure_night(&layout, &rtc, 22, 30, 6, 15);

  set_clock(&rtc, 22, 29);
  layout.evaluate_night_schedule();
  EXPECT_FALSE(layout.is_blanked());

  set_clock(&rtc, 22, 30);
  layout.evaluate_night_schedule();
  EXPECT_TRUE(layout.is_blanked());

  set_clock(&rtc, 23, 59);
  layout.evaluate_night_schedule();
  EXPECT_TRUE(layout.is_blanked());

  set_clock(&rtc, 0, 0);
  layout.evaluate_night_schedule();
  EXPECT_TRUE(layout.is_blanked());

  set_clock(&rtc, 6, 14);
  layout.evaluate_night_schedule();
  EXPECT_TRUE(layout.is_blanked());

  set_clock(&rtc, 6, 15);
  layout.evaluate_night_schedule();
  EXPECT_FALSE(layout.is_blanked());
}

TEST(NightSchedule, PowerOnOverrideUntilNextOff) {
  RealTimeClock rtc;
  PixelLayout layout;
  configure_night(&layout, &rtc);

  set_clock(&rtc, 1, 0);
  layout.evaluate_night_schedule();
  ASSERT_TRUE(layout.is_blanked());

  layout.on_power_on();
  EXPECT_FALSE(layout.is_blanked());

  set_clock(&rtc, 2, 0);
  layout.evaluate_night_schedule();
  EXPECT_FALSE(layout.is_blanked());

  // Leave night window then re-enter: override clears on off edge.
  set_clock(&rtc, 8, 0);
  layout.evaluate_night_schedule();
  EXPECT_FALSE(layout.is_blanked());

  set_clock(&rtc, 23, 0);
  layout.evaluate_night_schedule();
  EXPECT_TRUE(layout.is_blanked());
}

TEST(NightSchedule, SleepUntilWake) {
  RealTimeClock rtc;
  PixelLayout layout;
  configure_night(&layout, &rtc);

  // Daytime sleep blanks until next wake (exit of next night window).
  set_clock(&rtc, 10, 0);
  layout.evaluate_night_schedule();
  ASSERT_FALSE(layout.is_blanked());
  layout.sleep_until_wake();
  EXPECT_TRUE(layout.is_blanked());

  set_clock(&rtc, 23, 0);
  layout.evaluate_night_schedule();
  EXPECT_TRUE(layout.is_blanked());

  set_clock(&rtc, 7, 0);
  layout.evaluate_night_schedule();
  EXPECT_FALSE(layout.is_blanked());
}

TEST(NightSchedule, PowerOnClearsSleep) {
  RealTimeClock rtc;
  PixelLayout layout;
  configure_night(&layout, &rtc);

  set_clock(&rtc, 15, 0);
  layout.sleep_until_wake();
  ASSERT_TRUE(layout.is_blanked());
  layout.on_power_on();
  EXPECT_FALSE(layout.is_blanked());
}

TEST(NightSchedule, BlankPaintSkipsWidgets) {
  RealTimeClock rtc;
  PixelLayout layout;
  layout.set_background(Color(0, 0, 40));
  configure_night(&layout, &rtc);

  const uint8_t packed[] = {0x10};
  CustomWidget pixel;
  pixel.set_color(Color(255, 0, 0));
  pixel.set_pixels(packed, 1, 1);
  pixel.set_x(0);
  pixel.set_y(0);
  pixel.set_width(1);
  pixel.set_height(1);
  layout.set_root(&pixel);
  layout.host_init(4, 4);

  set_clock(&rtc, 1, 0);
  esphome::set_millis(1000);
  layout.host_paint();
  ASSERT_TRUE(layout.is_blanked());
  const auto px = rgb_at(layout.host_buffer(), 4, 0, 0);
  EXPECT_EQ(px.r, 0);
  EXPECT_EQ(px.g, 0);
  EXPECT_EQ(px.b, 40);

  layout.host_shutdown();
}

TEST(NightSchedule, NotConfiguredIgnoresSleep) {
  PixelLayout layout;
  layout.sleep_until_wake();
  layout.evaluate_night_schedule();
  EXPECT_FALSE(layout.is_blanked());
}
