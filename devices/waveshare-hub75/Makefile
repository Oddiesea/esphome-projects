DEVICE_YAML ?= waveshare-hub75.yml
LOCAL_DEVICE_YAML ?= waveshare-hub75.local.yml
DEVICE      ?= waveshare-matrix.local
SERIAL_PORT ?= /dev/cu.usbmodem1101

define HELP_EXTRA
	@echo "  --- Component dev ---"
	@echo "  local-config        Validate $(LOCAL_DEVICE_YAML) (local component)"
	@echo "  local-build         Compile $(LOCAL_DEVICE_YAML) (local component)"
	@echo "  local-flash         Compile + USB serial upload ($(SERIAL_PORT))"
	@echo "  color-test          Numbered RGBYCW grid (no pixel_layout) + flash"
endef

include ../../mk/esphome-project.mk

.PHONY: local-config local-build local-flash color-test

local-config: secrets-link
	"$(ESPHOME)" config "$(LOCAL_DEVICE_YAML)"

local-build: secrets-link
	"$(ESPHOME)" compile "$(LOCAL_DEVICE_YAML)"

local-flash: secrets-link
	"$(ESPHOME)" compile "$(LOCAL_DEVICE_YAML)"
	"$(ROOT)/.venv/bin/python" -m esptool --chip esp32s3 --port "$(SERIAL_PORT)" \
	  --before usb-reset --after hard-reset --no-stub write-flash -z \
	  --flash-mode dio --flash-size detect 0x0 \
	  .esphome/build/waveshare-matrix-local/build/firmware.factory.bin

color-test: secrets-link
	"$(ESPHOME)" compile waveshare-hub75.color-test.yml
	"$(ROOT)/.venv/bin/python" -m esptool --chip esp32s3 --port "$(SERIAL_PORT)" \
	  --before usb-reset --after hard-reset --no-stub write-flash -z \
	  --flash-mode dio --flash-size detect 0x0 \
	  .esphome/build/waveshare-color-test/build/firmware.factory.bin
	"$(ESPHOME)" logs waveshare-hub75.color-test.yml --device "$(SERIAL_PORT)"
