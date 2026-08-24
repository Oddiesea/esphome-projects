#include "pixel_layout.h"
#include "test_helpers.h"

#include "esphome/components/display/display.h"
#include "esphome/core/hal.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

using esphome::Color;
using esphome::pixel_layout::CustomWidget;
using esphome::pixel_layout::PixelLayout;
using esphome::pixel_layout::test::rgb_at;

TEST(PixelLayout, SetupWithoutDisplayFails) {
  PixelLayout layout;
  layout.setup();
  EXPECT_TRUE(layout.is_failed());
}

TEST(PixelLayout, HostPaintWithoutDisplay) {
  PixelLayout layout;
  layout.set_background(Color(0, 16, 32));
  layout.host_init(4, 4);
  ASSERT_NE(layout.host_buffer(), nullptr);
  EXPECT_EQ(layout.host_width(), 4);
  EXPECT_EQ(layout.host_height(), 4);
  esphome::set_millis(500);
  layout.host_paint();
  const auto bg = rgb_at(layout.host_buffer(), 4, 0, 0);
  EXPECT_EQ(bg.r, 0);
  EXPECT_EQ(bg.g, 16);
  EXPECT_EQ(bg.b, 32);
  layout.host_shutdown();
}

TEST(PixelLayout, ShowScreenPinAndEnable) {
  PixelLayout layout;
  CustomWidget a;
  CustomWidget b;
  CustomWidget c;
  layout.add_screen(&a, 1000, esphome::pixel_layout::ScreenTransition::CUT, 0, "alpha");
  layout.add_screen(&b, 2000, esphome::pixel_layout::ScreenTransition::CUT, 0, "bravo");
  layout.add_screen(&c, 3000, esphome::pixel_layout::ScreenTransition::CUT, 0, "charlie");
  layout.host_init(8, 8);

  EXPECT_EQ(layout.screen_count(), 3u);
  EXPECT_EQ(layout.current_screen_id(), "alpha");
  EXPECT_TRUE(layout.show_screen("bravo"));
  EXPECT_EQ(layout.current_screen_index(), 1u);
  EXPECT_EQ(layout.current_screen_id(), "bravo");

  layout.set_pinned(true);
  esphome::set_millis(10000);
  layout.host_advance(10000);
  EXPECT_EQ(layout.current_screen_index(), 1u);

  layout.set_pinned(false);
  layout.set_screen_enabled(2, false);
  EXPECT_TRUE(layout.show_next_enabled());
  EXPECT_EQ(layout.current_screen_id(), "alpha");
  layout.set_screen_enabled(0, false);  // disabling current auto-advances
  EXPECT_EQ(layout.current_screen_id(), "bravo");
  EXPECT_FALSE(layout.show_next_enabled());  // only bravo left enabled

  layout.host_shutdown();
}

TEST(PixelLayout, RotateAndTransitionOverride) {
  using esphome::pixel_layout::ScreenTransition;
  PixelLayout layout;
  CustomWidget a;
  CustomWidget b;
  layout.set_rotate_ms(5000);
  layout.set_transition(ScreenTransition::FADE);
  layout.set_transition_ms(400);
  layout.add_screen(&a, 1000, ScreenTransition::CUT, 0, "a");
  layout.add_screen(&b, 1000, ScreenTransition::CUT, 0, "b");
  layout.host_init(8, 8);

  // Override off: per-screen dwell 1000ms
  esphome::set_millis(0);
  layout.show_screen(0);
  esphome::set_millis(999);
  layout.host_advance(999);
  EXPECT_EQ(layout.current_screen_index(), 0u);
  esphome::set_millis(1000);
  layout.host_advance(1000);
  EXPECT_EQ(layout.current_screen_index(), 1u);

  layout.show_screen(0);
  layout.set_rotate_override(true);
  layout.set_rotate_ms(3000);
  esphome::set_millis(2000);
  layout.host_advance(2000);
  EXPECT_EQ(layout.current_screen_index(), 0u);
  esphome::set_millis(5000);
  layout.host_advance(5000);
  EXPECT_EQ(layout.current_screen_index(), 1u);

  layout.set_rotate_override(false);
  layout.set_transition_override(true);
  layout.set_transition(ScreenTransition::SLIDE_LEFT);
  layout.set_transition_ms(0);  // cut-like via zero duration path still uses type
  EXPECT_TRUE(layout.get_transition_override());
  EXPECT_EQ(layout.get_transition(), ScreenTransition::SLIDE_LEFT);

  layout.host_shutdown();
}

TEST(PixelLayout, SetupBlitsBackgroundAndWidget) {
  esphome::display::Display display(8, 8);
  PixelLayout layout;
  layout.set_display(&display);
  layout.set_background(Color(0, 0, 32));

  const uint8_t packed[] = {0x10};
  CustomWidget pixel;
  pixel.set_color(Color(255, 0, 0));
  pixel.set_pixels(packed, 1, 1);
  pixel.set_x(2);
  pixel.set_y(3);
  pixel.set_width(1);
  pixel.set_height(1);
  layout.set_root(&pixel);

  esphome::set_millis(1000);
  layout.setup();
  EXPECT_FALSE(layout.is_failed());
  EXPECT_TRUE(display.has_writer());

  display.invoke_writer();
  ASSERT_EQ(display.captured().size(), 8u * 8u * 3u);
  EXPECT_EQ(display.last_order(), esphome::display::COLOR_ORDER_RGB);
  EXPECT_EQ(display.last_bitness(), esphome::display::COLOR_BITNESS_888);

  const auto bg = rgb_at(display.captured().data(), 8, 0, 0);
  EXPECT_EQ(bg.r, 0);
  EXPECT_EQ(bg.g, 0);
  EXPECT_EQ(bg.b, 32);

  const auto px = rgb_at(display.captured().data(), 8, 2, 3);
  EXPECT_EQ(px.r, 255);
  EXPECT_EQ(px.g, 0);
  EXPECT_EQ(px.b, 0);
}
