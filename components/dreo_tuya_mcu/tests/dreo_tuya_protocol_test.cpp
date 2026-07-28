#include "dreo_tuya_protocol.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

using esphome::dreo_tuya_mcu::DpValue;
using esphome::dreo_tuya_mcu::append_checksum;
using esphome::dreo_tuya_mcu::build_dp_write_frame;
using esphome::dreo_tuya_mcu::build_time_payload;
using esphome::dreo_tuya_mcu::decode_dp_tlv;
using esphome::dreo_tuya_mcu::frame_checksum;

TEST(FrameChecksum, HeartbeatAndQueryFrames) {
  const uint8_t heartbeat[] = {0x55, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  EXPECT_EQ(frame_checksum(heartbeat, sizeof(heartbeat)), 0xFF);

  const uint8_t query[] = {0x55, 0xAA, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00};
  EXPECT_EQ(frame_checksum(query, sizeof(query)), 0x07);
}

TEST(AppendChecksum, AddsValidByte) {
  std::vector<uint8_t> frame = {0x55, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  append_checksum(frame);
  ASSERT_EQ(frame.size(), 9u);
  EXPECT_EQ(frame.back(), 0xFF);
}

TEST(DecodeDpTlv, BoolEnumAndValue) {
  const uint8_t bool_on[] = {0x01, 0x01, 0x00, 0x01, 0x01};
  DpValue dp{};
  EXPECT_EQ(decode_dp_tlv(bool_on, sizeof(bool_on), &dp), 5u);
  EXPECT_EQ(dp.id, 1);
  EXPECT_EQ(dp.type, 0x01);
  EXPECT_EQ(dp.value, 1u);

  const uint8_t enum_speed[] = {0x04, 0x04, 0x00, 0x01, 0x03};
  EXPECT_EQ(decode_dp_tlv(enum_speed, sizeof(enum_speed), &dp), 5u);
  EXPECT_EQ(dp.id, 4);
  EXPECT_EQ(dp.type, 0x04);
  EXPECT_EQ(dp.value, 3u);

  const uint8_t timer[] = {0x06, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x78};
  EXPECT_EQ(decode_dp_tlv(timer, sizeof(timer), &dp), 8u);
  EXPECT_EQ(dp.id, 6);
  EXPECT_EQ(dp.type, 0x02);
  EXPECT_EQ(dp.value, 120u);
}

TEST(DecodeDpTlv, RejectsInvalid) {
  DpValue dp{};
  EXPECT_EQ(decode_dp_tlv(nullptr, 4, &dp), 0u);
  EXPECT_EQ(decode_dp_tlv(nullptr, 4, nullptr), 0u);

  const uint8_t short_buf[] = {0x01, 0x01};
  EXPECT_EQ(decode_dp_tlv(short_buf, sizeof(short_buf), &dp), 0u);

  const uint8_t bad_len[] = {0x01, 0x01, 0x00, 0x02, 0x01};
  EXPECT_EQ(decode_dp_tlv(bad_len, sizeof(bad_len), &dp), 0u);
}

TEST(BuildDpWriteFrame, BoolPowerOn) {
  auto frame = build_dp_write_frame(1, 0x01, 1);
  ASSERT_EQ(frame.size(), 14u);
  EXPECT_EQ(frame[0], 0x55);
  EXPECT_EQ(frame[1], 0xAA);
  EXPECT_EQ(frame[4], 0x06);
  EXPECT_EQ(frame[7], 0x05);
  EXPECT_EQ(frame[8], 0x01);
  EXPECT_EQ(frame[9], 0x01);
  EXPECT_EQ(frame[12], 0x01);
  EXPECT_EQ(frame.back(), frame_checksum(frame.data(), frame.size() - 1));
}

TEST(BuildDpWriteFrame, EnumSpeed) {
  auto frame = build_dp_write_frame(4, 0x04, 5);
  EXPECT_EQ(frame[7], 0x05);
  EXPECT_EQ(frame[8], 4);
  EXPECT_EQ(frame[9], 0x04);
  EXPECT_EQ(frame[12], 5);
  EXPECT_EQ(frame.back(), frame_checksum(frame.data(), frame.size() - 1));
}

TEST(BuildDpWriteFrame, TimerValue) {
  auto frame = build_dp_write_frame(6, 0x02, 480);
  ASSERT_EQ(frame.size(), 17u);
  EXPECT_EQ(frame[7], 0x08);
  EXPECT_EQ(frame[8], 6);
  EXPECT_EQ(frame[9], 0x02);
  EXPECT_EQ(frame[10], 0x00);
  EXPECT_EQ(frame[11], 0x04);
  EXPECT_EQ(frame[12], 0x00);
  EXPECT_EQ(frame[13], 0x00);
  EXPECT_EQ(frame[14], 0x01);
  EXPECT_EQ(frame[15], 0xE0);
  EXPECT_EQ(frame.back(), frame_checksum(frame.data(), frame.size() - 1));
}

TEST(BuildTimePayload, EncodesFields) {
  auto payload = build_time_payload(26, 7, 28, 11, 5, 30, 2);
  ASSERT_EQ(payload.size(), 9u);
  EXPECT_EQ(payload[0], 0x01);
  EXPECT_EQ(payload[1], 26);
  EXPECT_EQ(payload[2], 7);
  EXPECT_EQ(payload[3], 28);
  EXPECT_EQ(payload[4], 11);
  EXPECT_EQ(payload[5], 5);
  EXPECT_EQ(payload[6], 30);
  EXPECT_EQ(payload[7], 2);
  EXPECT_EQ(payload[8], 0x00);
}
