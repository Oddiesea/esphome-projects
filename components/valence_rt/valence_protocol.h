#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace esphome {
namespace valence_rt {

static constexpr uint8_t MAX_BATTERIES = 4;
static constexpr uint8_t BATTERY_ID_LEN = 12;
static constexpr size_t DATA_FRAME_LEN = 59;

struct BatteryReading {
  bool valid{false};
  float soc{NAN};
  float voltage{NAN};
  float current{NAN};
  float current_2{NAN};
  float temperature{NAN};
  float temperature_pcb{NAN};
  float cell_voltage_1{NAN};
  float cell_voltage_2{NAN};
  float cell_voltage_3{NAN};
  float cell_voltage_4{NAN};
};

/** Modbus RTU CRC-16 (poly 0xA001). */
uint16_t modbus_crc16(const uint8_t *buf, size_t len);

/** True if trailing 2 bytes match Modbus CRC of the preceding payload. */
bool frame_crc_ok(const uint8_t *buf, size_t len);

/**
 * Validate a Valence data response: length, function/byte-count header, CRC.
 * Expected: [bus_id, 0x03, 0x36, ...payload..., crc_lo, crc_hi]
 */
bool is_valid_data_frame(const uint8_t *frame, size_t len);

/**
 * Decode a validated (or at least DATA_FRAME_LEN) Valence data frame into engineering units.
 * Does not check CRC — call is_valid_data_frame() first when the wire CRC matters.
 */
bool decode_data_frame(const uint8_t *frame, size_t len, BatteryReading *out);

/** Pack power in watts: V × I1 (signed with current). NAN if either input is NAN. */
float battery_power_w(const BatteryReading &reading);

/**
 * Parse bridge ASCII line: B,idx,soc,u,i1,i2,t1,tpcb,u1,u2,u3,u4
 * On success, *out_index is 0-based battery index and *out is filled with valid=true.
 */
bool parse_b_line(const char *line, uint8_t max_batteries, uint8_t *out_index, BatteryReading *out);

}  // namespace valence_rt
}  // namespace esphome
