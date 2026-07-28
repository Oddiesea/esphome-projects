#include "dreo_tuya_mcu.h"

#include "esphome/components/wifi/wifi_component.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"

#include <cstdio>
#include <cstdlib>

namespace esphome {
namespace dreo_tuya_mcu {

static const char *const TAG = "dreo_tuya_mcu";

static const uint8_t FRAME_QUERY_DPS[] = {0x55, 0xAA, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x07};
static const uint8_t FRAME_HEARTBEAT[] = {0x55, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF};
static const uint8_t FRAME_WIFI_CONNECTED[] = {0x55, 0xAA, 0x00, 0x00, 0x03, 0x00, 0x00, 0x01, 0x04, 0x07};
static const uint8_t FRAME_ACK_PRODUCT[] = {0x55, 0xAA, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
static const uint8_t FRAME_ACK_WORK_MODE[] = {0x55, 0xAA, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01};

void DreoTuyaSwitch::write_state(bool state) {
  if (this->parent_ != nullptr && this->parent_->send_dp_bool(this->dp_, state))
    this->publish_state(state);
}

void DreoTuyaSelect::control(const std::string &value) {
  if (this->parent_ == nullptr)
    return;
  uint32_t val = 0;
  if (this->dp_ == 3) {
    if (value == "Normal")
      val = 1;
    else if (value == "Natural")
      val = 2;
    else if (value == "Sleep")
      val = 3;
    else if (value == "Auto")
      val = 4;
    else
      return;
  } else if (this->dp_ == 4) {
    val = static_cast<uint32_t>(atoi(value.c_str()));
    if (val < 1 || val > 8)
      return;
  } else {
    return;
  }
  if (this->parent_->send_dp_enum(this->dp_, val))
    this->publish_state(value);
}

void DreoTuyaNumber::control(float value) {
  if (this->parent_ != nullptr && this->parent_->send_dp_value(this->dp_, static_cast<uint32_t>(value)))
    this->publish_state(value);
}

void DreoTuyaQueryButton::press_action() {
  if (this->parent_ != nullptr)
    this->parent_->query_dps();
}

void DreoTuyaMcuComponent::setup() {
  this->boot_ms_ = millis();
  ESP_LOGCONFIG(TAG, "Dreo Tuya MCU (DR-HTF007S protocol)");
  this->check_uart_settings(115200);
}

void DreoTuyaMcuComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "  Handshake phase: %d", this->handshake_phase_);
}

void DreoTuyaMcuComponent::loop() {
  uint8_t byte;
  while (this->available()) {
    if (!this->read_byte(&byte))
      break;
    this->rx_buffer_.push_back(byte);
  }
  this->process_rx_buffer_();
  this->tick_(millis());
}

void DreoTuyaMcuComponent::write_frame_(const uint8_t *data, size_t len) {
  this->write_array(data, len);
  this->flush();
}

bool DreoTuyaMcuComponent::send_dp_bool(uint8_t dp, bool value) {
  if (this->suppress_send_)
    return false;
  auto frame = build_dp_write_frame(dp, 0x01, value ? 1 : 0);
  this->write_frame_(frame.data(), frame.size());
  ESP_LOGD(TAG, "TX DP%u bool=%u", dp, value ? 1 : 0);
  return true;
}

bool DreoTuyaMcuComponent::send_dp_enum(uint8_t dp, uint32_t value) {
  if (this->suppress_send_)
    return false;
  auto frame = build_dp_write_frame(dp, 0x04, value);
  this->write_frame_(frame.data(), frame.size());
  ESP_LOGD(TAG, "TX DP%u enum=%u", dp, value);
  return true;
}

bool DreoTuyaMcuComponent::send_dp_value(uint8_t dp, uint32_t value) {
  if (this->suppress_send_)
    return false;
  auto frame = build_dp_write_frame(dp, 0x02, value);
  this->write_frame_(frame.data(), frame.size());
  ESP_LOGD(TAG, "TX DP%u value=%u", dp, value);
  return true;
}

void DreoTuyaMcuComponent::query_dps() {
  this->write_frame_(FRAME_QUERY_DPS, sizeof(FRAME_QUERY_DPS));
  ESP_LOGI(TAG, "Manual DP query sent");
}

void DreoTuyaMcuComponent::process_rx_buffer_() {
  int pos = 0;
  while (pos + 9 <= static_cast<int>(this->rx_buffer_.size())) {
    if (this->rx_buffer_[pos] != 0x55 || this->rx_buffer_[pos + 1] != 0xAA) {
      pos++;
      continue;
    }
    if (pos + 8 > static_cast<int>(this->rx_buffer_.size()))
      break;

    uint8_t cmd = this->rx_buffer_[pos + 4];
    uint16_t dlen = (static_cast<uint16_t>(this->rx_buffer_[pos + 6]) << 8) | this->rx_buffer_[pos + 7];
    int pkt_end = pos + 8 + dlen + 1;
    if (pkt_end > static_cast<int>(this->rx_buffer_.size()))
      break;

    uint8_t csum = frame_checksum(this->rx_buffer_.data() + pos, pkt_end - pos - 1);
    if (csum != this->rx_buffer_[pkt_end - 1]) {
      ESP_LOGW(TAG, "Checksum fail: calc=0x%02X got=0x%02X", csum, this->rx_buffer_[pkt_end - 1]);
      pos++;
      continue;
    }

    this->handle_frame_(cmd, this->rx_buffer_.data() + pos + 8, dlen);
    pos = pkt_end;
  }

  if (pos > 0)
    this->rx_buffer_.erase(this->rx_buffer_.begin(), this->rx_buffer_.begin() + pos);
}

void DreoTuyaMcuComponent::handle_frame_(uint8_t cmd, const uint8_t *data, uint16_t dlen) {
  switch (cmd) {
    case 0x00:
      if (this->handshake_phase_ < 3) {
        if (this->handshake_phase_ == 0)
          ESP_LOGI(TAG, "First heartbeat, starting handshake");
        this->handshake_phase_ = 1;
        this->write_frame_(FRAME_WIFI_CONNECTED, sizeof(FRAME_WIFI_CONNECTED));
      } else {
        ESP_LOGD(TAG, "Heartbeat OK (phase=%d)", this->handshake_phase_);
      }
      break;

    case 0x01: {
      std::string info(reinterpret_cast<const char *>(data), dlen);
      ESP_LOGI(TAG, "Product info: %s", info.c_str());
      this->write_frame_(FRAME_ACK_PRODUCT, sizeof(FRAME_ACK_PRODUCT));
      this->write_frame_(FRAME_WIFI_CONNECTED, sizeof(FRAME_WIFI_CONNECTED));
      ESP_LOGI(TAG, "Sent WiFi status (connected)");
      if (this->handshake_phase_ < 2)
        this->handshake_phase_ = 2;
      break;
    }

    case 0x02:
      ESP_LOGI(TAG, "MCU working mode received");
      this->write_frame_(FRAME_ACK_WORK_MODE, sizeof(FRAME_ACK_WORK_MODE));
      break;

    case 0x07:
      this->handle_dp_report_(data, dlen);
      break;

    case 0x03:
      ESP_LOGD(TAG, "WiFi status ack");
      if (!this->dp_queried_) {
        this->write_frame_(FRAME_QUERY_DPS, sizeof(FRAME_QUERY_DPS));
        this->dp_queried_ = true;
        ESP_LOGI(TAG, "Sent DP query");
      }
      break;

    case 0x0E: {
      uint32_t tnow = millis();
      if (tnow - this->last_time_req_log_ms_ > 2000) {
        this->last_time_req_log_ms_ = tnow;
        std::string dump;
        char buf[8];
        for (uint16_t i = 0; i < dlen && i < 12; i++) {
          snprintf(buf, sizeof(buf), "%02X ", data[i]);
          dump += buf;
        }
        ESP_LOGI(TAG, "Time req data: %s", dump.c_str());
      }
      if (this->time_ == nullptr)
        break;
      auto now = this->time_->now();
      if (!now.is_valid() || tnow - this->last_time_reply_ms_ <= 500)
        break;
      this->last_time_reply_ms_ = tnow;
      uint8_t dow = now.day_of_week;
      if (dow == 1)
        dow = 7;
      else
        dow -= 1;
      auto payload = build_time_payload(static_cast<uint8_t>(now.year % 100), static_cast<uint8_t>(now.month),
                                        static_cast<uint8_t>(now.day_of_month), static_cast<uint8_t>(now.hour),
                                        static_cast<uint8_t>(now.minute), static_cast<uint8_t>(now.second), dow);
      std::vector<uint8_t> frame = {0x55, 0xAA, 0x00, 0x00, 0x0E, 0x00, 0x00, static_cast<uint8_t>(payload.size())};
      frame.insert(frame.end(), payload.begin(), payload.end());
      append_checksum(frame);
      this->write_frame_(frame.data(), frame.size());
      ESP_LOGI(TAG, "Sent time: %02d-%02d-%02d %02d:%02d:%02d dow=%u", now.year % 100, now.month, now.day_of_month,
               now.hour, now.minute, now.second, dow);
      break;
    }

    case 0x04:
    case 0x05: {
      ESP_LOGW(TAG, "WiFi reset (cmd 0x%02X) — entering safe mode AP", cmd);
      uint8_t ws[] = {0x55, 0xAA, 0x00, 0x00, 0x03, 0x00, 0x00, 0x01, 0x01, 0x00};
      ws[9] = frame_checksum(ws, 9);
      this->write_frame_(ws, sizeof(ws));
      App.safe_reboot();
      break;
    }

    default: {
      std::string hex;
      char buf[8];
      for (uint16_t i = 0; i < dlen && i < 16; i++) {
        snprintf(buf, sizeof(buf), "%02X ", data[i]);
        hex += buf;
      }
      ESP_LOGD(TAG, "Cmd 0x%02X len=%u data=%s", cmd, dlen, hex.c_str());
      break;
    }
  }
}

void DreoTuyaMcuComponent::handle_dp_report_(const uint8_t *data, uint16_t dlen) {
  ESP_LOGI(TAG, "DP report (%u bytes)", dlen);
  this->suppress_send_ = true;

  size_t offset = 0;
  while (offset < dlen) {
    DpValue dp{};
    size_t consumed = decode_dp_tlv(data + offset, dlen - offset, &dp);
    if (consumed == 0)
      break;
    ESP_LOGD(TAG, "DP%u type=%u val=%u", dp.id, dp.type, dp.value);
    this->publish_dp_(dp);
    offset += consumed;
  }

  this->suppress_send_ = false;
  if (this->handshake_phase_ < 3)
    this->handshake_phase_ = 3;
}

void DreoTuyaMcuComponent::publish_dp_(const DpValue &dp) {
  auto sw_it = this->switches_.find(dp.id);
  if (sw_it != this->switches_.end() && sw_it->second != nullptr) {
    sw_it->second->publish_state(dp.value != 0);
    return;
  }

  auto sel_it = this->selects_.find(dp.id);
  if (sel_it != this->selects_.end() && sel_it->second != nullptr) {
    if (dp.id == 3) {
      const char *m = nullptr;
      switch (dp.value) {
        case 1:
          m = "Normal";
          break;
        case 2:
          m = "Natural";
          break;
        case 3:
          m = "Sleep";
          break;
        case 4:
          m = "Auto";
          break;
        default:
          break;
      }
      if (m != nullptr)
        sel_it->second->publish_state(m);
    } else if (dp.id == 4 && dp.value >= 1 && dp.value <= 8) {
      char buf[4];
      snprintf(buf, sizeof(buf), "%u", dp.value);
      sel_it->second->publish_state(buf);
    }
    return;
  }

  auto num_it = this->numbers_.find(dp.id);
  if (num_it != this->numbers_.end() && num_it->second != nullptr) {
    num_it->second->publish_state(static_cast<float>(dp.value));
    return;
  }

  auto sens_it = this->sensors_.find(dp.id);
  if (sens_it != this->sensors_.end() && sens_it->second != nullptr) {
    sens_it->second->publish_state(static_cast<float>(dp.value));
    return;
  }

  auto ts_it = this->text_sensors_.find(dp.id);
  if (ts_it != this->text_sensors_.end() && ts_it->second != nullptr) {
    if (dp.id == 9) {
      const char *f = "OK";
      if (dp.value == 1)
        f = "E1";
      else if (dp.value == 2)
        f = "EU";
      else if (dp.value > 0)
        f = "Unknown";
      ts_it->second->publish_state(f);
    } else if (dp.id == 12) {
      ts_it->second->publish_state(dp.value == 0 ? "°C" : "°F");
    }
    return;
  }

  ESP_LOGD(TAG, "Unhandled DP%u type=%u val=%u", dp.id, dp.type, dp.value);
}

void DreoTuyaMcuComponent::tick_(uint32_t now_ms) {
  uint32_t hb_ms = this->handshake_phase_ >= 3 ? 15000 : 3000;
  if (now_ms - this->last_heartbeat_ms_ >= hb_ms) {
    this->write_frame_(FRAME_HEARTBEAT, sizeof(FRAME_HEARTBEAT));
    this->last_heartbeat_ms_ = now_ms;
  }

  if (this->handshake_phase_ >= 3 && now_ms - this->last_wifi_status_ms_ >= 30000) {
    bool connected = wifi::global_wifi_component->is_connected();
    uint8_t status = connected ? 0x04 : 0x02;
    uint8_t ws[] = {0x55, 0xAA, 0x00, 0x00, 0x03, 0x00, 0x00, 0x01, status, 0x00};
    ws[9] = frame_checksum(ws, 9);
    this->write_frame_(ws, sizeof(ws));
    this->last_wifi_status_ms_ = now_ms;
  }

  if (this->handshake_phase_ >= 1 && !this->dp_queried_ && now_ms - this->boot_ms_ > 15000) {
    this->write_frame_(FRAME_QUERY_DPS, sizeof(FRAME_QUERY_DPS));
    this->dp_queried_ = true;
    ESP_LOGI(TAG, "Fallback DP query");
  }
}

}  // namespace dreo_tuya_mcu
}  // namespace esphome
