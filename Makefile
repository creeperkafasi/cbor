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
CFILES = $(LIB_DIR)/cbor.c $(LIB_DIR)/debug.c
CFILES_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(CFILES))

# Main application
MAIN_SRC = $(SRC_DIR)/main.c
MAIN_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(MAIN_SRC))
MAIN_OUT = $(BUILD_DIR)/$(if $(filter embedded,$(TARGET)),main.elf,main)

# Examples - JUST ADD NEW EXAMPLES HERE!
EXAMPLES = parse-identify encode-identify test-parse test-encode test-indefinite

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
else ifeq ($(TARGET),embedded)
    CFLAGS += -mcpu=cortex-m3 -mthumb -mfloat-abi=soft
    CFLAGS += -ffunction-sections -fdata-sections
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
	@echo ""
	@echo "Usage:"
	@echo "  make TARGET=native        # Build for native (default)"
	@echo "  make TARGET=embedded      # Build for embedded"
	@echo "  make parse-identify       # Build only parse-identify example"
	@echo "  make new-example          # Build only new-example"
	@echo ""
	@echo "Build artifacts go to: build/$(TARGET)/"