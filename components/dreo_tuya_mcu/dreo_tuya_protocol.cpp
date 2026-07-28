#include "dreo_tuya_protocol.h"

namespace esphome {
namespace dreo_tuya_mcu {

uint8_t frame_checksum(const uint8_t *data, size_t len) {
  uint32_t sum = 0;
  for (size_t i = 0; i < len; i++)
    sum += data[i];
  return static_cast<uint8_t>(sum & 0xFF);
}

void append_checksum(std::vector<uint8_t> &frame) {
  frame.push_back(frame_checksum(frame.data(), frame.size()));
}

size_t decode_dp_tlv(const uint8_t *data, size_t len, DpValue *out) {
  if (data == nullptr || out == nullptr || len < 4)
    return 0;

  uint8_t did = data[0];
  uint8_t dty = data[1];
  uint16_t dlen = (static_cast<uint16_t>(data[2]) << 8) | data[3];
  if (len < 4 + dlen)
    return 0;

  uint32_t val = 0;
  if (dty == 0x01 && dlen == 1) {
    val = data[4];
  } else if (dty == 0x02 && dlen == 4) {
    val = (static_cast<uint32_t>(data[4]) << 24) | (static_cast<uint32_t>(data[5]) << 16) |
          (static_cast<uint32_t>(data[6]) << 8) | data[7];
  } else if (dty == 0x04 && dlen == 1) {
    val = data[4];
  } else {
    return 0;
  }

  out->id = did;
  out->type = dty;
  out->value = val;
  return 4 + dlen;
}

std::vector<uint8_t> build_dp_write_frame(uint8_t dp_id, uint8_t dp_type, uint32_t value) {
  std::vector<uint8_t> frame = {0x55, 0xAA, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00};
  frame.push_back(dp_id);
  frame.push_back(dp_type);
  if (dp_type == 0x02) {
    frame.push_back(0x00);
    frame.push_back(0x04);
    frame.push_back((value >> 24) & 0xFF);
    frame.push_back((value >> 16) & 0xFF);
    frame.push_back((value >> 8) & 0xFF);
    frame.push_back(value & 0xFF);
    frame[7] = 0x08;
  } else {
    frame.push_back(0x00);
    frame.push_back(0x01);
    frame.push_back(value & 0xFF);
    frame[7] = 0x05;
  }
  append_checksum(frame);
  return frame;
}

std::vector<uint8_t> build_time_payload(uint8_t year_2digit, uint8_t month, uint8_t day, uint8_t hour,
                                        uint8_t minute, uint8_t second, uint8_t day_of_week) {
  return {0x01, year_2digit, month, day, hour, minute, second, day_of_week, 0x00};
}

}  // namespace dreo_tuya_mcu
}  // namespace esphome
