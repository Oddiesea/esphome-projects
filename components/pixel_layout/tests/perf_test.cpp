#include "pixel_layout.h"
#include "test_helpers.h"

#include "esphome/components/display/display.h"
#include "esphome/components/time/real_time_clock.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using esphome::Color;
using esphome::pixel_layout::ClockFace;
using esphome::pixel_layout::ClockTheme;
using esphome::pixel_layout::ClockWidget;
using esphome::pixel_layout::CustomWidget;
using esphome::pixel_layout::DrawContext;
using esphome::pixel_layout::test::nonzero_count;

namespace {

constexpr int kW = 128;
constexpr int kH = 64;
constexpr int kWarmup = 30;
constexpr int kFrames = 200;

// Debug host budget for one 128x64 composite frame (clear + AA fills + clocks + blit).
// Sized for GitHub ubuntu-latest Debug, not a tuned Mac. Override with PIXEL_LAYOUT_MAX_US_PER_FRAME.
constexpr double kDefaultMaxUsPerFrame = 2500.0;
// Full-buffer opaque blend throughput floor (pixels/s). Override with PIXEL_LAYOUT_MIN_BLEND_PPS.
constexpr double kDefaultMinBlendPixelsPerSec = 2.0e6;

double env_or(const char *name, double fallback) {
  const char *raw = std::getenv(name);
  if (raw == nullptr || raw[0] == '\0')
    return fallback;
  return std::strtod(raw, nullptr);
}

double ns_per(const std::chrono::steady_clock::duration &d) {
  return static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(d).count());
}

struct BusyScene {
  ClockWidget digital;
  ClockWidget analog;
  CustomWidget heart;
  esphome::time::RealTimeClock rtc;
  uint8_t packed[16]{};

  BusyScene() {
    esphome::time::ESPTime now;
    now.valid = true;
    now.hour = 12;
    now.minute = 34;
    now.second = 56;
    rtc.set_now(now);

    digital.set_face(ClockFace::DIGITAL);
    digital.set_theme(ClockTheme::SEVEN_SEGMENT);
    digital.set_blink_colon(false);
    digital.set_time(&rtc);
    digital.set_x(2);
    digital.set_y(2);
    digital.set_width(78);
    digital.set_height(28);
    digital.set_color(Color(0, 220, 255));

    analog.set_face(ClockFace::ANALOG);
    analog.set_time(&rtc);
    analog.set_x(86);
    analog.set_y(2);
    analog.set_width(40);
    analog.set_height(40);
    analog.set_color(Color(255, 255, 255));

    // 8x7 packed heart (nibble pairs)
    const uint8_t src[] = {0x01, 0x10, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
                           0x11, 0x11, 0x11, 0x10, 0x01, 0x11, 0x10, 0x00};
    std::memcpy(packed, src, sizeof(src));
    heart.set_color(Color(255, 40, 80));
    heart.set_pixels(packed, 8, 7);
    heart.set_x(4);
    heart.set_y(48);
  }
};

void paint_composite(DrawContext &ctx, BusyScene &scene, uint32_t now_ms) {
  ctx.clear(Color(8, 8, 16));
  ctx.fill_round_rect(0, 0, kW, 34, 4, true, Color(20, 24, 40), 180);
  ctx.fill_ellipse(100, 50, 24, 12, true, Color(40, 80, 40), 120);
  scene.digital.prepare(now_ms);
  scene.digital.layout(0, 0, kW, kH);
  scene.digital.draw(ctx, now_ms);
  scene.analog.prepare(now_ms);
  scene.analog.layout(0, 0, kW, kH);
  scene.analog.draw(ctx, now_ms);
  scene.heart.layout(0, 0, kW, kH);
  scene.heart.draw(ctx, now_ms);
}

}  // namespace

TEST(Perf, CompositeFrameBudget) {
  BusyScene scene;
  std::vector<uint8_t> buf(static_cast<size_t>(kW * kH * 3), 0);
  DrawContext ctx;
  ctx.init(buf.data(), kW, kH);
  esphome::display::Display blit(kW, kH);

  for (int i = 0; i < kWarmup; i++) {
    paint_composite(ctx, scene, static_cast<uint32_t>(i * 16));
    ctx.blit(blit);
  }
  ASSERT_GT(nonzero_count(buf.data(), kW, kH), 100);

  const auto t0 = std::chrono::steady_clock::now();
  for (int i = 0; i < kFrames; i++) {
    paint_composite(ctx, scene, static_cast<uint32_t>(1000 + i * 16));
    ctx.blit(blit);
  }
  const auto t1 = std::chrono::steady_clock::now();

  const double us_total = ns_per(t1 - t0) / 1000.0;
  const double us_frame = us_total / kFrames;
  const double fps = 1.0e6 / us_frame;
  const double max_us = env_or("PIXEL_LAYOUT_MAX_US_PER_FRAME", kDefaultMaxUsPerFrame);

  std::printf("pixel_layout composite %dx%d: %.1f us/frame (%.0f fps), gate %.0f us\n", kW, kH, us_frame, fps, max_us);
  RecordProperty("us_per_frame", std::to_string(static_cast<int>(us_frame + 0.5)));
  RecordProperty("fps", std::to_string(static_cast<int>(fps + 0.5)));

  EXPECT_LT(us_frame, max_us) << "128x64 composite frame exceeded host budget (set PIXEL_LAYOUT_MAX_US_PER_FRAME to override)";
}

TEST(Perf, BlendThroughputFloor) {
  std::vector<uint8_t> buf(static_cast<size_t>(kW * kH * 3), 0);
  DrawContext ctx;
  ctx.init(buf.data(), kW, kH);
  const Color ink(255, 80, 40);
  const int warmup = 10;
  const int iters = 80;

  for (int i = 0; i < warmup; i++)
    ctx.fill_rect(0, 0, kW, kH, ink, 180);

  const auto t0 = std::chrono::steady_clock::now();
  for (int i = 0; i < iters; i++)
    ctx.fill_rect(0, 0, kW, kH, ink, 180);
  const auto t1 = std::chrono::steady_clock::now();

  const double sec = ns_per(t1 - t0) / 1.0e9;
  const double pixels = static_cast<double>(iters) * kW * kH;
  const double pps = pixels / sec;
  const double min_pps = env_or("PIXEL_LAYOUT_MIN_BLEND_PPS", kDefaultMinBlendPixelsPerSec);

  std::printf("pixel_layout blend: %.2e px/s (%.1f ns/px), floor %.2e px/s\n", pps, 1.0e9 / pps, min_pps);
  RecordProperty("blend_px_per_sec", std::to_string(static_cast<long long>(pps + 0.5)));

  EXPECT_GT(pps, min_pps) << "blend_pixel throughput dropped below host floor (set PIXEL_LAYOUT_MIN_BLEND_PPS to override)";
}
