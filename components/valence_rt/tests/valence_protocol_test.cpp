#include "valence_protocol.h"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

using esphome::valence_rt::BatteryReading;
using esphome::valence_rt::DATA_FRAME_LEN;
using esphome::valence_rt::battery_power_w;
using esphome::valence_rt::decode_data_frame;
using esphome::valence_rt::frame_crc_ok;
using esphome::valence_rt::is_valid_data_frame;
using esphome::valence_rt::modbus_crc16;
using esphome::valence_rt::parse_b_line;

namespace {

void write_le_u16(uint8_t *p, uint16_t value) {
  p[0] = static_cast<uint8_t>(value & 0xFF);
  p[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

void write_le_i16(uint8_t *p, int16_t value) {
  write_le_u16(p, static_cast<uint16_t>(value));
}

void append_crc(uint8_t *frame, size_t payload_len) {
  uint16_t crc = modbus_crc16(frame, payload_len);
  frame[payload_len] = static_cast<uint8_t>(crc & 0xFF);
  frame[payload_len + 1] = static_cast<uint8_t>((crc >> 8) & 0xFF);
}

std::array<uint8_t, DATA_FRAME_LEN> make_data_frame(float voltage_v, float current_a, float current_2_a, float soc_pct,
                                                    uint8_t temp_c, uint8_t pcb_temp_c, float cell1, float cell2,
                                                    float cell3, float cell4) {
  std::array<uint8_t, DATA_FRAME_LEN> frame{};
  frame[0] = 0x02;  // bus id
  frame[1] = 0x03;
  frame[2] = 0x36;
  frame[3] = temp_c;
  frame[17] = pcb_temp_c;
  write_le_i16(&frame[23], static_cast<int16_t>(std::lround(current_a * 1000.0f)));
  write_le_i16(&frame[27], static_cast<int16_t>(std::lround(current_2_a * 1000.0f)));
  write_le_u16(&frame[35], static_cast<uint16_t>(std::lround(voltage_v * 1000.0f)));
  frame[43] = static_cast<uint8_t>(std::lround(soc_pct / 100.0f * 255.0f));
  write_le_u16(&frame[49], static_cast<uint16_t>(std::lround(cell1 * 1000.0f)));
  write_le_u16(&frame[51], static_cast<uint16_t>(std::lround(cell2 * 1000.0f)));
  write_le_u16(&frame[53], static_cast<uint16_t>(std::lround(cell3 * 1000.0f)));
  write_le_u16(&frame[55], static_cast<uint16_t>(std::lround(cell4 * 1000.0f)));
  append_crc(frame.data(), DATA_FRAME_LEN - 2);
  return frame;
}

}  // namespace

TEST(ModbusCrc, KnownVector) {
  const uint8_t empty = 0;
  EXPECT_EQ(modbus_crc16(&empty, 0), 0xFFFF);

  const uint8_t payload[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x0A};
  uint16_t crc = modbus_crc16(payload, sizeof(payload));
  uint8_t frame[8];
  std::memcpy(frame, payload, sizeof(payload));
  frame[6] = static_cast<uint8_t>(crc & 0xFF);
  frame[7] = static_cast<uint8_t>((crc >> 8) & 0xFF);
  EXPECT_TRUE(frame_crc_ok(frame, sizeof(frame)));

  frame[6] ^= 0xFF;
  EXPECT_FALSE(frame_crc_ok(frame, sizeof(frame)));
}

TEST(DataFrame, DecodesEngineeringUnits) {
  auto frame = make_data_frame(/*voltage*/ 13.280f, /*i1*/ 2.500f, /*i2*/ -1.250f, /*soc*/ 80.0f,
                               /*temp*/ 25, /*pcb*/ 27, /*cells*/ 3.320f, 3.321f, 3.319f, 3.320f);

  EXPECT_TRUE(is_valid_data_frame(frame.data(), frame.size()));

  BatteryReading reading{};
  ASSERT_TRUE(decode_data_frame(frame.data(), frame.size(), &reading));
  EXPECT_TRUE(reading.valid);
  EXPECT_NEAR(reading.voltage, 13.280f, 0.001f);
  EXPECT_NEAR(reading.current, 2.500f, 0.001f);
  EXPECT_NEAR(reading.current_2, -1.250f, 0.001f);
  EXPECT_NEAR(reading.soc, 80.0f, 0.5f);
  EXPECT_FLOAT_EQ(reading.temperature, 25.0f);
  EXPECT_FLOAT_EQ(reading.temperature_pcb, 27.0f);
  EXPECT_NEAR(reading.cell_voltage_1, 3.320f, 0.001f);
  EXPECT_NEAR(reading.cell_voltage_2, 3.321f, 0.001f);
  EXPECT_NEAR(reading.cell_voltage_3, 3.319f, 0.001f);
  EXPECT_NEAR(reading.cell_voltage_4, 3.320f, 0.001f);
  EXPECT_NEAR(battery_power_w(reading), 13.280f * 2.500f, 0.01f);
}

TEST(DataFrame, RejectsBadHeaderOrCrc) {
  auto frame = make_data_frame(13.0f, 1.0f, 0.0f, 50.0f, 20, 21, 3.2f, 3.2f, 3.2f, 3.2f);
  EXPECT_TRUE(is_valid_data_frame(frame.data(), frame.size()));

  frame[1] = 0x04;
  EXPECT_FALSE(is_valid_data_frame(frame.data(), frame.size()));

  frame = make_data_frame(13.0f, 1.0f, 0.0f, 50.0f, 20, 21, 3.2f, 3.2f, 3.2f, 3.2f);
  frame[DATA_FRAME_LEN - 1] ^= 0x01;
  EXPECT_FALSE(is_valid_data_frame(frame.data(), frame.size()));

  EXPECT_FALSE(decode_data_frame(frame.data(), 10, nullptr));
  BatteryReading reading{};
  EXPECT_FALSE(decode_data_frame(frame.data(), 10, &reading));
}

TEST(DataFrame, NegativeDischargePower) {
  auto frame = make_data_frame(12.800f, -5.000f, -4.900f, 40.0f, 22, 23, 3.2f, 3.2f, 3.2f, 3.2f);
  BatteryReading reading{};
  ASSERT_TRUE(decode_data_frame(frame.data(), frame.size(), &reading));
  EXPECT_NEAR(battery_power_w(reading), -64.0f, 0.05f);
}

TEST(BridgeLine, ParsesValidBLine) {
  uint8_t index = 255;
  BatteryReading reading{};
  ASSERT_TRUE(parse_b_line("B,1,81.2,13.201,1.100,-0.050,24,26,3.300,3.301,3.299,3.300", 4, &index, &reading));
  EXPECT_EQ(index, 0);
  EXPECT_TRUE(reading.valid);
  EXPECT_NEAR(reading.soc, 81.2f, 0.01f);
  EXPECT_NEAR(reading.voltage, 13.201f, 0.001f);
  EXPECT_NEAR(reading.current, 1.100f, 0.001f);
  EXPECT_NEAR(reading.current_2, -0.050f, 0.001f);
  EXPECT_FLOAT_EQ(reading.temperature, 24.0f);
  EXPECT_FLOAT_EQ(reading.temperature_pcb, 26.0f);
  EXPECT_NEAR(battery_power_w(reading), 13.201f * 1.100f, 0.01f);
}

TEST(BridgeLine, RejectsMalformedOrOutOfRange) {
  uint8_t index = 0;
  BatteryReading reading{};
  EXPECT_FALSE(parse_b_line("OK,1", 4, &index, &reading));
  EXPECT_FALSE(parse_b_line("B,1,81.2,13.2", 4, &index, &reading));
  EXPECT_FALSE(parse_b_line("B,5,81.2,13.201,1.1,0,24,26,3.3,3.3,3.3,3.3", 4, &index, &reading));
  EXPECT_FALSE(parse_b_line("B,0,81.2,13.201,1.1,0,24,26,3.3,3.3,3.3,3.3", 4, &index, &reading));
  EXPECT_FALSE(parse_b_line(nullptr, 4, &index, &reading));
}
