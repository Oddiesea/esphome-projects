DEVICES_DIR := devices
PROJECTS := solar-plant dreo-nomad waveshare-hub75

include mk/components.mk
include mk/ci.mk

.PHONY: help clean serve
.PHONY: $(PROJECTS)

help:
	@echo "ESPHome monorepo"
	@echo ""
	@echo "Devices (under $(DEVICES_DIR)/): $(PROJECTS)"
	@echo ""
	@echo "Configurator:"
	@echo "  make serve             pixel_layout UI + HA proxy + MCP (http://127.0.0.1:8765/)"
	@echo ""
	@echo "CI / verification:"
	@echo "  make test              Host unit tests (all components)"
	@echo "  make smoke             ESPHome smoke compiles (ci/smoke/)"
	@echo "  make ci                test + smoke"
	@echo "  make package           Zip components to dist/"
	@echo "  make clean             Remove build caches, test dirs, and dist/"
	@echo "  make clean-tests       Remove component test build dirs"
	@echo ""
	@echo "Per device:"
	@echo "  make -C $(DEVICES_DIR)/solar-plant build"
	@echo "  make -C $(DEVICES_DIR)/dreo-nomad config"
	@echo ""
	@echo "Filter by component: make test COMPONENT=valence_rt"
	@echo "See ci/README.md for CI layout."

clean:
	@for p in $(PROJECTS); do $(MAKE) -C $(DEVICES_DIR)/$$p clean; done
	$(MAKE) clean-tests clean-dist

serve:
	$(MAKE) -C configurators/pixel_layout serve

$(PROJECTS):
	$(MAKE) -C $(DEVICES_DIR)/$@
