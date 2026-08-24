#include "playlist_ha.h"

#include "esphome/core/log.h"

namespace esphome {
namespace pixel_layout {

#ifdef USE_SELECT
void PixelLayoutScreenSelect::setup() {
  if (this->parent_ != nullptr) {
    std::vector<std::string> opts;
    opts.reserve(this->parent_->screen_count());
    for (size_t i = 0; i < this->parent_->screen_count(); i++) {
      const std::string &id = this->parent_->screen_id(i);
      if (!id.empty())
        opts.push_back(id);
    }
    if (!opts.empty())
      this->traits.set_options(opts);
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
#endif

#ifdef USE_BUTTON
void PixelLayoutNextButton::dump_config() { LOG_BUTTON("", "Pixel Layout Next Screen", this); }

void PixelLayoutNextButton::press_action() {
  if (this->parent_ != nullptr)
    this->parent_->show_next_enabled();
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
#endif

}  // namespace pixel_layout
}  // namespace esphome
