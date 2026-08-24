COMPONENT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

.PHONY: test clean-test help
help:
	@echo "hub75_dma component"
	@echo "  test       No host tests (DMA hardware driver; use make smoke COMPONENT=hub75_dma)"

test:
	@echo "hub75_dma: no host unit tests (covered by ESPHome smoke compile)"

clean-test:
	@true
