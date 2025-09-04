/*
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

// Maximum safe recursion depth (conservative for Contiki-NG with 2KB total stack)
#define MAX_SAFE_DEPTH 25

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
    
    // Check for dangerous stack usage (Contiki-NG constraints: 2KB total, 1KB max for app)
    if (stack_used > 512) { // 512 bytes warning threshold (50% of 1KB limit)
        printf("⚠️  WARNING: High stack usage detected: %zu bytes\n", stack_used);
    }
    
    if (stack_used > 1024) { // 1KB danger threshold (absolute max for Contiki-NG)
        printf("🚨 DANGER: Critical stack usage: %zu bytes - EXCEEDS CONTIKI-NG LIMIT!\n", stack_used);
    }
    
    // Check stack canary
    if (stack_canary != STACK_CANARY) {
        printf("💥 STACK CORRUPTION DETECTED!\n");
        exit(1);
    }
}

void recursive_processor(const cbor_value_t* value, void* arg) {
    monitor_stack_usage();
    
    current_depth++;
    if (current_depth > max_depth_reached) {
        max_depth_reached = current_depth;
    }
    
    printf("📊 Depth: %d, Stack used: %zu bytes\n", current_depth, max_stack_used);
    
    // Safety check
    if (current_depth > MAX_SAFE_DEPTH) {
        printf("🛑 STOPPING: Maximum safe depth exceeded\n");
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
    
    printf("🧪 STACK OVERFLOW DETECTION TEST\n");
    printf("================================\n");
    printf("Stack canary: 0x%08X\n", stack_canary);
    
    monitor_stack_usage();
    
    // Create extremely deep nesting to test stack limits
    uint8_t deep_structure[200];
    size_t pos = 0;
    
    // Create nested arrays: [[[[[[...]]]]]] (reduced for Contiki-NG)
    int nesting_levels = 30; // Reduced nesting level for embedded constraints
    for (int i = 0; i < nesting_levels && pos < sizeof(deep_structure) - 2; i++) {
        deep_structure[pos++] = 0x81; // Array with 1 element
    }
    
    // Final element
    deep_structure[pos++] = 0x01; // Integer 1
    
    printf("📦 Generated structure with %d nesting levels\n", nesting_levels);
    MEMORY_PROFILE_BUFFER("overflow_test_data", pos);
    
    slice_t cbor_data = {
        .len = pos,
        .ptr = deep_structure
    };
    
    // Parse and process with stack monitoring
    cbor_parse_result_t result = cbor_parse(cbor_data);
    
    if (result.is_error) {
        printf("❌ Parse failed with error: %d\n", result.err);
        MEMORY_PROFILE_REPORT();
        MEMORY_PROFILE_FUNCTION_EXIT();
        return 1;
    }
    
    printf("✅ Parse successful, starting recursive processing...\n");
    
    if (result.ok.type == CBOR_TYPE_ARRAY) {
        cbor_process_array(result.ok.value.array, recursive_processor, NULL);
    }
    
    // Final safety check
    monitor_stack_usage();
    
    printf("\n🎯 Final Results:\n");
    printf("   - Maximum depth reached: %d\n", max_depth_reached);
    printf("   - Maximum stack used: %zu bytes\n", max_stack_used);
    printf("   - Stack canary: %s\n", 
           stack_canary == STACK_CANARY ? "✅ INTACT" : "💥 CORRUPTED");
    
    MEMORY_PROFILE_REPORT();
    MEMORY_PROFILE_FUNCTION_EXIT();
    
    if (max_stack_used > 1024) {
        printf("⚠️  WARNING: High stack usage detected - EXCEEDS CONTIKI-NG SAFE LIMIT\n");
        return 2;
    }
    
    if (max_stack_used > 512) {
        printf("⚠️  CAUTION: Moderate stack usage - monitor in production\n");
        return 1;
    }
    
    printf("🎉 Stack overflow detection test completed safely\n");
    return 0;
}
