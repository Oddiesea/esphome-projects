#include "playlist_ha.h"

#include "esphome/core/log.h"

static const char *const TAG = "pixel_layout.ha";

namespace esphome {
namespace pixel_layout {

#ifdef USE_SELECT
void PixelLayoutScreenSelect::setup() {
  // Options come from select.new_select() at codegen (screen ids / transition names).
  // SelectTraits no longer accepts std::vector<std::string>.
  if (this->parent_ != nullptr) {
    this->parent_->add_on_playlist_change([this]() { this->sync_(); });
  }
  this->sync_();
}

void PixelLayoutScreenSelect::dump_config() { LOG_SELECT("", "Pixel Layout Screen", this); }

void PixelLayoutScreenSelect::control(const std::string &value) {
  if (this->parent_ != nullptr)
    this->parent_->show_screen(value);
  this->publish_state(value);
}

void PixelLayoutScreenSelect::sync_() {
  if (this->parent_ == nullptr)
    return;
  const std::string id = this->parent_->current_screen_id();
  if (!id.empty())
    this->publish_state(id);
}

void PixelLayoutTransitionSelect::setup() {
  if (this->parent_ != nullptr) {
    this->parent_->add_on_playlist_change([this]() { this->sync_(); });
  }
  this->sync_();
}

void PixelLayoutTransitionSelect::dump_config() { LOG_SELECT("", "Pixel Layout Transition", this); }

void PixelLayoutTransitionSelect::control(const std::string &value) {
  ScreenTransition t = ScreenTransition::FADE;
  if (screen_transition_from_name(value.c_str(), &t) && this->parent_ != nullptr)
    this->parent_->set_transition(t);
  this->publish_state(value);
}

void PixelLayoutTransitionSelect::sync_() {
  if (this->parent_ == nullptr)
    return;
  this->publish_state(screen_transition_name(this->parent_->get_transition()));
}
#endif

#ifdef USE_SWITCH
void PixelLayoutPinSwitch::setup() {
  if (this->parent_ != nullptr) {
    this->parent_->add_on_playlist_change([this]() {
      if (this->parent_ != nullptr)
        this->publish_state(this->parent_->is_pinned());
    });
    this->publish_state(this->parent_->is_pinned());
  }
}

void PixelLayoutPinSwitch::dump_config() { LOG_SWITCH("", "Pixel Layout Pin Screen", this); }

void PixelLayoutPinSwitch::write_state(bool state) {
  if (this->parent_ != nullptr)
    this->parent_->set_pinned(state);
  this->publish_state(state);
}

void PixelLayoutRandomSwitch::setup() {
  if (this->parent_ != nullptr) {
    this->parent_->add_on_playlist_change([this]() {
      if (this->parent_ != nullptr)
        this->publish_state(this->parent_->get_screen_random());
    });
    this->publish_state(this->parent_->get_screen_random());
  }
}

void PixelLayoutRandomSwitch::dump_config() { LOG_SWITCH("", "Pixel Layout Random Order", this); }

void PixelLayoutRandomSwitch::write_state(bool state) {
  if (this->parent_ != nullptr)
    this->parent_->set_screen_random(state);
  this->publish_state(state);
}

void PixelLayoutRotateOverrideSwitch::setup() {
  if (this->parent_ != nullptr) {
    this->parent_->add_on_playlist_change([this]() {
      if (this->parent_ != nullptr)
        this->publish_state(this->parent_->get_rotate_override());
    });
    this->publish_state(this->parent_->get_rotate_override());
  }
}

void PixelLayoutRotateOverrideSwitch::dump_config() { LOG_SWITCH("", "Pixel Layout Override Rotate", this); }

void PixelLayoutRotateOverrideSwitch::write_state(bool state) {
  if (this->parent_ != nullptr)
    this->parent_->set_rotate_override(state);
  this->publish_state(state);
}

void PixelLayoutTransitionOverrideSwitch::setup() {
  if (this->parent_ != nullptr) {
    this->parent_->add_on_playlist_change([this]() {
      if (this->parent_ != nullptr)
        this->publish_state(this->parent_->get_transition_override());
    });
    this->publish_state(this->parent_->get_transition_override());
  }
}

void PixelLayoutTransitionOverrideSwitch::dump_config() { LOG_SWITCH("", "Pixel Layout Override Transition", this); }

void PixelLayoutTransitionOverrideSwitch::write_state(bool state) {
  if (this->parent_ != nullptr)
    this->parent_->set_transition_override(state);
  this->publish_state(state);
}

void PixelLayoutScreenEnabledSwitch::setup() {
  if (this->parent_ != nullptr) {
    this->parent_->add_on_playlist_change([this]() {
      if (this->parent_ != nullptr)
        this->publish_state(this->parent_->is_screen_enabled(this->index_));
    });
    this->publish_state(this->parent_->is_screen_enabled(this->index_));
  }
}

void PixelLayoutScreenEnabledSwitch::dump_config() { LOG_SWITCH("", "Pixel Layout Screen Enabled", this); }

void PixelLayoutScreenEnabledSwitch::write_state(bool state) {
  if (this->parent_ != nullptr)
    this->parent_->set_screen_enabled(this->index_, state);
  this->publish_state(state);
}

void PixelLayoutNightScheduleSwitch::setup() {
  if (this->parent_ != nullptr) {
    this->parent_->add_on_playlist_change([this]() {
      if (this->parent_ != nullptr)
        this->publish_state(this->parent_->get_night_schedule_enabled());
    });
    this->publish_state(this->parent_->get_night_schedule_enabled());
  }
}

void PixelLayoutNightScheduleSwitch::dump_config() { LOG_SWITCH("", "Pixel Layout Night Schedule", this); }

void PixelLayoutNightScheduleSwitch::write_state(bool state) {
  if (this->parent_ != nullptr)
    this->parent_->set_night_schedule_enabled(state);
  this->publish_state(state);
}
#endif

#ifdef USE_BUTTON
void PixelLayoutNextButton::dump_config() { LOG_BUTTON("", "Pixel Layout Next Screen", this); }

void PixelLayoutNextButton::press_action() {
  if (this->parent_ != nullptr)
    this->parent_->show_next_enabled();
}

void PixelLayoutSleepButton::dump_config() { LOG_BUTTON("", "Pixel Layout Sleep", this); }

void PixelLayoutSleepButton::press_action() {
  if (this->parent_ != nullptr)
    this->parent_->sleep_until_wake();
}
#endif

#ifdef USE_NUMBER
void PixelLayoutRotateIntervalNumber::setup() {
  if (this->parent_ != nullptr) {
    this->parent_->add_on_playlist_change([this]() {
      if (this->parent_ != nullptr)
        this->publish_state(this->parent_->get_rotate_ms() / 1000.0f);
    });
    this->publish_state(this->parent_->get_rotate_ms() / 1000.0f);
  }
}

void PixelLayoutRotateIntervalNumber::dump_config() { LOG_NUMBER("", "Pixel Layout Rotate Interval", this); }

void PixelLayoutRotateIntervalNumber::control(float value) {
  if (this->parent_ != nullptr)
    this->parent_->set_rotate_ms(static_cast<uint32_t>(value * 1000.0f));
  this->publish_state(value);
}

void PixelLayoutTransitionDurationNumber::setup() {
  if (this->parent_ != nullptr) {
    this->parent_->add_on_playlist_change([this]() {
      if (this->parent_ != nullptr)
        this->publish_state(this->parent_->get_transition_ms() / 1000.0f);
    });
    this->publish_state(this->parent_->get_transition_ms() / 1000.0f);
  }
}

void PixelLayoutTransitionDurationNumber::dump_config() { LOG_NUMBER("", "Pixel Layout Transition Duration", this); }

void PixelLayoutTransitionDurationNumber::control(float value) {
  if (this->parent_ != nullptr)
    this->parent_->set_transition_ms(static_cast<uint32_t>(value * 1000.0f));
  this->publish_state(value);
}

void PixelLayoutNightHourNumber::setup() {
  if (this->parent_ != nullptr) {
    this->parent_->add_on_playlist_change([this]() { this->sync_(); });
  }
  this->sync_();
}

void PixelLayoutNightHourNumber::dump_config() { LOG_NUMBER("", "Pixel Layout Night Time", this); }

void PixelLayoutNightHourNumber::sync_() {
  if (this->parent_ == nullptr)
    return;
  float v = 0;
  switch (this->which_) {
    case 0:
      v = this->parent_->get_night_off_hour();
      break;
    case 1:
      v = this->parent_->get_night_off_minute();
      break;
    case 2:
      v = this->parent_->get_night_on_hour();
      break;
    default:
      v = this->parent_->get_night_on_minute();
      break;
  }
  this->publish_state(v);
}

void PixelLayoutNightHourNumber::control(float value) {
  if (this->parent_ != nullptr) {
    const uint8_t v = static_cast<uint8_t>(value);
    switch (this->which_) {
      case 0:
        this->parent_->set_night_off_hour(v);
        break;
      case 1:
        this->parent_->set_night_off_minute(v);
        break;
      case 2:
        this->parent_->set_night_on_hour(v);
        break;
      default:
        this->parent_->set_night_on_minute(v);
        break;
    }
  }
  this->publish_state(value);
}
#endif

#ifdef USE_TEXT_SENSOR
void PixelLayoutScreenLabelTextSensor::setup() {
  if (this->parent_ != nullptr) {
    this->parent_->add_on_playlist_change([this]() { this->sync_(); });
  }
  this->sync_();
}

void PixelLayoutScreenLabelTextSensor::dump_config() {
  LOG_TEXT_SENSOR("", "Pixel Layout Screen Label", this);
}

void PixelLayoutScreenLabelTextSensor::sync_() {
  if (this->parent_ == nullptr)
    return;
  std::string label;
  if (this->index_ < this->parent_->screen_count()) {
    label = this->parent_->screen_id(this->index_);
  }
  this->publish_state(label);
}
#endif

#ifdef USE_PIXEL_LAYOUT_SD_STORAGE
#ifdef USE_SWITCH
void PixelLayoutUseSdLayoutSwitch::setup() {
  if (this->parent_ != nullptr)
    this->publish_state(this->parent_->sd_storage().use_sd_layout());
}

void PixelLayoutUseSdLayoutSwitch::dump_config() { LOG_SWITCH("", "Pixel Layout Use SD Layout", this); }

void PixelLayoutUseSdLayoutSwitch::write_state(bool state) {
  if (this->parent_ != nullptr)
    this->parent_->sd_storage().set_use_sd_layout(state);
  this->publish_state(state);
}
#endif

#ifdef USE_BUTTON
void PixelLayoutReloadSdLayoutButton::dump_config() { LOG_BUTTON("", "Pixel Layout Reload SD Layout", this); }

void PixelLayoutReloadSdLayoutButton::press_action() {
  if (this->parent_ != nullptr)
    this->parent_->reload_from_sd();
}
#endif

#ifdef USE_TEXT_SENSOR
void PixelLayoutSdStatusTextSensor::setup() {
  if (this->parent_ != nullptr)
    this->last_ = this->parent_->sd_storage().status();
  this->publish_state(this->last_);
}

void PixelLayoutSdStatusTextSensor::loop() {
  if (this->parent_ == nullptr)
    return;
  const std::string &s = this->parent_->sd_storage().status();
  if (s == this->last_)
    return;
  this->last_ = s;
  this->publish_state(this->last_);
}

void PixelLayoutSdStatusTextSensor::dump_config() { LOG_TEXT_SENSOR("", "Pixel Layout SD Status", this); }
#endif
#endif

}  // namespace pixel_layout
}  // namespace esphome
