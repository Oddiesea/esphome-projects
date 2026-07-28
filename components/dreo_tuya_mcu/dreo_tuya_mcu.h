#pragma once

#include "dreo_tuya_protocol.h"

#include "esphome/components/button/button.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

#include <map>
#include <vector>

namespace esphome {
namespace dreo_tuya_mcu {

class DreoTuyaMcuComponent;

class DreoTuyaSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(DreoTuyaMcuComponent *parent) { this->parent_ = parent; }
  void set_datapoint(uint8_t dp) { this->dp_ = dp; }

 protected:
  void write_state(bool state) override;

  DreoTuyaMcuComponent *parent_{nullptr};
  uint8_t dp_{0};
};

class DreoTuyaSelect : public select::Select, public Component {
 public:
  void set_parent(DreoTuyaMcuComponent *parent) { this->parent_ = parent; }
  void set_datapoint(uint8_t dp) { this->dp_ = dp; }

 protected:
  void control(const std::string &value) override;

  DreoTuyaMcuComponent *parent_{nullptr};
  uint8_t dp_{0};
};

class DreoTuyaNumber : public number::Number, public Component {
 public:
  void set_parent(DreoTuyaMcuComponent *parent) { this->parent_ = parent; }
  void set_datapoint(uint8_t dp) { this->dp_ = dp; }

 protected:
  void control(float value) override;

  DreoTuyaMcuComponent *parent_{nullptr};
  uint8_t dp_{0};
};

class DreoTuyaQueryButton : public button::Button, public Component {
 public:
  void set_parent(DreoTuyaMcuComponent *parent) { this->parent_ = parent; }

 protected:
  void press_action() override;

  DreoTuyaMcuComponent *parent_{nullptr};
};

class DreoTuyaMcuComponent : public Component, public uart::UARTDevice {
 public:
  void set_time(time::RealTimeClock *time) { this->time_ = time; }

  void register_switch(uint8_t dp, DreoTuyaSwitch *sw) { this->switches_[dp] = sw; }
  void register_select(uint8_t dp, DreoTuyaSelect *sel) { this->selects_[dp] = sel; }
  void register_number(uint8_t dp, DreoTuyaNumber *num) { this->numbers_[dp] = num; }
  void register_sensor(uint8_t dp, sensor::Sensor *sens) { this->sensors_[dp] = sens; }
  void register_text_sensor(uint8_t dp, text_sensor::TextSensor *ts) { this->text_sensors_[dp] = ts; }
  void register_query_button(DreoTuyaQueryButton *btn) { this->query_button_ = btn; }

  bool send_dp_bool(uint8_t dp, bool value);
  bool send_dp_enum(uint8_t dp, uint32_t value);
  bool send_dp_value(uint8_t dp, uint32_t value);
  void query_dps();

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::BUS; }

 protected:
  void process_rx_buffer_();
  void handle_frame_(uint8_t cmd, const uint8_t *data, uint16_t dlen);
  void handle_dp_report_(const uint8_t *data, uint16_t dlen);
  void publish_dp_(const DpValue &dp);
  void write_frame_(const uint8_t *data, size_t len);
  void tick_(uint32_t now_ms);

  time::RealTimeClock *time_{nullptr};
  DreoTuyaQueryButton *query_button_{nullptr};

  int handshake_phase_{0};
  bool suppress_send_{false};
  bool dp_queried_{false};

  uint32_t last_heartbeat_ms_{0};
  uint32_t last_wifi_status_ms_{0};
  uint32_t boot_ms_{0};
  uint32_t last_time_req_log_ms_{0};
  uint32_t last_time_reply_ms_{0};

  std::vector<uint8_t> rx_buffer_;

  std::map<uint8_t, DreoTuyaSwitch *> switches_;
  std::map<uint8_t, DreoTuyaSelect *> selects_;
  std::map<uint8_t, DreoTuyaNumber *> numbers_;
  std::map<uint8_t, sensor::Sensor *> sensors_;
  std::map<uint8_t, text_sensor::TextSensor *> text_sensors_;
};

}  // namespace dreo_tuya_mcu
}  // namespace esphome
