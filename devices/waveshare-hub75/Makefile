DEVICE_YAML ?= waveshare-hub75.yml
LOCAL_DEVICE_YAML ?= waveshare-hub75.local.yml
DEVICE      ?= waveshare-matrix.local

define HELP_EXTRA
	@echo "  --- Component dev ---"
	@echo "  local-config        Validate $(LOCAL_DEVICE_YAML) (local component)"
	@echo "  local-build         Compile $(LOCAL_DEVICE_YAML) (local component)"
endef

include ../../mk/esphome-project.mk

.PHONY: local-config local-build

local-config: secrets-link
	"$(ESPHOME)" config "$(LOCAL_DEVICE_YAML)"

local-build: secrets-link
	"$(ESPHOME)" compile "$(LOCAL_DEVICE_YAML)"
