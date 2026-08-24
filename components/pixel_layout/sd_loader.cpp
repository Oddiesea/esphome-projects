#include "pixel_layout.h"

#ifdef USE_PIXEL_LAYOUT_SD_STORAGE

#include "esphome/core/log.h"

#include <cJSON.h>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <memory>

#ifdef USE_ESP32
#include "mbedtls/sha256.h"
#endif

#ifdef USE_IMAGE
#include "esphome/components/image/image.h"
#endif

static const char *const TAG = "pixel_layout.sd_loader";

namespace esphome {
namespace pixel_layout {

#ifdef USE_IMAGE
SdOwnedImage::~SdOwnedImage() { delete this->image; }
#endif

namespace {

bool read_file(const std::string &path, std::string *out) {
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

bool sha256_hex(const std::string &data, std::string *hex) {
#ifdef USE_ESP32
  unsigned char hash[32];
  mbedtls_sha256(reinterpret_cast<const unsigned char *>(data.data()), data.size(), hash, 0);
  static const char *digits = "0123456789abcdef";
  hex->resize(64);
  for (size_t i = 0; i < 32; i++) {
    (*hex)[i * 2] = digits[hash[i] >> 4];
    (*hex)[i * 2 + 1] = digits[hash[i] & 0x0F];
  }
  return true;
#else
  (void) data;
  hex->clear();
  return false;
#endif
}

uint32_t parse_ms_value(const cJSON *node) {
  if (node == nullptr)
    return 0;
  if (cJSON_IsNumber(node))
    return static_cast<uint32_t>(node->valuedouble);
  if (!cJSON_IsString(node))
    return 0;
  const char *s = node->valuestring;
  if (s == nullptr || *s == 0)
    return 0;
  char *end = nullptr;
  double v = strtod(s, &end);
  if (end == s)
    return 0;
  while (*end == ' ')
    end++;
  if (strncmp(end, "ms", 2) == 0)
    return static_cast<uint32_t>(v);
  if (*end == 's' || *end == 0)
    return static_cast<uint32_t>(v * 1000.0);
  return static_cast<uint32_t>(v);
}

Color parse_color_json(const cJSON *node) {
  if (node == nullptr)
    return Color(255, 255, 255);
  if (cJSON_IsArray(node) && cJSON_GetArraySize(node) >= 3) {
    return Color(static_cast<uint8_t>(cJSON_GetArrayItem(node, 0)->valueint),
                 static_cast<uint8_t>(cJSON_GetArrayItem(node, 1)->valueint),
                 static_cast<uint8_t>(cJSON_GetArrayItem(node, 2)->valueint));
  }
  if (!cJSON_IsString(node))
    return Color(255, 255, 255);
  const char *s = node->valuestring;
  if (s == nullptr)
    return Color(255, 255, 255);
  if (strcasecmp(s, "white") == 0)
    return Color(255, 255, 255);
  if (strcasecmp(s, "black") == 0)
    return Color(0, 0, 0);
  if (s[0] == '#' && strlen(s) >= 7) {
    unsigned r = 0, g = 0, b = 0;
    if (sscanf(s + 1, "%2x%2x%2x", &r, &g, &b) == 3)
      return Color(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
  }
  return Color(255, 255, 255);
}

Outline parse_outline_json(const cJSON *node) {
  if (node == nullptr)
    return Outline::NONE;
  if (cJSON_IsBool(node))
    return cJSON_IsTrue(node) ? Outline::BLACK : Outline::NONE;
  if (!cJSON_IsString(node))
    return Outline::NONE;
  const char *s = node->valuestring;
  if (strcasecmp(s, "white") == 0)
    return Outline::WHITE;
  if (strcasecmp(s, "black") == 0)
    return Outline::BLACK;
  return Outline::NONE;
}

AlignMain parse_main_align(const char *s) {
  if (s == nullptr || strcasecmp(s, "start") == 0)
    return AlignMain::START;
  if (strcasecmp(s, "center") == 0)
    return AlignMain::CENTER;
  if (strcasecmp(s, "end") == 0)
    return AlignMain::END;
  if (strcasecmp(s, "space_between") == 0 || strcasecmp(s, "space-between") == 0)
    return AlignMain::SPACE_BETWEEN;
  return AlignMain::START;
}

AlignCross parse_cross_align(const char *s) {
  if (s == nullptr || strcasecmp(s, "start") == 0)
    return AlignCross::START;
  if (strcasecmp(s, "center") == 0)
    return AlignCross::CENTER;
  if (strcasecmp(s, "end") == 0)
    return AlignCross::END;
  return AlignCross::CENTER;
}

IconAlign parse_icon_align(const char *s) {
  if (s == nullptr || strcasecmp(s, "middle") == 0)
    return IconAlign::MIDDLE;
  if (strcasecmp(s, "top") == 0)
    return IconAlign::TOP;
  if (strcasecmp(s, "bottom") == 0)
    return IconAlign::BOTTOM;
  return IconAlign::MIDDLE;
}

ClockFace parse_clock_face(const char *s) {
  if (s != nullptr && strcasecmp(s, "analog") == 0)
    return ClockFace::ANALOG;
  return ClockFace::DIGITAL;
}

ClockTheme parse_clock_theme(const char *s) {
  if (s == nullptr)
    return ClockTheme::SEVEN_SEGMENT;
  if (strcasecmp(s, "rounded") == 0)
    return ClockTheme::ROUNDED;
  if (strcasecmp(s, "block") == 0)
    return ClockTheme::BLOCK;
  if (strcasecmp(s, "tiny") == 0)
    return ClockTheme::TINY;
  if (strcasecmp(s, "typeface") == 0)
    return ClockTheme::TYPEFACE;
  if (strcasecmp(s, "split_flap") == 0)
    return ClockTheme::SPLIT_FLAP;
  if (strcasecmp(s, "perspective") == 0)
    return ClockTheme::PERSPECTIVE;
  if (strcasecmp(s, "ring") == 0)
    return ClockTheme::RING;
  if (strcasecmp(s, "minimal") == 0)
    return ClockTheme::MINIMAL;
  if (strcasecmp(s, "ticks") == 0)
    return ClockTheme::TICKS;
  if (strcasecmp(s, "square") == 0)
    return ClockTheme::SQUARE;
  return ClockTheme::SEVEN_SEGMENT;
}

ClockSize parse_clock_size(const char *s) {
  if (s == nullptr)
    return ClockSize::MD;
  if (strcasecmp(s, "sm") == 0)
    return ClockSize::SM;
  if (strcasecmp(s, "lg") == 0)
    return ClockSize::LG;
  return ClockSize::MD;
}

DateStyle parse_date_style(const char *s) {
  if (s == nullptr)
    return DateStyle::TEXT;
  if (strcasecmp(s, "two_line") == 0)
    return DateStyle::TWO_LINE;
  if (strcasecmp(s, "calendar") == 0)
    return DateStyle::CALENDAR;
  return DateStyle::TEXT;
}

TextStyle parse_text_style(const char *s) {
  if (s != nullptr && strcasecmp(s, "two_line") == 0)
    return TextStyle::TWO_LINE;
  return TextStyle::TEXT;
}

BoxShape parse_box_shape(const char *s) {
  if (s == nullptr)
    return BoxShape::RECT;
  if (strcasecmp(s, "rounded") == 0)
    return BoxShape::ROUNDED;
  if (strcasecmp(s, "oval") == 0)
    return BoxShape::OVAL;
  if (strcasecmp(s, "pill") == 0)
    return BoxShape::PILL;
  if (strcasecmp(s, "triangle") == 0)
    return BoxShape::TRIANGLE;
  if (strcasecmp(s, "diamond") == 0)
    return BoxShape::DIAMOND;
  if (strcasecmp(s, "plus") == 0)
    return BoxShape::PLUS;
  if (strcasecmp(s, "frame") == 0)
    return BoxShape::FRAME;
  if (strcasecmp(s, "ring") == 0)
    return BoxShape::RING;
  if (strcasecmp(s, "line") == 0)
    return BoxShape::LINE;
  return BoxShape::RECT;
}

BoxPoint parse_box_point(const char *s) {
  if (s == nullptr)
    return BoxPoint::UP;
  if (strcasecmp(s, "down") == 0)
    return BoxPoint::DOWN;
  if (strcasecmp(s, "left") == 0)
    return BoxPoint::LEFT;
  if (strcasecmp(s, "right") == 0)
    return BoxPoint::RIGHT;
  return BoxPoint::UP;
}

WeatherTextPosition parse_weather_text_pos(const char *s) {
  if (s == nullptr)
    return WeatherTextPosition::END;
  if (strcasecmp(s, "start") == 0)
    return WeatherTextPosition::START;
  if (strcasecmp(s, "below") == 0)
    return WeatherTextPosition::BELOW;
  if (strcasecmp(s, "above") == 0)
    return WeatherTextPosition::ABOVE;
  return WeatherTextPosition::END;
}

}  // namespace

class SdWidgetBuilder {
 public:
  SdWidgetBuilder(PixelLayout *host, const std::string &root_path, std::vector<std::unique_ptr<Widget>> *owned,
                  std::vector<SdOwnedImage> *images)
      : host_(host), root_path_(root_path), owned_(owned), images_(images) {}

  Widget *build(const cJSON *node) {
    if (node == nullptr || !cJSON_IsObject(node))
      return nullptr;
    const cJSON *type = cJSON_GetObjectItem(node, "type");
    if (!cJSON_IsString(type))
      return nullptr;
    const char *t = type->valuestring;
    Widget *w = nullptr;
    if (strcmp(t, "stack") == 0)
      w = this->build_stack_(node);
    else if (strcmp(t, "row") == 0)
      w = this->build_row_(node);
    else if (strcmp(t, "column") == 0)
      w = this->build_column_(node);
    else if (strcmp(t, "box") == 0 || strcmp(t, "shape") == 0)
      w = this->build_box_(node);
    else if (strcmp(t, "text") == 0 || strcmp(t, "sensor") == 0)
      w = this->build_text_(node);
    else if (strcmp(t, "icon") == 0)
      w = this->build_icon_(node);
    else if (strcmp(t, "clock") == 0)
      w = this->build_clock_(node);
    else if (strcmp(t, "date") == 0)
      w = this->build_date_(node);
    else if (strcmp(t, "weather") == 0)
      w = this->build_weather_(node);
    else if (strcmp(t, "sprite") == 0)
      w = this->build_sprite_(node);
    else if (strcmp(t, "custom") == 0)
      w = this->build_custom_(node);
    if (w == nullptr)
      return nullptr;
    this->apply_common_(w, node);
    return w;
  }

 protected:
  Widget *own_(Widget *raw) {
    this->owned_->emplace_back(raw);
    return raw;
  }

  void apply_common_(Widget *w, const cJSON *node) {
    const cJSON *v = cJSON_GetObjectItem(node, "x");
    if (cJSON_IsNumber(v))
      w->set_x(v->valueint);
    v = cJSON_GetObjectItem(node, "y");
    if (cJSON_IsNumber(v))
      w->set_y(v->valueint);
    v = cJSON_GetObjectItem(node, "width");
    if (cJSON_IsNumber(v))
      w->set_width(v->valueint);
    v = cJSON_GetObjectItem(node, "height");
    if (cJSON_IsNumber(v))
      w->set_height(v->valueint);
    v = cJSON_GetObjectItem(node, "opacity");
    if (cJSON_IsNumber(v))
      w->set_opacity(static_cast<uint8_t>(v->valueint));
    v = cJSON_GetObjectItem(node, "expanded");
    if (cJSON_IsBool(v))
      w->set_expanded(cJSON_IsTrue(v));
    v = cJSON_GetObjectItem(node, "outline");
    w->set_outline(parse_outline_json(v));
  }

  void apply_text_common_(TextWidget *w, const cJSON *node) {
    const cJSON *v = cJSON_GetObjectItem(node, "text");
    if (cJSON_IsString(v))
      w->set_text(v->valuestring);
    v = cJSON_GetObjectItem(node, "format");
    if (cJSON_IsString(v))
      w->set_format(v->valuestring);
    v = cJSON_GetObjectItem(node, "unit");
    if (cJSON_IsString(v))
      w->set_unit(v->valuestring);
    v = cJSON_GetObjectItem(node, "label");
    if (cJSON_IsString(v))
      w->set_caption(v->valuestring);
    v = cJSON_GetObjectItem(node, "style");
    if (cJSON_IsString(v))
      w->set_style(parse_text_style(v->valuestring));
    v = cJSON_GetObjectItem(node, "color");
    w->set_color(parse_color_json(v));
#ifdef USE_FONT
    v = cJSON_GetObjectItem(node, "font");
    if (cJSON_IsString(v))
      w->set_font(this->host_->lookup_font(v->valuestring));
    v = cJSON_GetObjectItem(node, "icon_font");
    if (cJSON_IsString(v))
      w->set_icon_font(this->host_->lookup_font(v->valuestring));
#endif
#ifdef USE_SENSOR
    v = cJSON_GetObjectItem(node, "sensor_id");
    if (cJSON_IsString(v))
      w->set_sensor(this->host_->lookup_sensor(v->valuestring));
#endif
#ifdef USE_TEXT_SENSOR
    v = cJSON_GetObjectItem(node, "text_sensor_id");
    if (cJSON_IsString(v))
      w->set_text_sensor(this->host_->lookup_text_sensor(v->valuestring));
#endif
  }

  Widget *build_stack_(const cJSON *node) {
    auto *w = new StackWidget();
    const cJSON *children = cJSON_GetObjectItem(node, "children");
    if (cJSON_IsArray(children)) {
      cJSON *child = nullptr;
      cJSON_ArrayForEach(child, children) {
        Widget *built = this->build(child);
        if (built != nullptr)
          w->add_child(built);
      }
    }
    return this->own_(w);
  }

  Widget *build_row_(const cJSON *node) {
    auto *w = new RowWidget();
    const cJSON *v = cJSON_GetObjectItem(node, "gap");
    if (cJSON_IsNumber(v))
      w->set_gap(v->valueint);
    v = cJSON_GetObjectItem(node, "main_align");
    if (cJSON_IsString(v))
      w->set_main_align(parse_main_align(v->valuestring));
    v = cJSON_GetObjectItem(node, "cross_align");
    if (cJSON_IsString(v))
      w->set_cross_align(parse_cross_align(v->valuestring));
    const cJSON *children = cJSON_GetObjectItem(node, "children");
    if (cJSON_IsArray(children)) {
      cJSON *child = nullptr;
      cJSON_ArrayForEach(child, children) {
        Widget *built = this->build(child);
        if (built != nullptr)
          w->add_child(built);
      }
    }
    return this->own_(w);
  }

  Widget *build_column_(const cJSON *node) {
    auto *w = new ColumnWidget();
    const cJSON *v = cJSON_GetObjectItem(node, "gap");
    if (cJSON_IsNumber(v))
      w->set_gap(v->valueint);
    v = cJSON_GetObjectItem(node, "main_align");
    if (cJSON_IsString(v))
      w->set_main_align(parse_main_align(v->valuestring));
    v = cJSON_GetObjectItem(node, "cross_align");
    if (cJSON_IsString(v))
      w->set_cross_align(parse_cross_align(v->valuestring));
    const cJSON *children = cJSON_GetObjectItem(node, "children");
    if (cJSON_IsArray(children)) {
      cJSON *child = nullptr;
      cJSON_ArrayForEach(child, children) {
        Widget *built = this->build(child);
        if (built != nullptr)
          w->add_child(built);
      }
    }
    return this->own_(w);
  }

  Widget *build_box_(const cJSON *node) {
    auto *w = new BoxWidget();
    const cJSON *v = cJSON_GetObjectItem(node, "padding");
    if (cJSON_IsNumber(v))
      w->set_padding(v->valueint);
    v = cJSON_GetObjectItem(node, "kind");
    if (cJSON_IsString(v))
      w->set_shape(parse_box_shape(v->valuestring));
    v = cJSON_GetObjectItem(node, "point");
    if (cJSON_IsString(v))
      w->set_point(parse_box_point(v->valuestring));
    v = cJSON_GetObjectItem(node, "stroke");
    if (cJSON_IsNumber(v))
      w->set_stroke(v->valueint);
    v = cJSON_GetObjectItem(node, "radius");
    if (cJSON_IsNumber(v))
      w->set_radius(v->valueint);
    v = cJSON_GetObjectItem(node, "antialias");
    if (cJSON_IsBool(v))
      w->set_antialias(cJSON_IsTrue(v));
    v = cJSON_GetObjectItem(node, "fill");
    if (v != nullptr)
      w->set_fill(parse_color_json(v));
    else {
      v = cJSON_GetObjectItem(node, "color");
      if (v != nullptr)
        w->set_fill(parse_color_json(v));
    }
    v = cJSON_GetObjectItem(node, "child");
    if (v != nullptr) {
      Widget *child = this->build(v);
      if (child != nullptr)
        w->set_child(child);
    }
    return this->own_(w);
  }

  Widget *build_text_(const cJSON *node) {
    auto *w = new TextWidget();
    this->apply_text_common_(w, node);
    return this->own_(w);
  }

  Widget *build_icon_(const cJSON *node) {
    auto *w = new IconWidget();
    const cJSON *v = cJSON_GetObjectItem(node, "icon");
    if (cJSON_IsString(v))
      w->set_codepoint(v->valuestring);
    this->apply_text_common_(w, node);
    return this->own_(w);
  }

  Widget *build_clock_(const cJSON *node) {
    auto *w = new ClockWidget();
    const cJSON *v = cJSON_GetObjectItem(node, "face");
    if (cJSON_IsString(v))
      w->set_face(parse_clock_face(v->valuestring));
    v = cJSON_GetObjectItem(node, "theme");
    if (cJSON_IsString(v))
      w->set_theme(parse_clock_theme(v->valuestring));
    v = cJSON_GetObjectItem(node, "size");
    if (cJSON_IsString(v))
      w->set_size(parse_clock_size(v->valuestring));
    v = cJSON_GetObjectItem(node, "format");
    if (cJSON_IsString(v))
      w->set_format(v->valuestring);
    v = cJSON_GetObjectItem(node, "ghost");
    if (cJSON_IsBool(v))
      w->set_ghost(cJSON_IsTrue(v));
    v = cJSON_GetObjectItem(node, "show_seconds");
    if (cJSON_IsBool(v))
      w->set_show_seconds(cJSON_IsTrue(v));
    v = cJSON_GetObjectItem(node, "blink_colon");
    if (cJSON_IsBool(v))
      w->set_blink_colon(cJSON_IsTrue(v));
    v = cJSON_GetObjectItem(node, "color");
    w->set_color(parse_color_json(v));
#ifdef USE_TIME
    v = cJSON_GetObjectItem(node, "time_id");
    if (cJSON_IsString(v))
      w->set_time(this->host_->lookup_time(v->valuestring));
    v = cJSON_GetObjectItem(node, "fallback_time_id");
    if (cJSON_IsString(v))
      w->set_fallback_time(this->host_->lookup_time(v->valuestring));
#endif
#ifdef USE_FONT
    v = cJSON_GetObjectItem(node, "font");
    if (cJSON_IsString(v))
      w->set_font(this->host_->lookup_font(v->valuestring));
    v = cJSON_GetObjectItem(node, "icon_font");
    if (cJSON_IsString(v))
      w->set_icon_font(this->host_->lookup_font(v->valuestring));
#endif
    return this->own_(w);
  }

  Widget *build_date_(const cJSON *node) {
    auto *w = new DateWidget();
    const cJSON *v = cJSON_GetObjectItem(node, "style");
    if (cJSON_IsString(v))
      w->set_style(parse_date_style(v->valuestring));
    v = cJSON_GetObjectItem(node, "format");
    if (cJSON_IsString(v))
      w->set_format(v->valuestring);
    v = cJSON_GetObjectItem(node, "uppercase");
    if (cJSON_IsBool(v))
      w->set_uppercase(cJSON_IsTrue(v));
    v = cJSON_GetObjectItem(node, "show_year");
    if (cJSON_IsBool(v))
      w->set_show_year(cJSON_IsTrue(v));
    v = cJSON_GetObjectItem(node, "color");
    w->set_color(parse_color_json(v));
#ifdef USE_TIME
    v = cJSON_GetObjectItem(node, "time_id");
    if (cJSON_IsString(v))
      w->set_time(this->host_->lookup_time(v->valuestring));
#endif
#ifdef USE_FONT
    v = cJSON_GetObjectItem(node, "font");
    if (cJSON_IsString(v))
      w->set_font(this->host_->lookup_font(v->valuestring));
#endif
    return this->own_(w);
  }

  Widget *build_weather_(const cJSON *node) {
    auto *w = new WeatherWidget();
    const cJSON *v = cJSON_GetObjectItem(node, "show_icon");
    if (cJSON_IsBool(v))
      w->set_show_icon(cJSON_IsTrue(v));
    v = cJSON_GetObjectItem(node, "show_condition");
    if (cJSON_IsBool(v))
      w->set_show_condition(cJSON_IsTrue(v));
    v = cJSON_GetObjectItem(node, "text_position");
    if (cJSON_IsString(v))
      w->set_text_position(parse_weather_text_pos(v->valuestring));
    v = cJSON_GetObjectItem(node, "icon_align");
    if (cJSON_IsString(v))
      w->set_icon_align(parse_icon_align(v->valuestring));
    v = cJSON_GetObjectItem(node, "color");
    w->set_color(parse_color_json(v));
#ifdef USE_TEXT_SENSOR
    v = cJSON_GetObjectItem(node, "condition_id");
    if (cJSON_IsString(v))
      w->set_condition_sensor(this->host_->lookup_text_sensor(v->valuestring));
#endif
#ifdef USE_SENSOR
    v = cJSON_GetObjectItem(node, "temperature_id");
    if (cJSON_IsString(v))
      w->set_temperature_sensor(this->host_->lookup_sensor(v->valuestring));
#endif
#ifdef USE_FONT
    v = cJSON_GetObjectItem(node, "font");
    if (cJSON_IsString(v))
      w->set_font(this->host_->lookup_font(v->valuestring));
    v = cJSON_GetObjectItem(node, "icon_font");
    if (cJSON_IsString(v))
      w->set_icon_font(this->host_->lookup_font(v->valuestring));
#endif
    v = cJSON_GetObjectItem(node, "condition");
    if (cJSON_IsString(v))
      w->set_condition(v->valuestring);
    else if (!cJSON_GetObjectItem(node, "condition_id"))
      w->set_condition("cloudy");
    return this->own_(w);
  }

#ifdef USE_IMAGE
  bool load_rgb565_pack_(const std::string &pack_id, SdOwnedImage *out, int *fw, int *fh, int *frames) {
    std::string safe = pack_id;
    for (char &c : safe) {
      if (!(isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_'))
        c = '_';
    }
    std::string path = this->root_path_ + "/packs/" + safe + ".rgb565";
    std::string raw;
    if (!read_file(path, &raw) || raw.size() < 12)
      return false;
    if (memcmp(raw.data(), "PLI1", 4) != 0)
      return false;
    const uint8_t *p = reinterpret_cast<const uint8_t *>(raw.data());
    uint16_t width = p[4] | (uint16_t(p[5]) << 8);
    uint16_t height = p[6] | (uint16_t(p[7]) << 8);
    uint8_t chroma = p[8];
    size_t need = 12 + size_t(width) * height * 2;
    if (raw.size() < need)
      return false;
    out->data.assign(p + 12, p + need);
    auto transparency = chroma ? image::TRANSPARENCY_CHROMA_KEY : image::TRANSPARENCY_OPAQUE;
    out->image = new image::Image(out->data.data(), width, height, image::IMAGE_TYPE_RGB565, transparency);
    if (fw != nullptr)
      *fw = width;
    if (fh != nullptr)
      *fh = height;
    if (frames != nullptr)
      *frames = 1;
    return true;
  }
#endif

  Widget *build_sprite_(const cJSON *node) {
    auto *w = new SpriteWidget();
    bool has_fw = false, has_fh = false;
    const cJSON *v = cJSON_GetObjectItem(node, "frame_width");
    if (cJSON_IsNumber(v)) {
      w->set_frame_width(v->valueint);
      has_fw = true;
    }
    v = cJSON_GetObjectItem(node, "frame_height");
    if (cJSON_IsNumber(v)) {
      w->set_frame_height(v->valueint);
      has_fh = true;
    }
    v = cJSON_GetObjectItem(node, "frames");
    if (cJSON_IsNumber(v))
      w->set_frames(v->valueint);
    v = cJSON_GetObjectItem(node, "fps");
    if (cJSON_IsNumber(v))
      w->set_fps(static_cast<float>(v->valuedouble));
    v = cJSON_GetObjectItem(node, "loop");
    if (cJSON_IsBool(v))
      w->set_loop(cJSON_IsTrue(v));
#ifdef USE_IMAGE
    v = cJSON_GetObjectItem(node, "image_id");
    if (cJSON_IsString(v)) {
      SdOwnedImage img;
      int fw = 0, fh = 0, frames = 0;
      if (this->load_rgb565_pack_(v->valuestring, &img, &fw, &fh, &frames)) {
        w->set_image(img.image);
        if (!has_fw && fw > 0)
          w->set_frame_width(fw);
        if (!has_fh && fh > 0)
          w->set_frame_height(fh);
        this->images_->push_back(std::move(img));
      }
    }
#endif
    return this->own_(w);
  }

  Widget *build_custom_(const cJSON *node) {
    auto *w = new CustomWidget();
    const cJSON *v = cJSON_GetObjectItem(node, "color");
    w->set_color(parse_color_json(v));
    const cJSON *pal = cJSON_GetObjectItem(node, "palette");
    if (cJSON_IsArray(pal)) {
      cJSON *item = nullptr;
      cJSON_ArrayForEach(item, pal) { w->add_palette_color(parse_color_json(item)); }
    }
    v = cJSON_GetObjectItem(node, "pixels_packed");
    if (cJSON_IsArray(v)) {
      std::vector<uint8_t> packed;
      int width = 0, height = 0;
      cJSON *item = nullptr;
      size_t i = 0;
      cJSON_ArrayForEach(item, v) {
        if (i == 0)
          width = item->valueint;
        else if (i == 1)
          height = item->valueint;
        else
          packed.push_back(static_cast<uint8_t>(item->valueint));
        i++;
      }
      if (!packed.empty() && width > 0 && height > 0) {
        this->custom_pixel_storage_.push_back(std::move(packed));
        w->set_pixels(this->custom_pixel_storage_.back().data(), width, height);
      }
    }
    return this->own_(w);
  }

  PixelLayout *host_;
  std::string root_path_;
  std::vector<std::unique_ptr<Widget>> *owned_;
  std::vector<SdOwnedImage> *images_;
  std::vector<std::vector<uint8_t>> custom_pixel_storage_;
};

bool SdPlaylistLoader::load(const std::string &root_path, std::string *err) {
  if (this->host_ == nullptr) {
    if (err)
      *err = "no host";
    return false;
  }
  std::string manifest_raw;
  if (!read_file(root_path + "/manifest.json", &manifest_raw)) {
    if (err)
      *err = "manifest missing";
    return false;
  }
  cJSON *manifest = cJSON_Parse(manifest_raw.c_str());
  if (manifest == nullptr) {
    if (err)
      *err = "manifest parse error";
    return false;
  }
  const cJSON *schema = cJSON_GetObjectItem(manifest, "schema");
  if (!cJSON_IsNumber(schema) || schema->valueint != 1) {
    cJSON_Delete(manifest);
    if (err)
      *err = "unsupported manifest schema";
    return false;
  }
  std::string playlist_raw;
  if (!read_file(root_path + "/playlist.json", &playlist_raw)) {
    cJSON_Delete(manifest);
    if (err)
      *err = "playlist.json missing";
    return false;
  }
  const cJSON *expected_hash = cJSON_GetObjectItem(manifest, "playlist_json_sha256");
  if (cJSON_IsString(expected_hash)) {
    std::string actual;
    if (sha256_hex(playlist_raw, &actual) && actual != expected_hash->valuestring) {
      cJSON_Delete(manifest);
      if (err)
        *err = "playlist.json sha256 mismatch";
      return false;
    }
  }
  cJSON *playlist = cJSON_Parse(playlist_raw.c_str());
  cJSON_Delete(manifest);
  if (playlist == nullptr) {
    if (err)
      *err = "playlist.json parse error";
    return false;
  }

  std::vector<std::unique_ptr<Widget>> owned;
  std::vector<SdOwnedImage> images;
  SdWidgetBuilder builder(this->host_, root_path, &owned, &images);

  struct ScreenSpec {
    std::string id;
    Widget *root{nullptr};
    uint32_t duration_ms{0};
    ScreenTransition transition{ScreenTransition::FADE};
    uint32_t transition_ms{0};
  };
  std::vector<SdScreenSpec> specs;

  const cJSON *screens = cJSON_GetObjectItem(playlist, "screens");
  if (!cJSON_IsArray(screens)) {
    cJSON_Delete(playlist);
    if (err)
      *err = "playlist has no screens";
    return false;
  }
  cJSON *screen_node = nullptr;
  cJSON_ArrayForEach(screen_node, screens) {
    if (specs.size() >= kMaxScreenSlots)
      break;
    SdScreenSpec spec;
    const cJSON *sid = cJSON_GetObjectItem(screen_node, "id");
    if (cJSON_IsString(sid))
      spec.id = sid->valuestring;
    else
      spec.id = "screen_" + std::to_string(specs.size() + 1);
    const cJSON *root = cJSON_GetObjectItem(screen_node, "root");
    spec.root = builder.build(root);
    if (spec.root == nullptr)
      continue;
    spec.duration_ms = parse_ms_value(cJSON_GetObjectItem(screen_node, "duration_ms"));
    if (spec.duration_ms == 0)
      spec.duration_ms = parse_ms_value(cJSON_GetObjectItem(screen_node, "duration"));
    const cJSON *trans = cJSON_GetObjectItem(screen_node, "transition");
    if (cJSON_IsString(trans))
      screen_transition_from_name(trans->valuestring, &spec.transition);
    spec.transition_ms = parse_ms_value(cJSON_GetObjectItem(screen_node, "transition_ms"));
    if (spec.transition_ms == 0)
      spec.transition_ms = parse_ms_value(cJSON_GetObjectItem(screen_node, "transition_duration"));
    specs.push_back(std::move(spec));
  }
  cJSON_Delete(playlist);

  if (specs.empty()) {
    if (err)
      *err = "no valid screens in playlist";
    return false;
  }

  if (!this->host_->apply_sd_playlist(specs, std::move(owned), std::move(images), err))
    return false;
  if (err)
    *err = "loaded " + std::to_string(specs.size()) + " screens from sd";
  return true;
}

}  // namespace pixel_layout
}  // namespace esphome

#endif
