COMPONENT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

include ../../mk/component-tests.mk

.PHONY: help
help:
	@echo "pixel_layout component"
	@echo "  test       Run host GoogleTest suite (DrawContext, widgets, PixelLayout)"
