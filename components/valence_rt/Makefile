COMPONENT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

include ../../mk/component-tests.mk

.PHONY: help
help:
	@echo "valence_rt component"
	@echo "  test       Run host GoogleTest suite (valence_protocol)"
