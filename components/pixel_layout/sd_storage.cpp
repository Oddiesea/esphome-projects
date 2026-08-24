#include "sd_storage.h"

#ifdef USE_PIXEL_LAYOUT_SD_STORAGE

#include "pixel_layout.h"

#include "esphome/core/log.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include "esp_http_server.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"

static const char *const TAG = "pixel_layout.sd";

namespace esphome {
namespace pixel_layout {

namespace {

static SdStorageManager *g_sd_instance{nullptr};

bool mkdir_p(const std::string &path) {
  if (path.empty())
    return false;
  std::string cur;
  cur.reserve(path.size());
  for (size_t i = 0; i < path.size(); i++) {
    cur.push_back(path[i]);
    if (path[i] == '/' && cur.size() > 1) {
      ::mkdir(cur.c_str(), 0755);
    }
  }
  return ::mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
}

void set_cors_(httpd_req_t *req) {
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Authorization");
}

esp_err_t bundle_options_handler(httpd_req_t *req) {
  set_cors_(req);
  httpd_resp_set_status(req, "204 No Content");
  httpd_resp_send(req, nullptr, 0);
  return ESP_OK;
}

esp_err_t upload_post_handler(httpd_req_t *req) {
  set_cors_(req);
  if (g_sd_instance == nullptr) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "sd not ready");
    return ESP_FAIL;
  }
  std::string body;
  body.resize(req->content_len);
  size_t off = 0;
  while (off < body.size()) {
    int r = httpd_req_recv(req, body.data() + off, body.size() - off);
    if (r <= 0) {
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "read body");
      return ESP_FAIL;
    }
    off += size_t(r);
  }
  std::string err;
  if (!g_sd_instance->apply_bundle(reinterpret_cast<const uint8_t *>(body.data()), body.size(), &err)) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, err.c_str());
    return ESP_FAIL;
  }
  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, "{\"ok\":true}");
  return ESP_OK;
}

esp_err_t download_get_handler(httpd_req_t *req) {
  set_cors_(req);
  if (g_sd_instance == nullptr) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "sd not ready");
    return ESP_FAIL;
  }
  std::vector<uint8_t> blob;
  std::string err;
  if (!g_sd_instance->export_bundle(&blob, &err)) {
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, err.c_str());
    return ESP_FAIL;
  }
  httpd_resp_set_type(req, "application/octet-stream");
  httpd_resp_send(req, reinterpret_cast<const char *>(blob.data()), blob.size());
  return ESP_OK;
}

}  // namespace

void SdStorageManager::configure(const std::string &mount_path, const std::string &root_path, int clk_pin, int cmd_pin,
                                 int d0_pin, uint16_t upload_port) {
  this->mount_path_ = mount_path;
  this->root_path_ = root_path;
  this->clk_pin_ = clk_pin;
  this->cmd_pin_ = cmd_pin;
  this->d0_pin_ = d0_pin;
  this->upload_port_ = upload_port;
}

bool SdStorageManager::mount_() {
  if (this->mounted_)
    return true;
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.flags = SDMMC_HOST_FLAG_1BIT;
  host.max_freq_khz = SDMMC_FREQ_DEFAULT;

  sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
  slot.clk = gpio_num_t(this->clk_pin_);
  slot.cmd = gpio_num_t(this->cmd_pin_);
  slot.d0 = gpio_num_t(this->d0_pin_);
  slot.width = 1;

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024,
  };
  sdmmc_card_t *card = nullptr;
  esp_err_t ret = esp_vfs_fat_sdmmc_mount(this->mount_path_.c_str(), &host, &slot, &mount_config, &card);
  if (ret != ESP_OK) {
    this->status_ = std::string("mount failed: ") + esp_err_to_name(ret);
    ESP_LOGE(TAG, "SD mount failed: %s", esp_err_to_name(ret));
    return false;
  }
  this->mounted_ = true;
  this->status_ = "sd mounted";
  ESP_LOGI(TAG, "SD mounted at %s", this->mount_path_.c_str());
  return true;
}

void SdStorageManager::setup() {
  g_sd_instance = this;
  if (!this->mount_()) {
    return;
  }
  mkdir_p(this->root_path_);
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = this->upload_port_;
  config.uri_match_fn = httpd_uri_match_wildcard;
  httpd_handle_t server = nullptr;
  if (httpd_start(&server, &config) != ESP_OK) {
    this->status_ = "http start failed";
    return;
  }
  httpd_uri_t upload_uri = {
      .uri = "/pixel_layout/bundle",
      .method = HTTP_POST,
      .handler = upload_post_handler,
      .user_ctx = nullptr,
  };
  httpd_register_uri_handler(server, &upload_uri);
  httpd_uri_t download_uri = {
      .uri = "/pixel_layout/bundle",
      .method = HTTP_GET,
      .handler = download_get_handler,
      .user_ctx = nullptr,
  };
  httpd_register_uri_handler(server, &download_uri);
  httpd_uri_t options_uri = {
      .uri = "/pixel_layout/bundle",
      .method = HTTP_OPTIONS,
      .handler = bundle_options_handler,
      .user_ctx = nullptr,
  };
  httpd_register_uri_handler(server, &options_uri);
  this->http_ = server;
  this->status_ = "sd ready";
}

void SdStorageManager::loop() {}

bool SdStorageManager::ensure_mounted() { return this->mount_(); }

bool SdStorageManager::write_tree_(const std::string &dest_root, const uint8_t *data, size_t len, std::string *err) {
  if (len < 8 || std::memcmp(data, "PLB1", 4) != 0) {
    if (err)
      *err = "bad plbundle magic";
    return false;
  }
  uint16_t version = data[4] | (uint16_t(data[5]) << 8);
  uint16_t count = data[6] | (uint16_t(data[7]) << 8);
  if (version != 1) {
    if (err)
      *err = "unsupported plbundle version";
    return false;
  }
  if (!mkdir_p(dest_root)) {
    if (err)
      *err = "mkdir root failed";
    return false;
  }
  size_t pos = 8;
  for (uint16_t i = 0; i < count; i++) {
    if (pos + 2 > len) {
      if (err)
        *err = "truncated plbundle";
      return false;
    }
    uint16_t path_len = data[pos] | (uint16_t(data[pos + 1]) << 8);
    pos += 2;
    if (pos + path_len + 4 > len) {
      if (err)
        *err = "truncated plbundle path";
      return false;
    }
    std::string rel(reinterpret_cast<const char *>(data + pos), path_len);
    pos += path_len;
    uint32_t data_len = data[pos] | (uint32_t(data[pos + 1]) << 8) | (uint32_t(data[pos + 2]) << 16) |
                        (uint32_t(data[pos + 3]) << 24);
    pos += 4;
    if (pos + data_len > len) {
      if (err)
        *err = "truncated plbundle data";
      return false;
    }
    std::string full = dest_root + "/" + rel;
    auto slash = full.find_last_of('/');
    if (slash != std::string::npos)
      mkdir_p(full.substr(0, slash));
    FILE *f = fopen(full.c_str(), "wb");
    if (f == nullptr) {
      if (err)
        *err = "write failed: " + rel;
      return false;
    }
    if (data_len > 0 && fwrite(data + pos, 1, data_len, f) != data_len) {
      fclose(f);
      if (err)
        *err = "short write: " + rel;
      return false;
    }
    fclose(f);
    pos += data_len;
  }
  if (pos != len) {
    if (err)
      *err = "trailing plbundle bytes";
    return false;
  }
  return true;
}

namespace {

struct BundleEntry {
  std::string rel;
  std::vector<uint8_t> data;
};

bool read_file_vec_(const std::string &path, std::vector<uint8_t> *out) {
  FILE *f = fopen(path.c_str(), "rb");
  if (f == nullptr)
    return false;
  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return false;
  }
  long sz = ftell(f);
  if (sz < 0) {
    fclose(f);
    return false;
  }
  if (fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return false;
  }
  out->resize(static_cast<size_t>(sz));
  if (sz > 0 && fread(out->data(), 1, out->size(), f) != out->size()) {
    fclose(f);
    return false;
  }
  fclose(f);
  return true;
}

void collect_tree_(const std::string &root, const std::string &rel, std::vector<BundleEntry> *out) {
  const std::string dir = rel.empty() ? root : root + "/" + rel;
  DIR *d = opendir(dir.c_str());
  if (d == nullptr)
    return;
  struct dirent *ent;
  while ((ent = readdir(d)) != nullptr) {
    if (ent->d_name[0] == '.')
      continue;
    const std::string name = ent->d_name;
    const std::string child_rel = rel.empty() ? name : rel + "/" + name;
    const std::string full = root + "/" + child_rel;
    struct stat st;
    if (stat(full.c_str(), &st) != 0)
      continue;
    if (S_ISDIR(st.st_mode)) {
      collect_tree_(root, child_rel, out);
      continue;
    }
    BundleEntry entry;
    entry.rel = child_rel;
    if (read_file_vec_(full, &entry.data))
      out->push_back(std::move(entry));
  }
  closedir(d);
}

bool build_plbundle_(const std::vector<BundleEntry> &items, std::vector<uint8_t> *out) {
  std::vector<const BundleEntry *> sorted;
  sorted.reserve(items.size());
  for (const auto &item : items)
    sorted.push_back(&item);
  std::sort(sorted.begin(), sorted.end(), [](const BundleEntry *a, const BundleEntry *b) { return a->rel < b->rel; });
  size_t total = 8;
  for (const BundleEntry *item : sorted) {
    if (item->rel.size() > 65535)
      return false;
    total += 2 + item->rel.size() + 4 + item->data.size();
  }
  out->assign(total, 0);
  (*out)[0] = 'P';
  (*out)[1] = 'L';
  (*out)[2] = 'B';
  (*out)[3] = '1';
  (*out)[4] = 1;
  (*out)[6] = static_cast<uint8_t>(sorted.size() & 0xFF);
  (*out)[7] = static_cast<uint8_t>((sorted.size() >> 8) & 0xFF);
  size_t pos = 8;
  for (const BundleEntry *item : sorted) {
    const uint16_t path_len = static_cast<uint16_t>(item->rel.size());
    (*out)[pos++] = static_cast<uint8_t>(path_len & 0xFF);
    (*out)[pos++] = static_cast<uint8_t>((path_len >> 8) & 0xFF);
    std::memcpy(out->data() + pos, item->rel.data(), path_len);
    pos += path_len;
    const uint32_t data_len = static_cast<uint32_t>(item->data.size());
    (*out)[pos++] = static_cast<uint8_t>(data_len & 0xFF);
    (*out)[pos++] = static_cast<uint8_t>((data_len >> 8) & 0xFF);
    (*out)[pos++] = static_cast<uint8_t>((data_len >> 16) & 0xFF);
    (*out)[pos++] = static_cast<uint8_t>((data_len >> 24) & 0xFF);
    if (data_len > 0) {
      std::memcpy(out->data() + pos, item->data.data(), data_len);
      pos += data_len;
    }
  }
  return pos == total;
}

}  // namespace

bool SdStorageManager::export_bundle(std::vector<uint8_t> *out, std::string *err) {
  if (out == nullptr) {
    if (err)
      *err = "no output buffer";
    return false;
  }
  out->clear();
  if (!this->ensure_mounted()) {
    if (err)
      *err = this->status_;
    return false;
  }
  std::vector<BundleEntry> items;
  collect_tree_(this->root_path_, "", &items);
  if (items.empty()) {
    if (err)
      *err = "sd layout folder empty";
    return false;
  }
  bool has_playlist = false;
  for (const auto &item : items) {
    if (item.rel == "playlist.yml" || item.rel == "playlist.json")
      has_playlist = true;
  }
  if (!has_playlist) {
    if (err)
      *err = "playlist missing on sd";
    return false;
  }
  if (!build_plbundle_(items, out)) {
    if (err)
      *err = "plbundle build failed";
    return false;
  }
  if (err)
    err->clear();
  return true;
}

bool SdStorageManager::apply_bundle(const uint8_t *data, size_t len, std::string *err) {
  if (!this->ensure_mounted()) {
    if (err)
      *err = this->status_;
    return false;
  }
  if (!this->write_tree_(this->root_path_, data, len, err))
    return false;
  std::string reload_err;
  this->reload_layout(&reload_err);
  this->status_ = reload_err.empty() ? "bundle applied" : reload_err;
  return true;
}

bool SdStorageManager::reload_layout(std::string *err) {
  if (!this->use_sd_) {
    if (this->layout_ != nullptr)
      this->layout_->restore_progmem_playlist(err);
    else if (err)
      err->clear();
    this->status_ = "using progmem layout";
    return true;
  }
  if (!this->ensure_mounted()) {
    if (err)
      *err = this->status_;
    return false;
  }
  std::string manifest_path = this->root_path_ + "/manifest.json";
  FILE *f = fopen(manifest_path.c_str(), "rb");
  if (f == nullptr) {
    if (err)
      *err = "manifest missing on sd";
    return false;
  }
  fclose(f);
  if (this->layout_ == nullptr) {
    if (err)
      *err = "layout not linked";
    return false;
  }
  if (!this->layout_->load_playlist_from_sd(this->root_path_, err))
    return false;
  this->status_ = err != nullptr && !err->empty() ? *err : "sd layout loaded";
  return true;
}

}  // namespace pixel_layout
}  // namespace esphome

#endif
