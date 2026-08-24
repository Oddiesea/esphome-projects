#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace esphome {
namespace pixel_layout {

class PixelLayout;

#ifdef USE_PIXEL_LAYOUT_SD_STORAGE

class SdStorageManager {
 public:
  void configure(const std::string &mount_path, const std::string &root_path, int clk_pin, int cmd_pin, int d0_pin,
                   uint16_t upload_port);
  void set_layout(PixelLayout *layout) { this->layout_ = layout; }
  void setup();
  void loop();
  bool ensure_mounted();
  bool mounted() const { return this->mounted_; }
  bool apply_bundle(const uint8_t *data, size_t len, std::string *err);
  bool export_bundle(std::vector<uint8_t> *out, std::string *err);
  bool reload_layout(std::string *err);
  bool use_sd_layout() const { return this->use_sd_; }
  void set_use_sd_layout(bool on) { this->use_sd_ = on; }
  const std::string &status() const { return this->status_; }
  void set_status(const std::string &s) { this->status_ = s; }

 protected:
  bool mount_();
  bool write_tree_(const std::string &dest_root, const uint8_t *data, size_t len, std::string *err);

  std::string mount_path_{"/sdcard"};
  std::string root_path_{"/sdcard/pixel_layout"};
  int clk_pin_{1};
  int cmd_pin_{44};
  int d0_pin_{17};
  uint16_t upload_port_{8080};
  bool mounted_{false};
  bool use_sd_{true};
  std::string status_{"sd idle"};
  void *http_{nullptr};
  PixelLayout *layout_{nullptr};
};

#endif

}  // namespace pixel_layout
}  // namespace esphome
