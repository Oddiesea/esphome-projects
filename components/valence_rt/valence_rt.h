#pragma once

#include "valence_protocol.h"

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"

#include <cstring>
#include <string>

namespace esphome {
namespace valence_rt {

enum class ValenceRole : uint8_t {
  DIRECT = 0,   // Relay board: Valence RS485 on uart_id
  CLIENT = 1,   // Legacy: POLL a bridge over uart_id
  SERVER = 2,   // Legacy: Valence on uart_id, answer POLL on link_uart
};

struct BatterySensors {
  sensor::Sensor *soc{nullptr};
  sensor::Sensor *voltage{nullptr};
  sensor::Sensor *current{nullptr};
  sensor::Sensor *current_2{nullptr};
  sensor::Sensor *power{nullptr};
  sensor::Sensor *temperature{nullptr};
  sensor::Sensor *temperature_pcb{nullptr};
  sensor::Sensor *cell_voltage_1{nullptr};
  sensor::Sensor *cell_voltage_2{nullptr};
  sensor::Sensor *cell_voltage_3{nullptr};
  sensor::Sensor *cell_voltage_4{nullptr};
};

class ValenceRTComponent : public PollingComponent, public uart::UARTDevice {
 public:
  void set_max_batteries(uint8_t max_batteries) { this->max_batteries_ = max_batteries; }
  void set_role(ValenceRole role) { this->role_ = role; }
  void set_link_uart(uart::UARTComponent *uart) { this->link_uart_ = uart; }

  void set_soc_sensor(uint8_t index, sensor::Sensor *s) { this->batteries_[index].soc = s; }
  void set_voltage_sensor(uint8_t index, sensor::Sensor *s) { this->batteries_[index].voltage = s; }
  void set_current_sensor(uint8_t index, sensor::Sensor *s) { this->batteries_[index].current = s; }
  void set_current_2_sensor(uint8_t index, sensor::Sensor *s) { this->batteries_[index].current_2 = s; }
  void set_power_sensor(uint8_t index, sensor::Sensor *s) { this->batteries_[index].power = s; }
  void set_temperature_sensor(uint8_t index, sensor::Sensor *s) { this->batteries_[index].temperature = s; }
  void set_temperature_pcb_sensor(uint8_t index, sensor::Sensor *s) {
    this->batteries_[index].temperature_pcb = s;
  }
  void set_cell_voltage_1_sensor(uint8_t index, sensor::Sensor *s) {
    this->batteries_[index].cell_voltage_1 = s;
  }
  void set_cell_voltage_2_sensor(uint8_t index, sensor::Sensor *s) {
    this->batteries_[index].cell_voltage_2 = s;
  }
  void set_cell_voltage_3_sensor(uint8_t index, sensor::Sensor *s) {
    this->batteries_[index].cell_voltage_3 = s;
  }
  void set_cell_voltage_4_sensor(uint8_t index, sensor::Sensor *s) {
    this->batteries_[index].cell_voltage_4 = s;
  }

  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void flush_valence_rx_();
  void send_valence_frame_(const uint8_t *payload, size_t len);
  size_t read_valence_response_(uint8_t *buffer, size_t max_len, uint32_t wait_ms);

  void clear_readings_();
  void store_and_publish_(uint8_t index, const uint8_t *frame);
  void publish_reading_(uint8_t index);
  void publish_unavailable_(uint8_t index);
  uint8_t poll_valence_();

  void flush_link_rx_();
  void link_write_str_(const char *str);
  bool link_read_line_(std::string &line, uint32_t timeout_ms);
  void handle_server_poll_request_();
  void write_bridge_response_();
  void client_poll_bridge_();
  bool parse_b_line_(const std::string &line);

  ValenceRole role_{ValenceRole::DIRECT};
  uart::UARTComponent *link_uart_{nullptr};
  uint8_t max_batteries_{MAX_BATTERIES};
  BatterySensors batteries_[MAX_BATTERIES];
  BatteryReading readings_[MAX_BATTERIES];

  uint8_t assigned_count_{0};
  uint8_t bus_ids_[MAX_BATTERIES]{};
  uint8_t seen_ids_[MAX_BATTERIES][BATTERY_ID_LEN]{};
  uint8_t seen_count_{0};

  std::string link_line_buf_;
};

}  // namespace valence_rt
}  // namespace esphome
