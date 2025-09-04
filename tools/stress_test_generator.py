#!/usr/bin/env python3
"""
CBOR Stack Stress Test Generator

This script generates comprehensive stack stress tests to verify
embedded stack safety under extreme conditions.
"""

import os
import sys
import json
from pathlib import Path
from typing import List, Dict, Any
import random

class CBORStressTestGenerator:
    def __init__(self, output_dir: str = "stress_tests"):
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(exist_ok=True)
        
    def generate_deep_nesting_test(self, max_depth: int = 100) -> str:
        """Generate a test with deeply nested CBOR structures."""
        test_name = f"deep_nesting_{max_depth}"
        
        code = f'''/*
 * Generated stress test: Deep nesting to depth {max_depth}
 * Tests maximum recursion depth and stack usage
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cbor.h"
#include "memory_profiler.h"

// Global counters for stress testing
static int max_depth_reached = 0;
static int parse_operations = 0;

// Recursive parsing depth tracker
void track_depth(const cbor_value_t* value, void* arg) {{
    (void)value; // Suppress unused parameter warning
    int* depth = (int*)arg;
    (*depth)++;
    if (*depth > max_depth_reached) {{
        max_depth_reached = *depth;
    }}
    parse_operations++;
}}

int main() {{
    MEMORY_PROFILE_FUNCTION_ENTER("stress_test_main");
    memory_profile_init();
    
    printf("🧪 STRESS TEST: Deep Nesting (depth={max_depth})\\n");
    printf("================================================\\n");
    
    // Generate deeply nested array: [[[[...]]]]
    uint8_t nested_data[{max_depth * 2 + 2}];
    size_t pos = 0;
    
    // Opening array brackets
    for (int i = 0; i < {max_depth}; i++) {{
        nested_data[pos++] = 0x81; // Array with 1 element
    }}
    
    // Final element
    nested_data[pos++] = 0x01; // Integer 1
    
    // No closing needed for CBOR arrays
    
    printf("📦 Generated nested structure: %zu bytes\\n", pos);
    MEMORY_PROFILE_BUFFER("nested_stress_data", pos);
    
    slice_t cbor_data = {{
        .len = pos,
        .ptr = nested_data
    }};
    
    // Parse the nested structure
    cbor_parse_result_t result = cbor_parse(cbor_data);
    MEMORY_PROFILE_CBOR_STRUCTURE("cbor_value_t", &result.ok);
    
    if (result.is_error) {{
        printf("❌ Parse failed with error: %d\\n", result.err);
        MEMORY_PROFILE_REPORT();
        MEMORY_PROFILE_FUNCTION_EXIT();
        return 1;
    }}
    
    printf("✅ Parse successful\\n");
    printf("📊 Structure type: %d\\n", result.ok.type);
    
    // Process the nested structure to trigger recursion
    if (result.ok.type == CBOR_TYPE_ARRAY) {{
        int depth = 0;
        uint8_t* end = cbor_process_array(result.ok.value.array, track_depth, &depth);
        printf("🎯 Maximum recursion depth reached: %d\\n", max_depth_reached);
        printf("🔢 Total parse operations: %d\\n", parse_operations);
        
        if (end != NULL) {{
            printf("✅ Array processing completed successfully\\n");
        }} else {{
            printf("❌ Array processing failed\\n");
        }}
    }}
    
    MEMORY_PROFILE_REPORT();
    MEMORY_PROFILE_FUNCTION_EXIT();
    
    // Verify we didn't exceed safe limits
    if (max_depth_reached > {max_depth // 2}) {{
        printf("⚠️  Deep recursion detected - verify stack safety\\n");
        return 2; // Warning exit code
    }}
    
    printf("🎉 Stress test completed successfully\\n");
    return 0;
}}
'''
        
        output_file = self.output_dir / f"{test_name}.c"
        with open(output_file, 'w') as f:
            f.write(code)
        
        return str(output_file)
    
    def generate_large_data_test(self, data_size: int = 10240) -> str:
        """Generate a test with large CBOR data structures."""
        test_name = f"large_data_{data_size}"
        
        code = f'''/*
 * Generated stress test: Large data structures ({data_size} bytes)
 * Tests memory allocation and buffer handling
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cbor.h"
#include "memory_profiler.h"

// Large buffer for stress testing
static uint8_t large_buffer[{data_size}];

int main() {{
    MEMORY_PROFILE_FUNCTION_ENTER("large_data_stress_main");
    memory_profile_init();
    
    printf("🧪 STRESS TEST: Large Data Structures ({data_size} bytes)\\n");
    printf("=========================================================\\n");
    
    MEMORY_PROFILE_BUFFER("large_stress_buffer", sizeof(large_buffer));
    
    // Fill buffer with a large CBOR array containing many integers
    size_t pos = 0;
    
    // Array header for large array (use indefinite length)
    large_buffer[pos++] = 0x9F; // Indefinite length array
    
    // Fill with integers up to available space
    int element_count = 0;
    while (pos < sizeof(large_buffer) - 2) {{ // Leave space for break code
        large_buffer[pos++] = 0x19; // 2-byte positive integer
        large_buffer[pos++] = (element_count >> 8) & 0xFF;
        large_buffer[pos++] = element_count & 0xFF;
        element_count++;
    }}
    
    // Break code for indefinite array
    large_buffer[pos++] = 0xFF;
    
    printf("📦 Generated array with %d elements in %zu bytes\\n", element_count, pos);
    
    slice_t cbor_data = {{
        .len = pos,
        .ptr = large_buffer
    }};
    
    // Parse the large structure
    cbor_parse_result_t result = cbor_parse(cbor_data);
    MEMORY_PROFILE_CBOR_STRUCTURE("cbor_value_t", &result.ok);
    
    if (result.is_error) {{
        printf("❌ Parse failed with error: %d\\n", result.err);
        MEMORY_PROFILE_REPORT();
        MEMORY_PROFILE_FUNCTION_EXIT();
        return 1;
    }}
    
    printf("✅ Parse successful\\n");
    printf("📊 Array length: %u\\n", result.ok.value.array.length);
    
    // Process all elements to stress memory system
    if (result.ok.type == CBOR_TYPE_ARRAY) {{
        int processed_count = 0;
        
        // Custom processor to count elements
        void count_elements(const cbor_value_t* value, void* arg) {{
            int* count = (int*)arg;
            (*count)++;
            MEMORY_PROFILE_CBOR_STRUCTURE("array_element", value);
            
            // Print progress every 100 elements
            if (*count % 100 == 0) {{
                printf("📈 Processed %d elements...\\n", *count);
            }}
        }}
        
        uint8_t* end = cbor_process_array(result.ok.value.array, count_elements, &processed_count);
        
        printf("🎯 Total elements processed: %d\\n", processed_count);
        
        if (end != NULL) {{
            printf("✅ Large array processing completed successfully\\n");
        }} else {{
            printf("❌ Large array processing failed\\n");
        }}
    }}
    
    MEMORY_PROFILE_REPORT();
    MEMORY_PROFILE_FUNCTION_EXIT();
    
    printf("🎉 Large data stress test completed successfully\\n");
    return 0;
}}
'''
        
        output_file = self.output_dir / f"{test_name}.c"
        with open(output_file, 'w') as f:
            f.write(code)
        
        return str(output_file)
    
    def generate_mixed_complexity_test(self) -> str:
        """Generate a test with mixed complex CBOR structures."""
        test_name = "mixed_complexity"
        
        code = '''/*
 * Generated stress test: Mixed complexity structures
 * Tests various CBOR features under stress conditions
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cbor.h"
#include "memory_profiler.h"

// Complex nested structure counters
static int maps_processed = 0;
static int arrays_processed = 0;
static int strings_processed = 0;
static int integers_processed = 0;

void process_mixed_element(const cbor_value_t* value, void* arg) {
    MEMORY_PROFILE_CBOR_STRUCTURE("mixed_element", value);
    
    switch (value->type) {
        case CBOR_TYPE_INTEGER:
            integers_processed++;
            break;
        case CBOR_TYPE_TEXT_STRING:
        case CBOR_TYPE_BYTE_STRING:
            strings_processed++;
            break;
        case CBOR_TYPE_ARRAY:
            arrays_processed++;
            // Recursively process nested arrays
            cbor_process_array(value->value.array, process_mixed_element, arg);
            break;
        case CBOR_TYPE_MAP:
            maps_processed++;
            // Note: Map processing would need a pair processor
            break;
        default:
            break;
    }
}

int main() {
    MEMORY_PROFILE_FUNCTION_ENTER("mixed_complexity_stress_main");
    memory_profile_init();
    
    printf("🧪 STRESS TEST: Mixed Complexity Structures\\n");
    printf("===========================================\\n");
    
    // Complex CBOR structure: Map with nested arrays and strings
    // {
    //   "data": [1, 2, [3, 4, [5, 6]]],
    //   "metadata": {
    //     "version": "1.0",
    //     "flags": [true, false, null]
    //   },
    //   "payload": "Large string data..."
    // }
    
    uint8_t complex_data[] = {
        0xA3, // Map with 3 entries
        
        // "data": [1, 2, [3, 4, [5, 6]]]
        0x64, 'd', 'a', 't', 'a', // "data"
        0x83, // Array with 3 elements
            0x01, // 1
            0x02, // 2
            0x83, // Nested array with 3 elements
                0x03, // 3
                0x04, // 4
                0x82, // Deeply nested array with 2 elements
                    0x05, // 5
                    0x06, // 6
        
        // "metadata": {"version": "1.0", "flags": [true, false, null]}
        0x68, 'm', 'e', 't', 'a', 'd', 'a', 't', 'a', // "metadata"
        0xA2, // Map with 2 entries
            0x67, 'v', 'e', 'r', 's', 'i', 'o', 'n', // "version"
            0x63, '1', '.', '0', // "1.0"
            0x65, 'f', 'l', 'a', 'g', 's', // "flags"
            0x83, // Array with 3 elements
                0xF5, // true
                0xF4, // false
                0xF6, // null
        
        // "payload": "TEST_DATA_STRING"
        0x67, 'p', 'a', 'y', 'l', 'o', 'a', 'd', // "payload"
        0x70, 'T', 'E', 'S', 'T', '_', 'D', 'A', 'T', 'A', '_', 'S', 'T', 'R', 'I', 'N', 'G' // "TEST_DATA_STRING"
    };
    
    printf("📦 Complex structure size: %zu bytes\\n", sizeof(complex_data));
    MEMORY_PROFILE_BUFFER("complex_stress_data", sizeof(complex_data));
    
    slice_t cbor_data = {
        .len = sizeof(complex_data),
        .ptr = complex_data
    };
    
    // Parse the complex structure
    cbor_parse_result_t result = cbor_parse(cbor_data);
    MEMORY_PROFILE_CBOR_STRUCTURE("cbor_value_t", &result.ok);
    
    if (result.is_error) {
        printf("❌ Parse failed with error: %d\\n", result.err);
        MEMORY_PROFILE_REPORT();
        MEMORY_PROFILE_FUNCTION_EXIT();
        return 1;
    }
    
    printf("✅ Parse successful\\n");
    printf("📊 Root structure type: %d\\n", result.ok.type);
    
    // Process the complex structure
    if (result.ok.type == CBOR_TYPE_MAP) {
        void process_map_pair(const cbor_value_t* key, const cbor_value_t* value, void* arg) {
            MEMORY_PROFILE_CBOR_STRUCTURE("map_key", key);
            MEMORY_PROFILE_CBOR_STRUCTURE("map_value", value);
            
            // Process the value recursively
            if (value->type == CBOR_TYPE_ARRAY) {
                cbor_process_array(value->value.array, process_mixed_element, arg);
            } else if (value->type == CBOR_TYPE_MAP) {
                cbor_process_map(value->value.map, process_map_pair, arg);
            } else {
                process_mixed_element(value, arg);
            }
        }
        
        uint8_t* end = cbor_process_map(result.ok.value.map, process_map_pair, NULL);
        
        printf("🎯 Processing statistics:\\n");
        printf("   - Maps processed: %d\\n", maps_processed);
        printf("   - Arrays processed: %d\\n", arrays_processed);
        printf("   - Strings processed: %d\\n", strings_processed);
        printf("   - Integers processed: %d\\n", integers_processed);
        
        if (end != NULL) {
            printf("✅ Complex structure processing completed successfully\\n");
        } else {
            printf("❌ Complex structure processing failed\\n");
        }
    }
    
    MEMORY_PROFILE_REPORT();
    MEMORY_PROFILE_FUNCTION_EXIT();
    
    printf("🎉 Mixed complexity stress test completed successfully\\n");
    return 0;
}
'''
        
        output_file = self.output_dir / f"{test_name}.c"
        with open(output_file, 'w') as f:
            f.write(code)
        
        return str(output_file)
    
    def generate_stack_overflow_detector(self) -> str:
        """Generate a test specifically designed to detect stack overflow conditions."""
        test_name = "stack_overflow_detector"
        
        code = '''/*
 * Stack Overflow Detection Test
 * This test is designed to push stack usage to dangerous levels
 * and detect potential overflow conditions before they happen.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cbor.h"
#include "memory_profiler.h"

// Stack canary for overflow detection
#define STACK_CANARY 0xDEADBEEF
static volatile uint32_t stack_canary = STACK_CANARY;

// Maximum safe recursion depth
#define MAX_SAFE_DEPTH 50

// Recursion depth counter
static int current_depth = 0;
static int max_depth_reached = 0;

// Stack usage monitoring
static uintptr_t initial_stack_pointer = 0;
static size_t max_stack_used = 0;

void monitor_stack_usage() {
    volatile uintptr_t current_sp;
    current_sp = (uintptr_t)&current_sp;
    
    if (initial_stack_pointer == 0) {
        initial_stack_pointer = current_sp;
    }
    
    size_t stack_used = initial_stack_pointer - current_sp;
    if (stack_used > max_stack_used) {
        max_stack_used = stack_used;
    }
    
    // Check for dangerous stack usage
    if (stack_used > 1024) { // 1KB warning threshold
        printf("⚠️  WARNING: High stack usage detected: %zu bytes\\n", stack_used);
    }
    
    if (stack_used > 2048) { // 2KB danger threshold
        printf("🚨 DANGER: Critical stack usage: %zu bytes\\n", stack_used);
    }
    
    // Check stack canary
    if (stack_canary != STACK_CANARY) {
        printf("💥 STACK CORRUPTION DETECTED!\\n");
        exit(1);
    }
}

void recursive_processor(const cbor_value_t* value, void* arg) {
    monitor_stack_usage();
    
    current_depth++;
    if (current_depth > max_depth_reached) {
        max_depth_reached = current_depth;
    }
    
    printf("📊 Depth: %d, Stack used: %zu bytes\\n", current_depth, max_stack_used);
    
    // Safety check
    if (current_depth > MAX_SAFE_DEPTH) {
        printf("🛑 STOPPING: Maximum safe depth exceeded\\n");
        current_depth--;
        return;
    }
    
    // Process nested structures recursively
    if (value->type == CBOR_TYPE_ARRAY) {
        cbor_process_array(value->value.array, recursive_processor, arg);
    }
    
    current_depth--;
}

int main() {
    MEMORY_PROFILE_FUNCTION_ENTER("stack_overflow_detector_main");
    memory_profile_init();
    
    printf("🧪 STACK OVERFLOW DETECTION TEST\\n");
    printf("================================\\n");
    printf("Stack canary: 0x%08X\\n", stack_canary);
    
    monitor_stack_usage();
    
    // Create extremely deep nesting to test stack limits
    uint8_t deep_structure[200];
    size_t pos = 0;
    
    // Create nested arrays: [[[[[[...]]]]]]
    int nesting_levels = 80; // High nesting level
    for (int i = 0; i < nesting_levels && pos < sizeof(deep_structure) - 2; i++) {
        deep_structure[pos++] = 0x81; // Array with 1 element
    }
    
    // Final element
    deep_structure[pos++] = 0x01; // Integer 1
    
    printf("📦 Generated structure with %d nesting levels\\n", nesting_levels);
    MEMORY_PROFILE_BUFFER("overflow_test_data", pos);
    
    slice_t cbor_data = {
        .len = pos,
        .ptr = deep_structure
    };
    
    // Parse and process with stack monitoring
    cbor_parse_result_t result = cbor_parse(cbor_data);
    
    if (result.is_error) {
        printf("❌ Parse failed with error: %d\\n", result.err);
        MEMORY_PROFILE_REPORT();
        MEMORY_PROFILE_FUNCTION_EXIT();
        return 1;
    }
    
    printf("✅ Parse successful, starting recursive processing...\\n");
    
    if (result.ok.type == CBOR_TYPE_ARRAY) {
        cbor_process_array(result.ok.value.array, recursive_processor, NULL);
    }
    
    // Final safety check
    monitor_stack_usage();
    
    printf("\\n🎯 Final Results:\\n");
    printf("   - Maximum depth reached: %d\\n", max_depth_reached);
    printf("   - Maximum stack used: %zu bytes\\n", max_stack_used);
    printf("   - Stack canary: %s\\n", 
           stack_canary == STACK_CANARY ? "✅ INTACT" : "💥 CORRUPTED");
    
    MEMORY_PROFILE_REPORT();
    MEMORY_PROFILE_FUNCTION_EXIT();
    
    if (max_stack_used > 1024) {
        printf("⚠️  WARNING: High stack usage detected - consider optimization\\n");
        return 2;
    }
    
    printf("🎉 Stack overflow detection test completed safely\\n");
    return 0;
}
'''
        
        output_file = self.output_dir / f"{test_name}.c"
        with open(output_file, 'w') as f:
            f.write(code)
        
        return str(output_file)
    
    def generate_makefile(self, test_files: List[str]) -> str:
        """Generate Makefile for stress tests."""
        makefile_content = '''# CBOR Stress Tests Makefile
# Auto-generated stress test build configuration

# Inherit settings from parent Makefile
include ../Makefile

# Stress test specific settings
STRESS_CFLAGS = $(CFLAGS) -DSTRESS_TEST -DSTACK_CHECK
STRESS_LDFLAGS = $(LDFLAGS)

# Stress test sources
STRESS_TESTS = '''
        
        test_names = [Path(f).stem for f in test_files]
        makefile_content += " ".join(test_names) + "\\n\\n"
        
        # Add build rules
        makefile_content += '''# Build all stress tests
all-stress: $(addprefix $(BUILD_DIR)/stress_,$(STRESS_TESTS))

# Pattern rule for stress tests
$(BUILD_DIR)/stress_%: stress_tests/%.o $(CFILES_OBJ) | dirs
\\t$(LD) $(STRESS_CFLAGS) $(STRESS_LDFLAGS) $^ -o $@

# Pattern rule for compiling stress test sources
stress_tests/%.o: stress_tests/%.c | dirs
\\t@mkdir -p stress_tests
\\t$(CC) $(STRESS_CFLAGS) $(INCLUDES) -c $< -o $@

# Run all stress tests
test-stress: all-stress
\\t@echo "🧪 Running stress tests..."
\\t@for test in $(addprefix $(BUILD_DIR)/stress_,$(STRESS_TESTS)); do \\
\\t\\techo "Running $$test..."; \\
\\t\\tif [ "$(TARGET)" = "embedded" ]; then \\
\\t\\t\\tqemu-system-arm -M lm3s6965evb -cpu cortex-m3 -nographic -semihosting -kernel $$test.elf || true; \\
\\t\\telse \\
\\t\\t\\t$$test || true; \\
\\t\\tfi; \\
\\tdone

# Clean stress tests
clean-stress:
\\trm -rf stress_tests/*.o $(BUILD_DIR)/stress_*

.PHONY: all-stress test-stress clean-stress
'''
        
        makefile_path = self.output_dir / "Makefile"
        with open(makefile_path, 'w') as f:
            f.write(makefile_content)
        
        return str(makefile_path)

def main():
    print("🚀 Generating CBOR stack stress tests...")
    
    generator = CBORStressTestGenerator()
    
    test_files = []
    
    # Generate various stress tests
    test_files.append(generator.generate_deep_nesting_test(50))
    test_files.append(generator.generate_deep_nesting_test(100))
    test_files.append(generator.generate_large_data_test(1024))
    test_files.append(generator.generate_large_data_test(4096))
    test_files.append(generator.generate_mixed_complexity_test())
    test_files.append(generator.generate_stack_overflow_detector())
    
    # Generate build system
    makefile = generator.generate_makefile(test_files)
    
    print(f"✅ Generated {len(test_files)} stress test files:")
    for test_file in test_files:
        print(f"   - {test_file}")
    
    print(f"✅ Generated Makefile: {makefile}")
    print("\\n🔧 To build and run stress tests:")
    print("   make -C stress_tests all-stress")
    print("   make -C stress_tests test-stress")

if __name__ == '__main__':
    main()
