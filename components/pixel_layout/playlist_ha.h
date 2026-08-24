#pragma once

#include "pixel_layout.h"

#include "esphome/core/component.h"

#ifdef USE_SELECT
#include "esphome/components/select/select.h"
#endif
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif
#ifdef USE_BUTTON
#include "esphome/components/button/button.h"
#endif
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif

namespace esphome {
namespace pixel_layout {

#ifdef USE_SELECT
class PixelLayoutScreenSelect : public select::Select, public Component {
 public:
  void set_parent(PixelLayout *parent) { this->parent_ = parent; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void control(const std::string &value) override;
  void sync_();
  PixelLayout *parent_{nullptr};
};

class PixelLayoutTransitionSelect : public select::Select, public Component {
 public:
  void set_parent(PixelLayout *parent) { this->parent_ = parent; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void control(const std::string &value) override;
  void sync_();
  PixelLayout *parent_{nullptr};
};
#endif

#ifdef USE_SWITCH
class PixelLayoutPinSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(PixelLayout *parent) { this->parent_ = parent; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void write_state(bool state) override;
  PixelLayout *parent_{nullptr};
};

class PixelLayoutRandomSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(PixelLayout *parent) { this->parent_ = parent; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void write_state(bool state) override;
  PixelLayout *parent_{nullptr};
};

class PixelLayoutRotateOverrideSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(PixelLayout *parent) { this->parent_ = parent; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void write_state(bool state) override;
  PixelLayout *parent_{nullptr};
};

class PixelLayoutTransitionOverrideSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(PixelLayout *parent) { this->parent_ = parent; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void write_state(bool state) override;
  PixelLayout *parent_{nullptr};
};

class PixelLayoutScreenEnabledSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(PixelLayout *parent) { this->parent_ = parent; }
  void set_screen_index(size_t index) { this->index_ = index; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void write_state(bool state) override;
  PixelLayout *parent_{nullptr};
  size_t index_{0};
};

class PixelLayoutNightScheduleSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(PixelLayout *parent) { this->parent_ = parent; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void write_state(bool state) override;
  PixelLayout *parent_{nullptr};
};

#ifdef USE_PIXEL_LAYOUT_SD_STORAGE
class PixelLayoutUseSdLayoutSwitch : public switch_::Switch, public Component {
 public:
  void set_parent(PixelLayout *parent) { this->parent_ = parent; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void write_state(bool state) override;
  PixelLayout *parent_{nullptr};
};
#endif
#endif

#ifdef USE_BUTTON
class PixelLayoutNextButton : public button::Button, public Component {
 public:
  void set_parent(PixelLayout *parent) { this->parent_ = parent; }
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void press_action() override;
  PixelLayout *parent_{nullptr};
};

class PixelLayoutSleepButton : public button::Button, public Component {
 public:
  void set_parent(PixelLayout *parent) { this->parent_ = parent; }
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void press_action() override;
  PixelLayout *parent_{nullptr};
};

#ifdef USE_PIXEL_LAYOUT_SD_STORAGE
class PixelLayoutReloadSdLayoutButton : public button::Button, public Component {
 public:
  void set_parent(PixelLayout *parent) { this->parent_ = parent; }
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void press_action() override;
  PixelLayout *parent_{nullptr};
};
#endif
#endif

#ifdef USE_NUMBER
class PixelLayoutRotateIntervalNumber : public number::Number, public Component {
 public:
  void set_parent(PixelLayout *parent) { this->parent_ = parent; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void control(float value) override;
  PixelLayout *parent_{nullptr};
};

class PixelLayoutTransitionDurationNumber : public number::Number, public Component {
 public:
  void set_parent(PixelLayout *parent) { this->parent_ = parent; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void control(float value) override;
  PixelLayout *parent_{nullptr};
};

class PixelLayoutNightHourNumber : public number::Number, public Component {
 public:
  void set_parent(PixelLayout *parent) { this->parent_ = parent; }
  void set_which(uint8_t which) { this->which_ = which; }  // 0 off_h 1 off_m 2 on_h 3 on_m
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void control(float value) override;
  void sync_();
  PixelLayout *parent_{nullptr};
  uint8_t which_{0};
};
#endif

#ifdef USE_TEXT_SENSOR
class PixelLayoutScreenLabelTextSensor : public text_sensor::TextSensor, public Component {
 public:
  void set_parent(PixelLayout *parent) { this->parent_ = parent; }
  void set_screen_index(size_t index) { this->index_ = index; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void sync_();
  PixelLayout *parent_{nullptr};
  size_t index_{0};
};
#endif

#ifdef USE_PIXEL_LAYOUT_SD_STORAGE
#ifdef USE_TEXT_SENSOR
class PixelLayoutSdStatusTextSensor : public text_sensor::TextSensor, public Component {
 public:
  void set_parent(PixelLayout *parent) { this->parent_ = parent; }
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  PixelLayout *parent_{nullptr};
  std::string last_{};
};
#endif
#endif

}  // namespace pixel_layout
}  // namespace esphome
