#include "hub75_dma.h"

#include "esphome/core/log.h"

namespace esphome {
namespace hub75_dma {

static const char *const TAG = "hub75_dma";

void Hub75DmaDisplay::set_pins(InternalGPIOPin *r1, InternalGPIOPin *g1, InternalGPIOPin *b1, InternalGPIOPin *r2,
                               InternalGPIOPin *g2, InternalGPIOPin *b2, InternalGPIOPin *a, InternalGPIOPin *b,
                               InternalGPIOPin *c, InternalGPIOPin *d, InternalGPIOPin *e, InternalGPIOPin *lat,
                               InternalGPIOPin *oe, InternalGPIOPin *clk) {
  this->mxconfig_.gpio = {
      static_cast<int8_t>(r1->get_pin()),
      static_cast<int8_t>(g1->get_pin()),
      static_cast<int8_t>(b1->get_pin()),
      static_cast<int8_t>(r2->get_pin()),
      static_cast<int8_t>(g2->get_pin()),
      static_cast<int8_t>(b2->get_pin()),
      static_cast<int8_t>(a->get_pin()),
      static_cast<int8_t>(b->get_pin()),
      static_cast<int8_t>(c->get_pin()),
      static_cast<int8_t>(d->get_pin()),
      static_cast<int8_t>(e != nullptr ? e->get_pin() : -1),
      static_cast<int8_t>(lat->get_pin()),
      static_cast<int8_t>(oe->get_pin()),
      static_cast<int8_t>(clk->get_pin()),
  };
}

void Hub75DmaDisplay::setup() {
  const uint32_t interval_ms = this->get_update_interval();
  if (interval_ms > 0 && interval_ms <= 1000) {
    this->mxconfig_.min_refresh_rate = static_cast<uint8_t>(1000 / interval_ms);
  }

  this->dma_display_ = new MatrixPanel_I2S_DMA(this->mxconfig_);  // NOLINT(cppcoreguidelines-owning-memory)
  if (this->dma_display_ == nullptr || !this->dma_display_->begin()) {
    ESP_LOGE(TAG, "HUB75 DMA buffer allocation failed");
    this->mark_failed();
    return;
  }

  this->set_brightness(this->initial_brightness_);
  this->dma_display_->clearScreen();

  if (this->use_virtual_()) {
    this->virtual_ = new VirtualMatrixPanel(*this->dma_display_, this->chain_rows_, this->chain_cols_,
                                            this->mxconfig_.mx_width, this->mxconfig_.mx_height, this->chain_type_);
    if (this->virtual_ == nullptr) {
      ESP_LOGE(TAG, "Virtual matrix allocation failed");
      this->mark_failed();
      return;
    }
    if (this->rotation_ != 0)
      this->virtual_->setRotation(this->rotation_);
  }
}

void Hub75DmaDisplay::update() {
  if (this->is_failed() || this->dma_display_ == nullptr)
    return;

  if (this->enabled_) {
    this->do_update_();
  } else {
    this->dma_display_->clearScreen();
  }
  this->dma_display_->flipDMABuffer();
}

void Hub75DmaDisplay::dump_config() {
  LOG_DISPLAY("", "HUB75 DMA", this);
  if (this->is_failed()) {
    ESP_LOGCONFIG(TAG, "  Failed to allocate DMA buffer");
    return;
  }

  const HUB75_I2S_CFG cfg = this->dma_display_ != nullptr ? this->dma_display_->getCfg() : this->mxconfig_;
  ESP_LOGCONFIG(TAG, "  Module: %ux%u  grid: %ux%u  dma chain: %u", cfg.mx_width, cfg.mx_height, this->chain_cols_,
                this->chain_rows_, cfg.chain_length);
  ESP_LOGCONFIG(TAG, "  Canvas: %dx%d  rotation: %u", this->get_width(), this->get_height(), this->rotation_ * 90);
  ESP_LOGCONFIG(TAG, "  Pins: R1:%d G1:%d B1:%d R2:%d G2:%d B2:%d", cfg.gpio.r1, cfg.gpio.g1, cfg.gpio.b1, cfg.gpio.r2,
                cfg.gpio.g2, cfg.gpio.b2);
  ESP_LOGCONFIG(TAG, "  Pins: A:%d B:%d C:%d D:%d E:%d LAT:%d OE:%d CLK:%d", cfg.gpio.a, cfg.gpio.b, cfg.gpio.c,
                cfg.gpio.d, cfg.gpio.e, cfg.gpio.lat, cfg.gpio.oe, cfg.gpio.clk);
  ESP_LOGCONFIG(TAG, "  Clock phase: %s", YESNO(cfg.clkphase));
  ESP_LOGCONFIG(TAG, "  Double buffer: %s", YESNO(cfg.double_buff));
  ESP_LOGCONFIG(TAG, "  Brightness: %u", this->initial_brightness_);
}

void Hub75DmaDisplay::set_brightness(uint8_t brightness) {
  if (this->dma_display_ == nullptr)
    return;
  this->dma_display_->setBrightness8(brightness);
  for (auto *entity : this->brightness_values_) {
    entity->publish_state(brightness);
  }
}

void Hub75DmaDisplay::register_brightness(Hub75DmaBrightness *brightness) {
  this->brightness_values_.push_back(brightness);
}

void Hub75DmaDisplay::register_power_switch(Hub75DmaPowerSwitch *power_switch) {
  this->power_switches_.push_back(power_switch);
}

void HOT Hub75DmaDisplay::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (this->dma_display_ == nullptr)
    return;
  if (x < 0 || y < 0 || x >= this->get_width_internal() || y >= this->get_height_internal())
    return;
  this->draw_rgb_(x, y, color.r, color.g, color.b);
}

void Hub75DmaDisplay::draw_rgb_(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
  if (this->virtual_ != nullptr)
    this->virtual_->drawPixelRGB888(x, y, r, g, b);
  else
    this->dma_display_->drawPixelRGB888(x, y, r, g, b);
}

bool Hub75DmaDisplay::use_virtual_() const {
  return this->chain_rows_ > 1 || this->chain_type_ != CHAIN_NONE || this->rotation_ != 0;
}

int Hub75DmaDisplay::get_width_internal() {
  const int w = this->mxconfig_.mx_width * this->chain_cols_;
  const int h = this->mxconfig_.mx_height * this->chain_rows_;
  return (this->rotation_ == 1 || this->rotation_ == 3) ? h : w;
}

int Hub75DmaDisplay::get_height_internal() {
  const int w = this->mxconfig_.mx_width * this->chain_cols_;
  const int h = this->mxconfig_.mx_height * this->chain_rows_;
  return (this->rotation_ == 1 || this->rotation_ == 3) ? w : h;
}

void Hub75DmaDisplay::fill(Color color) {
  if (this->dma_display_ == nullptr)
    return;
  this->dma_display_->fillScreenRGB888(color.r, color.g, color.b);
}

void Hub75DmaDisplay::draw_pixels_at(int x_start, int y_start, int w, int h, const uint8_t *ptr,
                                     display::ColorOrder order, display::ColorBitness bitness, bool big_endian,
                                     int x_offset, int y_offset, int x_pad) {
  if (this->dma_display_ == nullptr)
    return;
  if (bitness == display::COLOR_BITNESS_888 && order == display::COLOR_ORDER_RGB && !big_endian) {
    const int stride = x_offset + w + x_pad;
    for (int y = 0; y < h; y++) {
      const uint8_t *row = ptr + static_cast<size_t>(y_offset + y) * static_cast<size_t>(stride) * 3 +
                           static_cast<size_t>(x_offset) * 3;
      for (int x = 0; x < w; x++) {
        const uint8_t *px = row + static_cast<size_t>(x) * 3;
        this->draw_rgb_(x_start + x, y_start + y, px[0], px[1], px[2]);
      }
    }
    return;
  }
  display::Display::draw_pixels_at(x_start, y_start, w, h, ptr, order, bitness, big_endian, x_offset, y_offset, x_pad);
}

void Hub75DmaBrightness::setup() {
  if (this->parent_ != nullptr)
    this->publish_state(this->parent_->get_initial_brightness());
}

void Hub75DmaBrightness::dump_config() { LOG_NUMBER("", "HUB75 DMA Brightness", this); }

void Hub75DmaBrightness::control(float value) {
  if (this->parent_ != nullptr)
    this->parent_->set_brightness(static_cast<uint8_t>(value));
  this->publish_state(value);
}

void Hub75DmaPowerSwitch::setup() {
  this->publish_state(true);
}

void Hub75DmaPowerSwitch::dump_config() { LOG_SWITCH("", "HUB75 DMA Power", this); }

void Hub75DmaPowerSwitch::write_state(bool state) {
  if (this->parent_ != nullptr)
    this->parent_->set_state(state);
  this->publish_state(state);
}

}  // namespace hub75_dma
}  // namespace esphome
