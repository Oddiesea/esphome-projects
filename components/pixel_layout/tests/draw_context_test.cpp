#include "pixel_layout.h"
#include "test_helpers.h"

#include "esphome/components/display/display.h"
#include "esphome/components/font/font.h"
#include "esphome/components/image/image.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

using esphome::Color;
using esphome::pixel_layout::BoxPoint;
using esphome::pixel_layout::BoxShape;
using esphome::pixel_layout::BoxWidget;
using esphome::pixel_layout::DrawContext;
using esphome::pixel_layout::test::make_block_font;
using esphome::pixel_layout::test::nonzero_count;
using esphome::pixel_layout::test::rgb_at;

namespace {

struct Canvas {
  static constexpr int kW = 16;
  static constexpr int kH = 16;
  std::vector<uint8_t> buf;
  DrawContext ctx;

  Canvas() : buf(static_cast<size_t>(kW * kH * 3), 0) { ctx.init(buf.data(), kW, kH); }

  Color at(int x, int y) const { return rgb_at(buf.data(), kW, x, y); }
  int lit() const { return nonzero_count(buf.data(), kW, kH); }
};

}  // namespace

TEST(DrawContext, ClearFillsRgb) {
  Canvas c;
  c.ctx.clear(Color(10, 20, 30));
  EXPECT_EQ(c.at(0, 0).r, 10);
  EXPECT_EQ(c.at(0, 0).g, 20);
  EXPECT_EQ(c.at(0, 0).b, 30);
  EXPECT_EQ(c.at(15, 15).r, 10);
}

TEST(DrawContext, ClearNullBufferIsNoop) {
  DrawContext ctx;
  ctx.init(nullptr, 8, 8);
  ctx.clear(Color(255, 0, 0));
  ctx.blend_pixel(0, 0, Color(0, 255, 0), 255);
}

TEST(DrawContext, OpaqueBlendOverwrites) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.blend_pixel(3, 4, Color(255, 128, 64), 255);
  EXPECT_EQ(c.at(3, 4).r, 255);
  EXPECT_EQ(c.at(3, 4).g, 128);
  EXPECT_EQ(c.at(3, 4).b, 64);
  EXPECT_EQ(c.at(3, 5).r, 0);
}

TEST(DrawContext, ZeroAlphaIsNoop) {
  Canvas c;
  c.ctx.clear(Color(1, 2, 3));
  c.ctx.blend_pixel(0, 0, Color(255, 0, 0), 0);
  EXPECT_EQ(c.at(0, 0).r, 1);
}

TEST(DrawContext, OutOfBoundsClips) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.blend_pixel(-1, 0, Color(255, 0, 0), 255);
  c.ctx.blend_pixel(0, -1, Color(255, 0, 0), 255);
  c.ctx.blend_pixel(16, 0, Color(255, 0, 0), 255);
  c.ctx.blend_pixel(0, 16, Color(255, 0, 0), 255);
  EXPECT_EQ(c.lit(), 0);
}

TEST(DrawContext, HalfAlphaBlend) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.blend_pixel(0, 0, Color(255, 0, 0), 128);
  EXPECT_EQ(c.at(0, 0).r, 128);
  EXPECT_EQ(c.at(0, 0).g, 0);
}

TEST(DrawContext, ColorWScalesSourceAlpha) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.blend_pixel(0, 0, Color(255, 0, 0, 128), 255);
  EXPECT_EQ(c.at(0, 0).r, 128);
}

TEST(DrawContext, AlphaScale) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.set_alpha_scale(128);
  c.ctx.blend_pixel(0, 0, Color(255, 0, 0), 255);
  EXPECT_EQ(c.at(0, 0).r, 128);
}

TEST(DrawContext, OriginShiftsWrites) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.set_origin(2, 3);
  c.ctx.blend_pixel(1, 1, Color(255, 0, 0), 255);
  EXPECT_EQ(c.at(1, 1).r, 0);
  EXPECT_EQ(c.at(3, 4).r, 255);
}

TEST(DrawContext, FillRectClipsAndBlends) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.fill_rect(-4, -4, 8, 8, Color(255, 0, 0), 255);
  EXPECT_EQ(c.at(0, 0).r, 255);
  EXPECT_EQ(c.at(3, 3).r, 255);
  EXPECT_EQ(c.at(4, 4).r, 0);
  c.ctx.set_origin(10, 10);
  c.ctx.fill_rect(0, 0, 2, 2, Color(0, 255, 0), 255);
  EXPECT_EQ(c.at(10, 10).g, 255);
  EXPECT_EQ(c.at(11, 11).g, 255);
  c.ctx.set_origin(0, 0);
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.fill_rect(0, 0, 2, 1, Color(255, 0, 0), 128);
  EXPECT_EQ(c.at(0, 0).r, 128);
  EXPECT_EQ(c.at(1, 0).r, 128);
}

TEST(DrawContext, FillRectAndLines) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.fill_rect(2, 2, 4, 3, Color(0, 255, 0), 255);
  EXPECT_EQ(c.at(2, 2).g, 255);
  EXPECT_EQ(c.at(5, 4).g, 255);
  EXPECT_EQ(c.at(6, 2).g, 0);
  EXPECT_EQ(c.lit(), 12);

  c.ctx.clear(Color(0, 0, 0));
  c.ctx.hline(0, 5, 8, Color(0, 0, 255), 255);
  EXPECT_EQ(c.lit(), 8);
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.vline(5, 0, 6, Color(0, 0, 255), 255);
  EXPECT_EQ(c.lit(), 6);
}

TEST(DrawContext, LineHitsEndpoints) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.line(0, 0, 7, 7, Color(255, 255, 255), 255);
  EXPECT_EQ(c.at(0, 0).r, 255);
  EXPECT_EQ(c.at(7, 7).r, 255);
  EXPECT_GT(c.lit(), 2);
}

TEST(DrawContext, ClipRectMasksFill) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.set_clip(2, 0, 4, 16);
  c.ctx.fill_rect(0, 0, 16, 16, Color(255, 0, 0), 255);
  EXPECT_EQ(c.at(1, 0).r, 0);
  EXPECT_EQ(c.at(2, 0).r, 255);
  EXPECT_EQ(c.at(5, 0).r, 255);
  EXPECT_EQ(c.at(6, 0).r, 0);
  c.ctx.reset_mask();
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.set_clip(0, 0, 0, 16);
  c.ctx.fill_rect(0, 0, 16, 16, Color(255, 0, 0), 255);
  EXPECT_EQ(c.lit(), 0);
}

TEST(DrawContext, DissolveLimitHidesThenShows) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.set_dissolve(0);
  c.ctx.fill_rect(0, 0, 16, 16, Color(0, 255, 0), 255);
  EXPECT_EQ(c.lit(), 0);
  c.ctx.set_dissolve(256);
  c.ctx.fill_rect(0, 0, 16, 16, Color(0, 255, 0), 255);
  EXPECT_EQ(c.lit(), 16 * 16);
}

TEST(DrawContext, BlindsOpenByRow) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.set_blinds(2, 8, false);
  c.ctx.fill_rect(0, 0, 16, 16, Color(0, 0, 255), 255);
  EXPECT_EQ(c.at(0, 0).b, 255);
  EXPECT_EQ(c.at(0, 1).b, 255);
  EXPECT_EQ(c.at(0, 2).b, 0);
  EXPECT_EQ(c.at(0, 8).b, 255);
}

TEST(DrawContext, CircleOutlineSymmetric) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.circle_outline(8, 8, 4, Color(255, 255, 255), 255);
  EXPECT_EQ(c.at(12, 8).r, c.at(4, 8).r);
  EXPECT_EQ(c.at(8, 12).r, c.at(8, 4).r);
  EXPECT_EQ(c.at(12, 8).r, 255);
  EXPECT_EQ(c.at(8, 8).r, 0);
}

TEST(DrawContext, RoundRectInteriorAndCorner) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.fill_round_rect(0, 0, 16, 16, 6, false, Color(255, 0, 0), 255);
  EXPECT_EQ(c.at(8, 8).r, 255);
  EXPECT_EQ(c.at(0, 0).r, 0);
}

TEST(DrawContext, RoundRectAntialiasPartialCoverage) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.fill_round_rect(0, 0, 16, 16, 8, true, Color(255, 0, 0), 255);
  EXPECT_EQ(c.at(8, 8).r, 255);
  bool partial = false;
  for (int y = 0; y < 16 && !partial; y++) {
    for (int x = 0; x < 16; x++) {
      const uint8_t r = c.at(x, y).r;
      if (r > 0 && r < 255)
        partial = true;
    }
  }
  EXPECT_TRUE(partial);
}

TEST(DrawContext, EllipseInteriorAndCorner) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.fill_ellipse(0, 0, 16, 16, false, Color(0, 255, 0), 255);
  EXPECT_EQ(c.at(8, 8).g, 255);
  EXPECT_EQ(c.at(0, 0).g, 0);
}

TEST(DrawContext, EllipseAntialiasPartialCoverage) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.fill_ellipse(0, 0, 16, 16, true, Color(0, 0, 255), 255);
  EXPECT_EQ(c.at(8, 8).b, 255);
  bool partial = false;
  for (int y = 0; y < 16 && !partial; y++) {
    for (int x = 0; x < 16; x++) {
      const uint8_t b = c.at(x, y).b;
      if (b > 0 && b < 255)
        partial = true;
    }
  }
  EXPECT_TRUE(partial);
}

TEST(DrawContext, DrawTextBlockGlyph) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  auto font = make_block_font();
  c.ctx.draw_text(0, 0, nullptr, Color(255, 255, 255), 255, "A");
  EXPECT_EQ(c.lit(), 0);
  c.ctx.draw_text(0, 0, &font, Color(255, 255, 255), 255, nullptr);
  EXPECT_EQ(c.lit(), 0);
  c.ctx.draw_text(1, 2, &font, Color(255, 0, 0), 255, "A");
  EXPECT_EQ(c.at(1, 2).r, 255);
  EXPECT_EQ(c.at(8, 9).r, 255);
  EXPECT_EQ(c.lit(), 64);
  int w = 0, h = 0;
  c.ctx.measure_text(&font, "A", &w, &h);
  EXPECT_EQ(w, 8);
  EXPECT_EQ(h, 8);
  c.ctx.measure_text(nullptr, "A", &w, &h);
  EXPECT_EQ(w, 0);
}

TEST(DrawContext, DrawImageRegion) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  esphome::image::Image img(2, 2);
  img.set_pixel(0, 0, Color(255, 0, 0));
  img.set_pixel(1, 0, Color(0, 255, 0));
  img.set_pixel(0, 1, Color(0, 0, 255));
  c.ctx.draw_image_region(&img, 4, 5, 0, 0, 2, 2, 255);
  EXPECT_EQ(c.at(4, 5).r, 255);
  EXPECT_EQ(c.at(5, 5).g, 255);
  EXPECT_EQ(c.at(4, 6).b, 255);
  c.ctx.draw_image_region(nullptr, 0, 0, 0, 0, 2, 2, 255);
}

TEST(DrawContext, BlitRgb888) {
  Canvas c;
  c.ctx.clear(Color(9, 8, 7));
  esphome::display::Display it(16, 16);
  c.ctx.blit(it);
  ASSERT_EQ(it.captured().size(), 16u * 16u * 3u);
  EXPECT_EQ(it.last_order(), esphome::display::COLOR_ORDER_RGB);
  EXPECT_EQ(it.last_bitness(), esphome::display::COLOR_BITNESS_888);
  EXPECT_FALSE(it.last_big_endian());
  EXPECT_EQ(it.captured()[0], 9);
  EXPECT_EQ(it.captured()[1], 8);
  EXPECT_EQ(it.captured()[2], 7);
}

TEST(DrawContext, FillTriangleCoversCentroidNotCorner) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.fill_triangle(0, 15, 15, 15, 7, 0, false, Color(255, 0, 0), 255);
  EXPECT_EQ(c.at(7, 10).r, 255);
  EXPECT_EQ(c.at(0, 0).r, 0);
  EXPECT_EQ(c.at(15, 0).r, 0);
}

TEST(DrawContext, StrokeEllipseHollowCenter) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.stroke_ellipse(0, 0, 16, 16, 2, false, Color(0, 255, 0), 255);
  EXPECT_EQ(c.at(8, 8).g, 0);
  EXPECT_EQ(c.at(8, 0).g, 255);
  EXPECT_EQ(c.at(0, 8).g, 255);
}

TEST(DrawContext, StrokeLineDiagonal) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  c.ctx.stroke_line(0, 0, 15, 15, 1, Color(0, 0, 255), 255);
  EXPECT_EQ(c.at(0, 0).b, 255);
  EXPECT_EQ(c.at(15, 15).b, 255);
  EXPECT_EQ(c.at(15, 0).b, 0);
}

TEST(BoxWidget, FrameHollowInterior) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  BoxWidget box;
  box.set_fill(Color(255, 255, 255));
  box.set_shape(BoxShape::FRAME);
  box.set_stroke(2);
  box.set_width(16);
  box.set_height(16);
  box.layout(0, 0, 16, 16);
  box.draw(c.ctx, 0);
  EXPECT_EQ(c.at(0, 0).r, 255);
  EXPECT_EQ(c.at(15, 15).r, 255);
  EXPECT_EQ(c.at(8, 8).r, 0);
}

TEST(BoxWidget, DiamondCenterLit) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  BoxWidget box;
  box.set_fill(Color(255, 255, 255));
  box.set_shape(BoxShape::DIAMOND);
  box.set_width(16);
  box.set_height(16);
  box.layout(0, 0, 16, 16);
  box.draw(c.ctx, 0);
  EXPECT_EQ(c.at(8, 8).r, 255);
  EXPECT_EQ(c.at(0, 0).r, 0);
}

TEST(BoxWidget, TrianglePointRight) {
  Canvas c;
  c.ctx.clear(Color(0, 0, 0));
  BoxWidget box;
  box.set_fill(Color(255, 255, 255));
  box.set_shape(BoxShape::TRIANGLE);
  box.set_point(BoxPoint::RIGHT);
  box.set_width(16);
  box.set_height(16);
  box.layout(0, 0, 16, 16);
  box.draw(c.ctx, 0);
  EXPECT_EQ(c.at(0, 8).r, 255);
  EXPECT_EQ(c.at(14, 8).r, 255);
  EXPECT_EQ(c.at(15, 0).r, 0);
}
