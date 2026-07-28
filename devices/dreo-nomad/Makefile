DEVICE_YAML ?= dreo-nomad.yml
LOCAL_DEVICE_YAML ?= dreo-nomad.local.yml
DEVICE      ?= bedroom-smart-fan.local

define HELP_EXTRA
	@echo "  --- Component dev ---"
	@echo "  local-config        Validate $(LOCAL_DEVICE_YAML) (local component)"
	@echo "  local-build         Compile $(LOCAL_DEVICE_YAML) (local component)"
	@echo "  --- CI (from repo root) ---"
	@echo "  make test COMPONENT=dreo_tuya_mcu"
	@echo "  make smoke COMPONENT=dreo_tuya_mcu"
	@echo "  --- First flash (stock HeFi BK7231N) ---"
	@echo "  deps                Install ltchiptool + pyserial"
	@echo "  uart-check          Probe UART (PORT= optional)"
	@echo "  backup PORT=...     Dump BK7231N flash before experiments"
	@echo "  build-stage1-wifi-nogzip  Build stage-1 RBL (recommended)"
	@echo "  ota-upload          Upload stage-1 via Dreo AP 192.168.0.1"
	@echo "  ota-upload-uart PORT=...  UART heartbeat + stage-1 OTA"
	@echo "  uart-flash PORT=... Direct UART flash full $(DEVICE_YAML)"
endef

include ../../mk/esphome-project.mk

# --- Dreo DR-HTF007S flash tooling (BK7231N replacement) --------------------

SHELL := /bin/bash
FLASH_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
REPO_ROOT := $(abspath $(FLASH_ROOT)/../..)
VENV_PYTHON := $(REPO_ROOT)/.venv/bin/python3
LT := $(REPO_ROOT)/.venv/bin/ltchiptool

PORT ?=
HOST ?= 192.168.0.1

BUILD_DIR := $(FLASH_ROOT)/.esphome/build/bedroom-smart-fan/.pioenvs/bedroom-smart-fan
UF2 ?= $(BUILD_DIR)/firmware.uf2

FW ?= firmware-stage1-wifi-only-nogzip.rbl
FW_STAGE1 ?= firmware-stage1.rbl
FW_STAGE1_WIFI ?= firmware-stage1-wifi-only.rbl
FW_STAGE1_NOGZIP ?= firmware-stage1-wifi-only-nogzip.rbl

DUMP ?= dumps/bk72xx-$(shell date +%Y%m%d-%H%M%S).bin
DUMP_DECRC ?= $(DUMP:.bin=-decrc.bin)

.PHONY: deps uart-check backup backup-uncrc dumps \
	build-stage1 build-stage1-nogzip build-stage1-wifi build-stage1-wifi-nogzip build-stage1-all \
	ota-upload ota-upload-stage1 ota-upload-stage1-wifi ota-upload-stage1-nogzip \
	ota-upload-uart ota-upload-uart-stage1 ota-upload-uart-stage1-nogzip \
	uart-flash uart-flash-lt uart-flash-retry \
	check-port check-firmware check-dump check-uf2 \
	local-config local-build
	"$(ESPHOME)" config "$(LOCAL_DEVICE_YAML)"

local-build: secrets-link
	"$(ESPHOME)" compile "$(LOCAL_DEVICE_YAML)"


deps:
	@test -x "$(ESPHOME)" || { echo "Missing ESPHome at $(ESPHOME)"; exit 1; }
	"$(REPO_ROOT)/.venv/bin/pip" install -q pyserial ltchiptool -r "$(FLASH_ROOT)/tools/requirements.txt" 2>/dev/null || \
		"$(REPO_ROOT)/.venv/bin/pip" install -q pyserial ltchiptool

uart-check: deps secrets-link
ifdef PORT
	"$(VENV_PYTHON)" "$(FLASH_ROOT)/tools/uart_check.py" "$(PORT)"
else
	"$(VENV_PYTHON)" "$(FLASH_ROOT)/tools/uart_check.py"
endif

dumps:
	@mkdir -p "$(FLASH_ROOT)/dumps"

backup: check-port deps dumps secrets-link
	@echo ">>> Full flash backup to $(DUMP)"
	@echo "    Wiring: fan TX -> FTDI RX, fan RX -> FTDI TX, GND."
	@echo "    When ltchiptool prompts, ground CEN on the back of the board briefly."
	"$(LT)" flash read -d "$(PORT)" bk72xx "$(FLASH_ROOT)/$(DUMP)"
	@echo ">>> Saved $(FLASH_ROOT)/$(DUMP)"

backup-uncrc: check-dump deps
	"$(LT)" soc bkpackager uncrc "$(FLASH_ROOT)/$(DUMP)" "$(FLASH_ROOT)/$(DUMP_DECRC)"

build-stage1: secrets-link
	"$(VENV_PYTHON)" "$(FLASH_ROOT)/tools/build_stage1.py"

build-stage1-nogzip: secrets-link
	"$(VENV_PYTHON)" "$(FLASH_ROOT)/tools/build_stage1.py" --nogzip

build-stage1-wifi: secrets-link
	"$(VENV_PYTHON)" "$(FLASH_ROOT)/tools/build_stage1.py" --profile wifi-only

build-stage1-wifi-nogzip: secrets-link
	"$(VENV_PYTHON)" "$(FLASH_ROOT)/tools/build_stage1.py" --profile wifi-only --nogzip

build-stage1-all: secrets-link
	"$(VENV_PYTHON)" "$(FLASH_ROOT)/tools/build_stage1.py" --all

ota-upload: check-firmware
	@echo ">>> Join Dreo AP, upload raw RBL to http://$(HOST)/module"
	@echo "    Do NOT use curl -F — it corrupts the multipart body."
	"$(VENV_PYTHON)" "$(FLASH_ROOT)/upload.py" "$(FLASH_ROOT)/$(FW)" --raw

ota-upload-stage1:
	$(MAKE) ota-upload FW=$(FW_STAGE1)

ota-upload-stage1-wifi:
	$(MAKE) ota-upload FW=$(FW_STAGE1_WIFI)

ota-upload-stage1-nogzip:
	$(MAKE) ota-upload FW=$(FW_STAGE1_NOGZIP)

ota-upload-uart: check-port check-firmware secrets-link
	"$(VENV_PYTHON)" "$(FLASH_ROOT)/tools/ota_upload.py" "$(PORT)" "$(FLASH_ROOT)/$(FW)"

ota-upload-uart-stage1:
	$(MAKE) ota-upload-uart FW=$(FW_STAGE1)

ota-upload-uart-stage1-nogzip:
	$(MAKE) ota-upload-uart FW=$(FW_STAGE1_NOGZIP)

uart-flash: check-port build secrets-link
	"$(ESPHOME)" upload "$(DEVICE_YAML)" --device "$(PORT)"

uart-flash-lt: check-port check-uf2 deps secrets-link
	"$(LT)" flash write -d "$(PORT)" -t 60 "$(UF2)"

uart-flash-retry: check-port check-uf2 deps secrets-link
	@n=0; while true; do \
	  n=$$((n+1)); echo ""; echo "=== Attempt $$n ==="; \
	  "$(LT)" flash write -d "$(PORT)" -t 60 "$(UF2)" && exit 0; \
	  echo ">>> Failed — retry in 4s (tap CEN on next 'Connecting')..."; sleep 4; \
	done

check-port:
ifndef PORT
	$(error Set PORT=/dev/cu.usbserial-XXXX (CP2102 on macOS; use cu not tty))
endif

check-firmware:
	@test -f "$(FLASH_ROOT)/$(FW)" || { echo "Missing $(FW) — run: make build-stage1-wifi-nogzip"; exit 1; }

check-dump:
	@test -f "$(FLASH_ROOT)/$(DUMP)" || { echo "Missing $(DUMP) — run: make backup PORT=..."; exit 1; }

check-uf2:
	@test -f "$(UF2)" || { echo "Missing $(UF2) — run: make build"; exit 1; }
