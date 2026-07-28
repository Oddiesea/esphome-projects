#include "valence_rt.h"
#include "esphome/core/log.h"

#include <cstdio>
#include <cstdlib>

namespace esphome {
namespace valence_rt {

static const char *const TAG = "valence_rt";

static const uint8_t REQ_INIT[] = {0x00, 0x03, 0x00, 0xFC, 0x00, 0x00};
static const uint8_t REQ_BROADCAST[] = {0xFF, 0x43, 0x06, 0x06};
static const uint8_t REQ_ID_POLL_PREFIX[] = {0xFF, 0x50, 0x06};
static const uint8_t REQ_ASSIGN_PREFIX[] = {0xFF, 0x41, 0x12};
static const uint8_t REQ_DATA_TAIL[] = {0x03, 0x00, 0x29, 0x00, 0x1B};

void ValenceRTComponent::setup() {
  if (this->role_ == ValenceRole::DIRECT || this->role_ == ValenceRole::SERVER) {
    this->flush_valence_rx_();
  }
  if (this->role_ == ValenceRole::SERVER) {
    this->flush_link_rx_();
    ESP_LOGCONFIG(TAG, "Valence RT bridge SERVER ready (max %u batteries)", this->max_batteries_);
  } else if (this->role_ == ValenceRole::CLIENT) {
    this->flush_link_rx_();
    ESP_LOGCONFIG(TAG, "Valence RT bridge CLIENT ready (max %u batteries)", this->max_batteries_);
  } else {
    ESP_LOGCONFIG(TAG, "Valence RT DIRECT ready (max %u batteries)", this->max_batteries_);
  }
}

void ValenceRTComponent::dump_config() {
  const char *role = "direct";
  if (this->role_ == ValenceRole::SERVER)
    role = "server";
  else if (this->role_ == ValenceRole::CLIENT)
    role = "client";
  ESP_LOGCONFIG(TAG, "Valence U1-12RT:");
  ESP_LOGCONFIG(TAG, "  Role: %s", role);
  ESP_LOGCONFIG(TAG, "  Max batteries: %u", this->max_batteries_);
  ESP_LOGCONFIG(TAG, "  Update interval: %u ms", this->get_update_interval());
  this->check_uart_settings(115200);
  if (this->role_ == ValenceRole::SERVER && this->link_uart_ == nullptr) {
    ESP_LOGE(TAG, "Server role requires link_uart_id");
  }
}

void ValenceRTComponent::loop() {
  if (this->role_ != ValenceRole::SERVER || this->link_uart_ == nullptr)
    return;

  while (this->link_uart_->available()) {
    uint8_t c;
    if (!this->link_uart_->read_byte(&c))
      break;
    if (c == '\r')
      continue;
    if (c == '\n') {
      if (this->link_line_buf_ == "POLL") {
        this->handle_server_poll_request_();
      } else if (!this->link_line_buf_.empty()) {
        ESP_LOGD(TAG, "Ignoring link cmd: %s", this->link_line_buf_.c_str());
      }
      this->link_line_buf_.clear();
    } else if (this->link_line_buf_.size() < 32) {
      this->link_line_buf_ += static_cast<char>(c);
    } else {
      this->link_line_buf_.clear();
    }
  }
}

void ValenceRTComponent::update() {
  if (this->role_ == ValenceRole::CLIENT) {
    this->client_poll_bridge_();
    return;
  }
  // DIRECT / SERVER: poll Valence RS485 on parent uart_id
  uint8_t n = this->poll_valence_();
  if (n == 0) {
    for (uint8_t i = 0; i < this->max_batteries_; i++)
      this->publish_unavailable_(i);
  }
}

void ValenceRTComponent::handle_server_poll_request_() {
  ESP_LOGD(TAG, "Link POLL received");
  this->poll_valence_();
  this->write_bridge_response_();
}

void ValenceRTComponent::write_bridge_response_() {
  char line[128];
  uint8_t count = 0;
  for (uint8_t i = 0; i < this->max_batteries_; i++) {
    const BatteryReading &r = this->readings_[i];
    if (!r.valid)
      continue;
    count++;
    snprintf(line, sizeof(line), "B,%u,%.1f,%.3f,%.3f,%.3f,%.0f,%.0f,%.3f,%.3f,%.3f,%.3f\n", i + 1, r.soc, r.voltage,
             r.current, r.current_2, r.temperature, r.temperature_pcb, r.cell_voltage_1, r.cell_voltage_2,
             r.cell_voltage_3, r.cell_voltage_4);
    this->link_write_str_(line);
  }
  if (count == 0) {
    this->link_write_str_("ERR,none\n");
  } else {
    snprintf(line, sizeof(line), "OK,%u\n", count);
    this->link_write_str_(line);
  }
}

void ValenceRTComponent::client_poll_bridge_() {
  this->flush_link_rx_();
  this->clear_readings_();

  // Parent uart_ is the link on client
  const char *poll = "POLL\n";
  this->write_array(reinterpret_cast<const uint8_t *>(poll), 5);
  this->flush();

  bool got_ok = false;
  bool got_err = false;
  uint32_t start = millis();
  const uint32_t timeout_ms = 4000;

  while ((millis() - start) < timeout_ms) {
    std::string line;
    if (!this->link_read_line_(line, 50))
      continue;
    if (line.empty())
      continue;
    ESP_LOGD(TAG, "Link RX: %s", line.c_str());
    if (line.rfind("B,", 0) == 0) {
      this->parse_b_line_(line);
    } else if (line.rfind("OK,", 0) == 0) {
      got_ok = true;
      break;
    } else if (line.rfind("ERR,", 0) == 0) {
      got_err = true;
      break;
    }
  }

  if (got_err || (!got_ok && !got_err)) {
    if (got_err) {
      ESP_LOGW(TAG, "Bridge reported no Valence packs (idle/asleep or disconnected)");
    } else {
      ESP_LOGW(TAG, "Bridge POLL timed out — is the bridge ESP32 powered and linked?");
    }
    for (uint8_t i = 0; i < this->max_batteries_; i++)
      this->publish_unavailable_(i);
    return;
  }

  for (uint8_t i = 0; i < this->max_batteries_; i++) {
    if (this->readings_[i].valid)
      this->publish_reading_(i);
    else
      this->publish_unavailable_(i);
  }
}

bool ValenceRTComponent::parse_b_line_(const std::string &line) {
  uint8_t index = 0;
  BatteryReading r{};
  if (!parse_b_line(line.c_str(), this->max_batteries_, &index, &r)) {
    ESP_LOGW(TAG, "Bad B line: %s", line.c_str());
    return false;
  }
  this->readings_[index] = r;
  return true;
}

void ValenceRTComponent::flush_link_rx_() {
  if (this->role_ == ValenceRole::SERVER) {
    if (this->link_uart_ == nullptr)
      return;
    while (this->link_uart_->available()) {
      uint8_t c;
      this->link_uart_->read_byte(&c);
    }
  } else {
    while (this->available())
      this->read();
  }
  this->link_line_buf_.clear();
}

void ValenceRTComponent::link_write_str_(const char *str) {
  if (this->link_uart_ == nullptr)
    return;
  size_t len = strlen(str);
  this->link_uart_->write_array(reinterpret_cast<const uint8_t *>(str), len);
  this->link_uart_->flush();
}

bool ValenceRTComponent::link_read_line_(std::string &line, uint32_t timeout_ms) {
  // Client: parent uart is the link
  line.clear();
  uint32_t start = millis();
  while ((millis() - start) < timeout_ms) {
    if (!this->available()) {
      delay(1);
      continue;
    }
    int c = this->read();
    if (c < 0)
      continue;
    if (c == '\r')
      continue;
    if (c == '\n')
      return true;
    if (line.size() < 160)
      line += static_cast<char>(c);
  }
  return !line.empty();
}

void ValenceRTComponent::flush_valence_rx_() {
  while (this->available())
    this->read();
}

void ValenceRTComponent::send_valence_frame_(const uint8_t *payload, size_t len) {
  uint16_t crc = modbus_crc16(payload, len);
  this->write_array(payload, len);
  this->write_byte(crc & 0xFF);
  this->write_byte((crc >> 8) & 0xFF);
  this->flush();
  delay(1);
}

size_t ValenceRTComponent::read_valence_response_(uint8_t *buffer, size_t max_len, uint32_t wait_ms) {
  delay(wait_ms);
  size_t count = 0;
  uint32_t start = millis();
  while (count < max_len && (millis() - start) < (wait_ms + 50)) {
    if (!this->available()) {
      delay(1);
      if (!this->available() && count > 0)
        break;
      continue;
    }
    buffer[count++] = this->read();
  }
  return count;
}

void ValenceRTComponent::clear_readings_() {
  for (uint8_t i = 0; i < MAX_BATTERIES; i++)
    this->readings_[i] = BatteryReading{};
}

void ValenceRTComponent::publish_unavailable_(uint8_t index) {
  BatterySensors &s = this->batteries_[index];
  const float nan = NAN;
  if (s.soc != nullptr)
    s.soc->publish_state(nan);
  if (s.voltage != nullptr)
    s.voltage->publish_state(nan);
  if (s.current != nullptr)
    s.current->publish_state(nan);
  if (s.current_2 != nullptr)
    s.current_2->publish_state(nan);
  if (s.power != nullptr)
    s.power->publish_state(nan);
  if (s.temperature != nullptr)
    s.temperature->publish_state(nan);
  if (s.temperature_pcb != nullptr)
    s.temperature_pcb->publish_state(nan);
  if (s.cell_voltage_1 != nullptr)
    s.cell_voltage_1->publish_state(nan);
  if (s.cell_voltage_2 != nullptr)
    s.cell_voltage_2->publish_state(nan);
  if (s.cell_voltage_3 != nullptr)
    s.cell_voltage_3->publish_state(nan);
  if (s.cell_voltage_4 != nullptr)
    s.cell_voltage_4->publish_state(nan);
}

void ValenceRTComponent::publish_reading_(uint8_t index) {
  BatterySensors &s = this->batteries_[index];
  const BatteryReading &r = this->readings_[index];
  if (s.soc != nullptr)
    s.soc->publish_state(r.soc);
  if (s.voltage != nullptr)
    s.voltage->publish_state(r.voltage);
  if (s.current != nullptr)
    s.current->publish_state(r.current);
  if (s.current_2 != nullptr)
    s.current_2->publish_state(r.current_2);
  // P = V × I1 (signed: + charge / − discharge)
  if (s.power != nullptr)
    s.power->publish_state(battery_power_w(r));
  if (s.temperature != nullptr)
    s.temperature->publish_state(r.temperature);
  if (s.temperature_pcb != nullptr)
    s.temperature_pcb->publish_state(r.temperature_pcb);
  if (s.cell_voltage_1 != nullptr)
    s.cell_voltage_1->publish_state(r.cell_voltage_1);
  if (s.cell_voltage_2 != nullptr)
    s.cell_voltage_2->publish_state(r.cell_voltage_2);
  if (s.cell_voltage_3 != nullptr)
    s.cell_voltage_3->publish_state(r.cell_voltage_3);
  if (s.cell_voltage_4 != nullptr)
    s.cell_voltage_4->publish_state(r.cell_voltage_4);
}

void ValenceRTComponent::store_and_publish_(uint8_t index, const uint8_t *cc) {
  BatteryReading r{};
  if (!decode_data_frame(cc, DATA_FRAME_LEN, &r))
    return;
  this->readings_[index] = r;
  this->publish_reading_(index);
  ESP_LOGD(TAG, "Battery %u: SOC=%.1f%% U=%.3fV I1=%.3fA", index + 1, r.soc, r.voltage, r.current);
}

uint8_t ValenceRTComponent::poll_valence_() {
  this->flush_valence_rx_();
  this->clear_readings_();
  this->assigned_count_ = 0;
  this->seen_count_ = 0;
  memset(this->seen_ids_, 0, sizeof(this->seen_ids_));
  memset(this->bus_ids_, 0, sizeof(this->bus_ids_));

  uint8_t next_bus_id = 2;
  uint8_t response[100];

  this->send_valence_frame_(REQ_INIT, sizeof(REQ_INIT));
  this->send_valence_frame_(REQ_INIT, sizeof(REQ_INIT));

  for (int loop = 0; loop < 3; loop++) {
    this->send_valence_frame_(REQ_BROADCAST, sizeof(REQ_BROADCAST));
    this->send_valence_frame_(REQ_BROADCAST, sizeof(REQ_BROADCAST));

    for (uint8_t slot = 0; slot < 6; slot++) {
      if (this->assigned_count_ >= this->max_batteries_)
        break;

      uint8_t poll[4];
      memcpy(poll, REQ_ID_POLL_PREFIX, 3);
      poll[3] = slot;
      this->send_valence_frame_(poll, sizeof(poll));

      size_t n = this->read_valence_response_(response, sizeof(response), 30);
      if (n < 17 || response[0] != 0xFF || response[1] != 0x70)
        continue;
      if (!frame_crc_ok(response, 17)) {
        ESP_LOGW(TAG, "ID poll CRC fail (slot %u)", slot);
        continue;
      }

      const uint8_t *bat_id = &response[3];
      bool duplicate = false;
      for (uint8_t i = 0; i < this->seen_count_; i++) {
        if (memcmp(this->seen_ids_[i], bat_id, BATTERY_ID_LEN) == 0) {
          duplicate = true;
          break;
        }
      }
      if (duplicate)
        continue;

      if (this->seen_count_ < MAX_BATTERIES) {
        memcpy(this->seen_ids_[this->seen_count_], bat_id, BATTERY_ID_LEN);
        this->seen_count_++;
      }

      uint8_t assign[16];
      memcpy(assign, REQ_ASSIGN_PREFIX, 3);
      memcpy(&assign[3], bat_id, BATTERY_ID_LEN);
      assign[15] = next_bus_id;
      this->send_valence_frame_(assign, sizeof(assign));
      this->read_valence_response_(response, sizeof(response), 35);

      this->bus_ids_[this->assigned_count_] = next_bus_id;
      ESP_LOGI(TAG, "Assigned battery bus id %u (slot %u)", next_bus_id, this->assigned_count_);
      this->assigned_count_++;
      next_bus_id++;
    }
  }

  if (this->assigned_count_ == 0) {
    ESP_LOGW(TAG, "No Valence batteries found (pack might be asleep or disconnected)");
    return 0;
  }

  uint8_t valid = 0;
  for (uint8_t i = 0; i < this->assigned_count_; i++) {
    uint8_t req[6];
    req[0] = this->bus_ids_[i];
    memcpy(&req[1], REQ_DATA_TAIL, 5);
    this->send_valence_frame_(req, sizeof(req));

    size_t n = this->read_valence_response_(response, sizeof(response), 30);
    if (n < DATA_FRAME_LEN) {
      size_t extra = this->read_valence_response_(response + n, sizeof(response) - n, 20);
      n += extra;
    }

    if (!is_valid_data_frame(response, n)) {
      ESP_LOGW(TAG, "Bad data frame for bus id %u (len=%u)", this->bus_ids_[i], (unsigned) n);
      continue;
    }
    this->store_and_publish_(i, response);
    valid++;
  }
  return valid;
}

}  // namespace valence_rt
}  // namespace esphome
