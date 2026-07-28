# CI and local verification targets. Include from repo root after mk/components.mk:
#   include mk/components.mk
#   include mk/ci.mk

SMOKE_DIR := ci/smoke
PACKAGE_SCRIPT := ci/scripts/package-component.sh

include $(dir $(lastword $(MAKEFILE_LIST)))esphome-env.mk

.PHONY: test smoke package ci clean-tests clean-dist

test:
ifdef COMPONENT
	$(MAKE) -C components/$(COMPONENT) test
else
	@for c in $(COMPONENTS); do \
	  echo "=== test $$c ==="; \
	  $(MAKE) -C components/$$c test || exit 1; \
	done
endif

smoke:
ifdef COMPONENT
	"$(ESPHOME)" config "$(SMOKE_DIR)/$(COMPONENT).yml"
	"$(ESPHOME)" compile "$(SMOKE_DIR)/$(COMPONENT).yml"
else
	@for c in $(COMPONENTS); do \
	  echo "=== smoke $$c ==="; \
	  "$(ESPHOME)" config "$(SMOKE_DIR)/$$c.yml" && \
	  "$(ESPHOME)" compile "$(SMOKE_DIR)/$$c.yml" || exit 1; \
	done
endif

package:
	@for c in $(COMPONENTS); do \
	  echo "=== package $$c ==="; \
	  "$(PACKAGE_SCRIPT)" "$$c" || exit 1; \
	done

ci: test smoke

clean-tests:
	@for c in $(COMPONENTS); do $(MAKE) -C components/$$c clean-test; done

clean-dist:
	rm -rf "$(ROOT)/dist"
