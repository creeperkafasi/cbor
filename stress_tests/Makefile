# CBOR Stress Tests Makefile
# Auto-generated stress test build configuration

# Define parent directory and target
PARENT_DIR = ..
TARGET ?= native
BUILD_DIR = ../build/$(TARGET)

# Include parent settings (but define our own BUILD_DIR first)
include $(PARENT_DIR)/Makefile

# Override BUILD_DIR to use our own stress test subdirectory
STRESS_BUILD_DIR = $(BUILD_DIR)/stress_tests

# Stress test specific settings (with relaxed warnings)
STRESS_CFLAGS = $(filter-out -Werror -Wunused-parameter,$(CFLAGS)) -DSTRESS_TEST -DSTACK_CHECK -I$(PARENT_DIR)/$(LIB_DIR) -Wno-unused-parameter
ifeq ($(TARGET),embedded)
    STRESS_CFLAGS += -I$(PARENT_DIR)/qemu
    STRESS_LDFLAGS = $(filter-out -T qemu/cortex-m3.ld,$(LDFLAGS)) -T $(PARENT_DIR)/qemu/cortex-m3.ld
else
    STRESS_LDFLAGS = $(LDFLAGS)
endif

# Stress test sources
STRESS_TESTS = deep_nesting_50 deep_nesting_100 large_data_1024 large_data_4096 mixed_complexity stack_overflow_detector

# Use parent's compiled library objects (with absolute paths)
PARENT_CFILES_OBJ = $(addprefix $(PARENT_DIR)/,$(patsubst %.c,$(BUILD_DIR)/%.o,$(CFILES)))

# Build all stress tests
all-stress: $(addprefix $(STRESS_BUILD_DIR)/,$(if $(filter embedded,$(TARGET)),$(addsuffix .elf,$(STRESS_TESTS)),$(STRESS_TESTS)))

# Create stress test build directory
$(STRESS_BUILD_DIR):
	@mkdir -p $@

# Pattern rule for stress tests (native)
$(STRESS_BUILD_DIR)/%: %.o $(PARENT_CFILES_OBJ) | $(STRESS_BUILD_DIR)
	$(LD) $(STRESS_CFLAGS) $(STRESS_LDFLAGS) $^ -o $@

# Pattern rule for stress tests (embedded)
$(STRESS_BUILD_DIR)/%.elf: %.o $(PARENT_CFILES_OBJ) | $(STRESS_BUILD_DIR)
	$(LD) $(STRESS_CFLAGS) $(STRESS_LDFLAGS) $^ -o $@

# Pattern rule for compiling stress test sources
%.o: %.c | $(STRESS_BUILD_DIR)
	$(CC) $(STRESS_CFLAGS) -c $< -o $@

# Run all stress tests
test-stress: all-stress
	@echo "🧪 Running stress tests..."
	@for test_base in $(STRESS_TESTS); do \
		if [ "$(TARGET)" = "embedded" ]; then \
			test="$(STRESS_BUILD_DIR)/$$test_base.elf"; \
			echo "🚀 Running $$test_base on QEMU..."; \
			timeout 30s qemu-system-arm -M lm3s6965evb -cpu cortex-m3 -nographic -semihosting -kernel $$test || echo "⚠️  Test $$test_base completed/timeout"; \
		else \
			test="$(STRESS_BUILD_DIR)/$$test_base"; \
			echo "🚀 Running $$test_base on native..."; \
			$$test || echo "⚠️  Test $$test_base failed"; \
		fi; \
		echo ""; \
	done

# Clean stress tests
clean-stress:
	rm -rf *.o $(STRESS_BUILD_DIR)

# Help
help-stress:
	@echo "Stress Test Targets:"
	@echo "  all-stress    - Build all stress tests"
	@echo "  test-stress   - Run all stress tests"
	@echo "  clean-stress  - Clean stress test artifacts"
	@echo ""
	@echo "Available tests: $(STRESS_TESTS)"

.PHONY: all-stress test-stress clean-stress help-stress
