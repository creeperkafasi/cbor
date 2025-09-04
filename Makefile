# Default target
TARGET ?= native

# Build directory
BUILD_DIR = build/$(TARGET)

# Toolchain configuration
ifeq ($(TARGET),native)
    CC = gcc
    LD = $(CC)
else ifeq ($(TARGET),embedded)
    CC = arm-none-eabi-gcc
    LD = $(CC)
else
    $(error "Unknown target: $(TARGET)")
endif

# Source files
SRC_DIR = .
LIB_DIR = lib
EXAMPLES_DIR = examples

# Library files
CFILES = $(LIB_DIR)/cbor.c $(LIB_DIR)/debug.c $(LIB_DIR)/memory_profiler.c
CFILES_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(CFILES))

# Main application
MAIN_SRC = $(SRC_DIR)/main.c
MAIN_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(MAIN_SRC))
MAIN_OUT = $(BUILD_DIR)/$(if $(filter embedded,$(TARGET)),main.elf,main)

# Examples - JUST ADD NEW EXAMPLES HERE!
EXAMPLES = identify-parse identify-encode test-parse test-encode test-indefinite

# Auto-generate example paths
EXAMPLE_SRCS = $(addprefix $(EXAMPLES_DIR)/,$(addsuffix .c,$(EXAMPLES)))
EXAMPLE_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(EXAMPLE_SRCS))
EXAMPLE_OUTS = $(addprefix $(BUILD_DIR)/,$(if $(filter embedded,$(TARGET)),$(addsuffix .elf,$(EXAMPLES)),$(EXAMPLES)))

# Include directories
INCLUDES = -I$(LIB_DIR)

# Common compiler flags
CFLAGS += -Os
CFLAGS += -ggdb
CFLAGS += -Wall -Werror
CFLAGS += -Wextra -Wpedantic -Wshadow -Wstrict-overflow=2 -fno-strict-aliasing
CFLAGS += -Wnull-dereference -Wdouble-promotion -Wformat=2

# Target-specific compiler flags
ifeq ($(TARGET),native)
    CFLAGS += -fsanitize=undefined -fsanitize=float-divide-by-zero
    CFLAGS += -fsanitize=float-cast-overflow -fsanitize=address
    CFLAGS += -fstack-protector-strong -fstack-clash-protection
    CFLAGS += -fstack-usage  # Generate .su files for stack analysis
else ifeq ($(TARGET),embedded)
    CFLAGS += -mcpu=cortex-m3 -mthumb -mfloat-abi=soft
    CFLAGS += -ffunction-sections -fdata-sections
    CFLAGS += -fstack-usage  # Generate .su files for stack analysis
    # Aggressive optimization for stack usage
    CFLAGS += -finline-functions -finline-small-functions
    CFLAGS += -fomit-frame-pointer -foptimize-sibling-calls
endif

# Compiler-specific flags
ifeq ($(findstring clang,$(CC)),clang)
    CFLAGS += -Wno-unknown-warning-option
endif

# Linker flags
LDFLAGS = 
ifeq ($(TARGET),embedded)
	CFILES += qemu/startup.c qemu/semihosting.c
	CFLAGS += -DTARGET_EMBEDDED
	INCLUDES += -Iqemu
    LDFLAGS += -T qemu/cortex-m3.ld -nostartfiles
    LDFLAGS += -specs=nosys.specs -Wl,--gc-sections
    LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(@F:.elf=.map)
endif

# Default goal
.DEFAULT_GOAL := all

# Phony targets
.PHONY: all clean dirs $(EXAMPLES)

# Create build directory
$(BUILD_DIR):
	@mkdir -p $@
	@mkdir -p $(BUILD_DIR)/$(LIB_DIR)
	@mkdir -p $(BUILD_DIR)/qemu
	@mkdir -p $(BUILD_DIR)/$(EXAMPLES_DIR)

# Build everything
all: dirs $(MAIN_OUT) $(EXAMPLE_OUTS)

# Main target
$(MAIN_OUT): $(MAIN_OBJ) $(CFILES_OBJ) | dirs
	$(LD) $(CFLAGS) $(LDFLAGS) $^ -o $@

# Pattern rule for ALL examples - NO NEED TO ADD NEW RULES!
$(BUILD_DIR)/%: $(BUILD_DIR)/$(EXAMPLES_DIR)/%.o $(CFILES_OBJ) | dirs
	$(LD) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/%.elf: $(BUILD_DIR)/$(EXAMPLES_DIR)/%.o $(CFILES_OBJ) | dirs
	$(LD) $(CFLAGS) $(LDFLAGS) $^ -o $@

# Pattern rule for compiling C files - FIXED DIRECTORY CREATION
$(BUILD_DIR)/%.o: %.c | dirs
	@mkdir -p $(@D)  # This ensures the output directory exists
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Individual example targets (e.g., 'make parse-identify')
$(EXAMPLES): %: $(BUILD_DIR)/% | dirs

# Ensure directories exist
dirs: $(BUILD_DIR)

# Clean build artifacts
clean:
	rm -rf build

# Help target
help:
	@echo "Available targets:"
	@echo "  all           - Build everything (default)"
	@echo "  clean         - Remove entire build directory"
	@echo "  help          - Show this help message"
	@echo "  $(EXAMPLES)   - Build individual examples"
	@echo "  memory-report - Generate memory analysis report"
	@echo "  stack-analysis - Analyze stack usage from .su files"
	@echo "  stress-tests  - Generate and run stress tests"
	@echo ""
	@echo "Usage:"
	@echo "  make TARGET=native        # Build for native (default)"
	@echo "  make TARGET=embedded      # Build for embedded"
	@echo "  make parse-identify       # Build only parse-identify example"
	@echo "  make new-example          # Build only new-example"
	@echo ""
	@echo "Build artifacts go to: build/$(TARGET)/"

# Memory analysis targets
memory-report: all
	@echo "🔍 Generating comprehensive memory report..."
	@if [ -x tools/memory_analyzer.py ]; then \
		python3 tools/memory_analyzer.py --build-dir $(BUILD_DIR) --target $(TARGET); \
	else \
		echo "❌ tools/memory_analyzer.py not found or not executable"; \
	fi

stack-analysis: all
	@echo "📊 Analyzing stack usage files..."
	@echo "Stack usage files (.su):"
	@find $(BUILD_DIR) -name "*.su" -exec echo "  {}" \; -exec head -5 {} \; 2>/dev/null || echo "  No .su files found"
	@echo ""
	@echo "Map files (.map):"
	@find $(BUILD_DIR) -name "*.map" | head -3
	@echo ""
	@echo "Use 'make memory-report' for detailed analysis"

stress-tests:
	@echo "🧪 Generating stress tests..."
	@if [ -x tools/stress_test_generator.py ]; then \
		cd tools && python3 stress_test_generator.py; \
	else \
		echo "❌ tools/stress_test_generator.py not found or not executable"; \
	fi

# Build stress tests if they exist
build-stress: all
	@if [ -d stress_tests ]; then \
		echo "🔨 Building stress tests..."; \
		$(MAKE) -C stress_tests all-stress TARGET=$(TARGET); \
	else \
		echo "❌ No stress_tests directory found. Run 'make stress-tests' first."; \
	fi

run-stress: build-stress
	@if [ -d stress_tests ]; then \
		echo "🏃 Running stress tests..."; \
		$(MAKE) -C stress_tests test-stress TARGET=$(TARGET); \
	else \
		echo "❌ No stress_tests directory found. Run 'make stress-tests' first."; \
	fi

.PHONY: memory-report stack-analysis stress-tests build-stress run-stress