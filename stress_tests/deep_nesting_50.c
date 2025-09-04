/*
 * Generated stress test: Deep nesting to depth 50
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
void track_depth(const cbor_value_t* value, void* arg) {
    (void)value; // Suppress unused parameter warning
    (void)value; // Suppress unused parameter warning
    int* depth = (int*)arg;
    (*depth)++;
    if (*depth > max_depth_reached) {
        max_depth_reached = *depth;
    }
    parse_operations++;
}

int main() {
    MEMORY_PROFILE_FUNCTION_ENTER("stress_test_main");
    memory_profile_init();
    
    printf("🧪 STRESS TEST: Deep Nesting (depth=50)\n");
    printf("================================================\n");
    
    // Generate deeply nested array: [[[[...]]]]
    uint8_t nested_data[102];
    size_t pos = 0;
    
    // Opening array brackets
    for (int i = 0; i < 50; i++) {
        nested_data[pos++] = 0x81; // Array with 1 element
    }
    
    // Final element
    nested_data[pos++] = 0x01; // Integer 1
    
    // No closing needed for CBOR arrays
    
    printf("📦 Generated nested structure: %zu bytes\n", pos);
    MEMORY_PROFILE_BUFFER("nested_stress_data", pos);
    
    slice_t cbor_data = {
        .len = pos,
        .ptr = nested_data
    };
    
    // Parse the nested structure
    cbor_parse_result_t result = cbor_parse(cbor_data);
    MEMORY_PROFILE_CBOR_STRUCTURE("cbor_value_t", &result.ok);
    
    if (result.is_error) {
        printf("❌ Parse failed with error: %d\n", result.err);
        MEMORY_PROFILE_REPORT();
        MEMORY_PROFILE_FUNCTION_EXIT();
        return 1;
    }
    
    printf("✅ Parse successful\n");
    printf("📊 Structure type: %d\n", result.ok.type);
    
    // Process the nested structure to trigger recursion
    if (result.ok.type == CBOR_TYPE_ARRAY) {
        int depth = 0;
        uint8_t* end = cbor_process_array(result.ok.value.array, track_depth, &depth);
        printf("🎯 Maximum recursion depth reached: %d\n", max_depth_reached);
        printf("🔢 Total parse operations: %d\n", parse_operations);
        
        if (end != NULL) {
            printf("✅ Array processing completed successfully\n");
        } else {
            printf("❌ Array processing failed\n");
        }
    }
    
    MEMORY_PROFILE_REPORT();
    MEMORY_PROFILE_FUNCTION_EXIT();
    
    // Verify we didn't exceed safe limits
    if (max_depth_reached > 25) {
        printf("⚠️  Deep recursion detected - verify stack safety\n");
        return 2; // Warning exit code
    }
    
    printf("🎉 Stress test completed successfully\n");
    return 0;
}
