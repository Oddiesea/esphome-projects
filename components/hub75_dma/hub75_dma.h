#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/components/display/display_buffer.h"
#include "esphome/components/number/number.h"
#include "esphome/components/switch/switch.h"
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif

// Vendor VirtualMatrixPanel headers trip -Wparentheses / -Wtype-limits /
// -Wimplicit-fallthrough on stock ESP-IDF builds; silence only around the include.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"
#include "ESP32-VirtualMatrixPanel-I2S-DMA.h"
#pragma GCC diagnostic pop

#include <vector>
#include <functional>

namespace esphome {
namespace hub75_dma {

class Hub75DmaBrightness;
class Hub75DmaCompensation;
class Hub75DmaPowerSwitch;
class Hub75DmaAdaptiveSwitch;

class Hub75DmaDisplay : public display::DisplayBuffer {
 public:
  void setup() override;
  void dump_config() override;
  void update() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  display::DisplayType get_display_type() override { return display::DisplayType::DISPLAY_TYPE_COLOR; }

  void set_panel_width(uint16_t width) { this->mxconfig_.mx_width = width; }
  void set_panel_height(uint16_t height) { this->mxconfig_.mx_height = height; }
  void set_chain_length(uint16_t chain_length) { this->mxconfig_.chain_length = chain_length; }
  void set_chain_rows(uint8_t rows) { this->chain_rows_ = rows == 0 ? 1 : rows; }
  void set_chain_cols(uint8_t cols) { this->chain_cols_ = cols == 0 ? 1 : cols; }
  void set_chain_type(PANEL_CHAIN_TYPE type) { this->chain_type_ = type; }
  void set_rotation(uint8_t rotation) { this->rotation_ = rotation & 3; }
  void set_initial_brightness(uint8_t brightness) { this->initial_brightness_ = brightness; }
  void set_double_buffer(bool double_buffer) { this->mxconfig_.double_buff = double_buffer; }
  void set_shift_driver(HUB75_I2S_CFG::shift_driver driver) { this->mxconfig_.driver = driver; }
  void set_line_decoder(HUB75_I2S_CFG::line_driver decoder) { this->mxconfig_.line_decoder = decoder; }
  void set_i2sspeed(HUB75_I2S_CFG::clk_speed speed) { this->mxconfig_.i2sspeed = speed; }
  void set_latch_blanking(uint8_t latch_blanking) { this->mxconfig_.latch_blanking = latch_blanking; }
  void set_clock_phase(bool clock_phase) { this->mxconfig_.clkphase = clock_phase; }

  void set_pins(InternalGPIOPin *r1, InternalGPIOPin *g1, InternalGPIOPin *b1, InternalGPIOPin *r2,
                InternalGPIOPin *g2, InternalGPIOPin *b2, InternalGPIOPin *a, InternalGPIOPin *b, InternalGPIOPin *c,
                InternalGPIOPin *d, InternalGPIOPin *e, InternalGPIOPin *lat, InternalGPIOPin *oe,
                InternalGPIOPin *clk);

  void fill(Color color) override;
  void draw_pixels_at(int x_start, int y_start, int w, int h, const uint8_t *ptr, display::ColorOrder order,
                      display::ColorBitness bitness, bool big_endian, int x_offset, int y_offset, int x_pad) override;
  void set_state(bool state);
  void set_brightness(uint8_t brightness);
  uint8_t get_initial_brightness() const { return this->initial_brightness_; }

  void register_brightness(Hub75DmaBrightness *brightness);
  void register_power_switch(Hub75DmaPowerSwitch *power_switch);
  void add_on_power_state_callback(std::function<void(bool)> cb) { this->power_state_cbs_.push_back(std::move(cb)); }

#ifdef USE_SENSOR
  void set_adaptive_lux_sensor(sensor::Sensor *sensor);
#endif
  void set_adaptive_compensation(Hub75DmaCompensation *compensation);
  void set_adaptive_enable_switch(Hub75DmaAdaptiveSwitch *enable);
  void set_adaptive_range(uint8_t min_brightness, uint8_t max_brightness, float lux_reference);
  void set_adaptive_enabled(bool enabled);
  bool get_adaptive_enabled() const { return this->adaptive_enabled_; }
  float get_adaptive_compensation() const;
  void apply_adaptive_brightness(float lux);
  void reapply_adaptive_brightness();
  float get_last_lux() const { return this->last_lux_; }

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override;
  int get_width_internal() override;
  int get_height_internal() override;
  void draw_rgb_(int x, int y, uint8_t r, uint8_t g, uint8_t b);
  bool use_virtual_() const;
  void on_lux_(float lux);

  MatrixPanel_I2S_DMA *dma_display_{nullptr};
  VirtualMatrixPanel *virtual_{nullptr};
  HUB75_I2S_CFG mxconfig_;
  uint8_t chain_rows_{1};
  uint8_t chain_cols_{1};
  PANEL_CHAIN_TYPE chain_type_{CHAIN_NONE};
  uint8_t rotation_{0};
  uint8_t initial_brightness_{128};
  bool enabled_{true};
  std::vector<Hub75DmaBrightness *> brightness_values_;
  std::vector<Hub75DmaPowerSwitch *> power_switches_;
  std::vector<std::function<void(bool)>> power_state_cbs_;

#ifdef USE_SENSOR
  sensor::Sensor *adaptive_lux_sensor_{nullptr};
#endif
  Hub75DmaCompensation *adaptive_compensation_{nullptr};
  Hub75DmaAdaptiveSwitch *adaptive_enable_{nullptr};
  bool adaptive_enabled_{true};
  uint8_t adaptive_min_{8};
  uint8_t adaptive_max_{255};
  float adaptive_lux_reference_{500.0f};
  float last_lux_{NAN};
};

class Hub75DmaBrightness : public number::Number, public Component {
 public:
  void set_parent(Hub75DmaDisplay *parent) { this->parent_ = parent; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void control(float value) override;
  Hub75DmaDisplay *parent_{nullptr};
};

class Hub75DmaCompensation : public number::Number, public Component {
 public:
  void set_parent(Hub75DmaDisplay *parent) { this->parent_ = parent; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void control(float value) override;
  Hub75DmaDisplay *parent_{nullptr};
};

class Hub75DmaPowerSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(Hub75DmaDisplay *parent) { this->parent_ = parent; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void write_state(bool state) override;
  Hub75DmaDisplay *parent_{nullptr};
};

class Hub75DmaAdaptiveSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(Hub75DmaDisplay *parent) { this->parent_ = parent; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void write_state(bool state) override;
  Hub75DmaDisplay *parent_{nullptr};
};

template<typename... Ts> class SetBrightnessAction : public Action<Ts...>, public Parented<Hub75DmaDisplay> {
 public:
  TEMPLATABLE_VALUE(uint8_t, brightness)
  void play(Ts... x) override { this->parent_->set_brightness(this->brightness_.value(x...)); }
};

}  // namespace hub75_dma
}  // namespace esphome
