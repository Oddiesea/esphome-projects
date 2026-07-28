#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace esphome {
namespace dreo_tuya_mcu {

struct DpValue {
  uint8_t id{0};
  uint8_t type{0};
  uint32_t value{0};
};

/** Sum of all bytes & 0xFF (Tuya MCU checksum). */
uint8_t frame_checksum(const uint8_t *data, size_t len);

/** Append checksum byte to frame (checksum of all bytes currently in out). */
void append_checksum(std::vector<uint8_t> &frame);

/** Parse one DP TLV from DP report payload. Returns bytes consumed or 0 on error. */
size_t decode_dp_tlv(const uint8_t *data, size_t len, DpValue *out);

/** Build cmd 0x06 DP write frame (bool or enum: type 0x01/0x04, value: type 0x02). */
std::vector<uint8_t> build_dp_write_frame(uint8_t dp_id, uint8_t dp_type, uint32_t value);

/** Build cmd 0x0E time response payload (caller adds header + checksum). */
std::vector<uint8_t> build_time_payload(uint8_t year_2digit, uint8_t month, uint8_t day, uint8_t hour,
                                        uint8_t minute, uint8_t second, uint8_t day_of_week);

}  // namespace dreo_tuya_mcu
}  // namespace esphome
