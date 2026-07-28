# Host GoogleTest targets for a component under components/<name>/.
# Include from components/<name>/Makefile:
#   COMPONENT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
#   include ../../mk/component-tests.mk

TEST_DIR ?= $(COMPONENT_DIR)/tests
TEST_BUILD ?= $(TEST_DIR)/build

.PHONY: test clean-test

test: $(TEST_BUILD)/CMakeCache.txt
	cmake --build "$(TEST_BUILD)" -j
	ctest --test-dir "$(TEST_BUILD)" --output-on-failure

$(TEST_BUILD)/CMakeCache.txt:
	cmake -S "$(TEST_DIR)" -B "$(TEST_BUILD)" -DCMAKE_BUILD_TYPE=Debug

clean-test:
	rm -rf "$(TEST_BUILD)"
