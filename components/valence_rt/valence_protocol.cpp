#include "valence_protocol.h"

#include <cstdio>

namespace esphome {
namespace valence_rt {

uint16_t modbus_crc16(const uint8_t *buf, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t pos = 0; pos < len; pos++) {
    crc ^= static_cast<uint16_t>(buf[pos]);
    for (uint8_t i = 0; i < 8; i++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

bool frame_crc_ok(const uint8_t *buf, size_t len) {
  if (buf == nullptr || len < 3)
    return false;
  uint16_t expected = modbus_crc16(buf, len - 2);
  uint16_t actual = static_cast<uint16_t>(buf[len - 2]) | (static_cast<uint16_t>(buf[len - 1]) << 8);
  return expected == actual;
}

bool is_valid_data_frame(const uint8_t *frame, size_t len) {
  if (frame == nullptr || len < DATA_FRAME_LEN)
    return false;
  if (frame[1] != 0x03 || frame[2] != 0x36)
    return false;
  return frame_crc_ok(frame, DATA_FRAME_LEN);
}

static inline float le_u16_milli(const uint8_t *p) {
  return static_cast<float>((static_cast<uint16_t>(p[1]) << 8) | p[0]) / 1000.0f;
}

static inline float le_i16_milli(const uint8_t *p) {
  int16_t raw = static_cast<int16_t>((static_cast<uint16_t>(p[1]) << 8) | p[0]);
  return static_cast<float>(raw) / 1000.0f;
}

bool decode_data_frame(const uint8_t *frame, size_t len, BatteryReading *out) {
  if (frame == nullptr || out == nullptr || len < DATA_FRAME_LEN)
    return false;

  BatteryReading r{};
  r.valid = true;
  r.soc = static_cast<float>(frame[43]) / 255.0f * 100.0f;
  r.current = le_i16_milli(&frame[23]);
  r.current_2 = le_i16_milli(&frame[27]);
  r.voltage = le_u16_milli(&frame[35]);
  r.temperature = static_cast<float>(frame[3]);
  r.temperature_pcb = static_cast<float>(frame[17]);
  r.cell_voltage_1 = le_u16_milli(&frame[49]);
  r.cell_voltage_2 = le_u16_milli(&frame[51]);
  r.cell_voltage_3 = le_u16_milli(&frame[53]);
  r.cell_voltage_4 = le_u16_milli(&frame[55]);
  *out = r;
  return true;
}

float battery_power_w(const BatteryReading &reading) {
  if (!reading.valid || std::isnan(reading.voltage) || std::isnan(reading.current))
    return NAN;
  return reading.voltage * reading.current;
}

bool parse_b_line(const char *line, uint8_t max_batteries, uint8_t *out_index, BatteryReading *out) {
  if (line == nullptr || out_index == nullptr || out == nullptr || max_batteries == 0)
    return false;

  unsigned idx = 0;
  BatteryReading r{};
  int n = std::sscanf(line, "B,%u,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f", &idx, &r.soc, &r.voltage, &r.current, &r.current_2,
                      &r.temperature, &r.temperature_pcb, &r.cell_voltage_1, &r.cell_voltage_2, &r.cell_voltage_3,
                      &r.cell_voltage_4);
  if (n != 11 || idx < 1 || idx > max_batteries)
    return false;

  r.valid = true;
  *out_index = static_cast<uint8_t>(idx - 1);
  *out = r;
  return true;
}

}  // namespace valence_rt
}  // namespace esphome
