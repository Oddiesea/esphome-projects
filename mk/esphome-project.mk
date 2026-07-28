# Shared ESPHome project targets. Include from a project Makefile:
#   DEVICE_YAML ?= your-device.yml
#   SMOKE_YAML  ?= ci/smoke.yml   # optional
#   DEVICE      ?= device.local
#   include ../../mk/esphome-project.mk

ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
PROJECT_DIR := $(CURDIR)
VENV_ESPHOME := $(ROOT)/.venv/bin/esphome

ifeq ($(shell test -x "$(VENV_ESPHOME)" && echo yes),yes)
  ESPHOME ?= $(VENV_ESPHOME)
else
  ESPHOME ?= esphome
endif

SECRETS_SOURCE := $(ROOT)/secrets.yaml
SECRETS_LINK := $(PROJECT_DIR)/secrets.yaml
TEST_BUILD ?= tests/build

.PHONY: secrets-link
secrets-link:
	@test -f "$(SECRETS_SOURCE)" || (echo "Missing $(SECRETS_SOURCE) — copy secrets.yaml.example to secrets.yaml at repo root" && exit 1)
	@ln -sf "$(SECRETS_SOURCE)" "$(SECRETS_LINK)"

.PHONY: help config validate build compile ota upload run flash logs clean

ifndef HELP_EXTRA
define HELP_EXTRA
endef
endif

help:
	@echo "Project: $(notdir $(PROJECT_DIR))"
	@echo "  config        Validate $(DEVICE_YAML)"
	@echo "  build         Compile $(DEVICE_YAML)"
	@echo "  ota           OTA upload to $(DEVICE)"
	@echo "  run           Compile + OTA upload + logs"
	@echo "  flash         Compile + USB serial upload + logs"
	@echo "  logs          Follow device logs"
	@echo "  clean         Remove local .esphome and test build dirs"
	$(HELP_EXTRA)
	@echo ""
	@echo "Secrets: $(SECRETS_SOURCE) (symlinked into this project)"
	@echo "Components: $(ROOT)/components"
	@echo "Overrides: DEVICE_YAML=... DEVICE=... ESPHOME=..."

config validate: secrets-link
	"$(ESPHOME)" config "$(DEVICE_YAML)"

build compile: secrets-link
	"$(ESPHOME)" compile "$(DEVICE_YAML)"

ota upload: secrets-link
	"$(ESPHOME)" upload "$(DEVICE_YAML)" --device "$(DEVICE)"

run: secrets-link
	"$(ESPHOME)" run "$(DEVICE_YAML)" --device "$(DEVICE)"

flash: secrets-link
	"$(ESPHOME)" run "$(DEVICE_YAML)" --device USB

logs: secrets-link
	"$(ESPHOME)" logs "$(DEVICE_YAML)" --device "$(DEVICE)"

clean:
	rm -rf "$(PROJECT_DIR)/.esphome" "$(TEST_BUILD)"
