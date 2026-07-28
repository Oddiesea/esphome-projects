# Resolve ESPHome CLI: prefer a working repo venv, else PATH.
# Include from repo root (ci.mk) or via esphome-project.mk (sets ROOT).

ifndef ROOT
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
endif

VENV_PYTHON := $(ROOT)/.venv/bin/python3
VENV_ESPHOME := $(ROOT)/.venv/bin/esphome

VENV_VALID := $(shell \
  test -x "$(VENV_PYTHON)" && \
  "$(VENV_PYTHON)" -m esphome version >/dev/null 2>&1 && \
  echo yes)

ifeq ($(VENV_VALID),yes)
  ESPHOME ?= $(VENV_ESPHOME)
else
  ESPHOME ?= esphome
endif
