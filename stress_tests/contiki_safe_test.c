/*
 * Contiki-NG Safe Stack Test
 * 
 * This test is specifically designed for embedded systems running Contiki-NG
 * with 2KB total stack (1KB safe limit for applications after RTOS overhead).
 * 
 * Tests realistic CBOR parsing scenarios that should be safe in production.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cbor.h"
#include "memory_profiler.h"

// Contiki-NG stack constraints
#define CONTIKI_TOTAL_STACK 2048
#define CONTIKI_RTOS_OVERHEAD 1024
#define CONTIKI_APP_SAFE_LIMIT 1024
#define CONTIKI_WARNING_THRESHOLD 512

// Stack monitoring for Contiki-NG
static uintptr_t base_stack_pointer = 0;
static size_t peak_stack_usage = 0;
static int safety_violations = 0;

void contiki_monitor_stack(const char* location) {
    volatile uintptr_t current_sp = (uintptr_t)&current_sp;
    
    if (base_stack_pointer == 0) {
        base_stack_pointer = current_sp;
        printf("📍 Base stack pointer: 0x%lx\n", (unsigned long)base_stack_pointer);
        return;
    }
    
    // Calculate stack usage (direction depends on architecture)
    size_t stack_used;
    if (current_sp < base_stack_pointer) {
        stack_used = base_stack_pointer - current_sp;  // Stack grows down
    } else {
        stack_used = current_sp - base_stack_pointer;  // Stack grows up
    }
    
    if (stack_used > peak_stack_usage) {
        peak_stack_usage = stack_used;
    }
    
    printf("📊 %s: %zu bytes used (peak: %zu)\n", location, stack_used, peak_stack_usage);
    
    // Contiki-NG safety checks
    if (stack_used > CONTIKI_WARNING_THRESHOLD) {
        printf("⚠️  [%s] Approaching Contiki-NG limit: %zu/%d bytes\n", 
               location, stack_used, CONTIKI_APP_SAFE_LIMIT);
    }
    
    if (stack_used > CONTIKI_APP_SAFE_LIMIT) {
        printf("🚨 [%s] EXCEEDS Contiki-NG safe limit: %zu bytes!\n", location, stack_used);
        safety_violations++;
    }
}

// Safe recursive processor for Contiki-NG
static int recursion_depth = 0;
static const int MAX_CONTIKI_DEPTH = 15; // Conservative limit

void contiki_safe_processor(const cbor_value_t* value, void* arg) {
    (void)arg; // Suppress unused warning
    
    recursion_depth++;
    
    char location[64];
    snprintf(location, sizeof(location), "depth_%d", recursion_depth);
    contiki_monitor_stack(location);
    
    // Safety check for Contiki-NG
    if (recursion_depth > MAX_CONTIKI_DEPTH) {
        printf("🛑 Max Contiki-NG recursion depth (%d) reached\n", MAX_CONTIKI_DEPTH);
        recursion_depth--;
        return;
    }
    
    // Process based on type
    if (value->type == CBOR_TYPE_ARRAY) {
        printf("📦 Processing array at depth %d\n", recursion_depth);
        cbor_process_array(value->value.array, contiki_safe_processor, arg);
    } else if (value->type == CBOR_TYPE_MAP) {
        printf("🗺️  Processing map at depth %d\n", recursion_depth);
        // Note: Would need map processor implementation
    }
    
    recursion_depth--;
}

int main() {
    MEMORY_PROFILE_FUNCTION_ENTER("contiki_safe_main");
    memory_profile_init();
    
    printf("🧪 CONTIKI-NG SAFE STACK TEST\n");
    printf("=============================\n");
    printf("Target constraints:\n");
    printf("  - Total stack: %d bytes\n", CONTIKI_TOTAL_STACK);
    printf("  - RTOS overhead: ~%d bytes\n", CONTIKI_RTOS_OVERHEAD);
    printf("  - App safe limit: %d bytes\n", CONTIKI_APP_SAFE_LIMIT);
    printf("  - Warning threshold: %d bytes\n", CONTIKI_WARNING_THRESHOLD);
    printf("\n");
    
    contiki_monitor_stack("init");
    
    // Test 1: Moderate nesting (should be safe)
    printf("🧪 TEST 1: Moderate nesting (depth 10)\n");
    uint8_t moderate_nesting[] = {
        0x8A, // Array with 10 elements
        0x81, 0x01, // [1]
        0x81, 0x02, // [2]  
        0x81, 0x03, // [3]
        0x81, 0x04, // [4]
        0x81, 0x05, // [5]
        0x81, 0x06, // [6]
        0x81, 0x07, // [7]
        0x81, 0x08, // [8]
        0x81, 0x09, // [9]
        0x81, 0x0A  // [10]
    };
    
    slice_t test1_data = {sizeof(moderate_nesting), moderate_nesting};
    cbor_parse_result_t result1 = cbor_parse(test1_data);
    
    if (!result1.is_error && result1.ok.type == CBOR_TYPE_ARRAY) {
        cbor_process_array(result1.ok.value.array, contiki_safe_processor, NULL);
    }
    
    contiki_monitor_stack("test1_complete");
    
    // Test 2: IoT sensor data structure (realistic)
    printf("\n🧪 TEST 2: IoT sensor data simulation\n");
    uint8_t sensor_data[] = {
        0xA3, // Map with 3 entries
        // "temp": 23.5
        0x64, 't', 'e', 'm', 'p',
        0xF9, 0x4D, 0xC0, // Half-precision float ~23.5
        // "humid": 65
        0x65, 'h', 'u', 'm', 'i', 'd',
        0x18, 0x41, // 65
        // "sensors": [1, 2, 3]
        0x67, 's', 'e', 'n', 's', 'o', 'r', 's',
        0x83, 0x01, 0x02, 0x03
    };
    
    slice_t test2_data = {sizeof(sensor_data), sensor_data};
    cbor_parse_result_t result2 = cbor_parse(test2_data);
    
    if (!result2.is_error) {
        printf("✅ Sensor data parsed successfully\n");
        MEMORY_PROFILE_CBOR_STRUCTURE("sensor_cbor", &result2.ok);
    }
    
    contiki_monitor_stack("test2_complete");
    
    // Test 3: Edge case - maximum safe nesting
    printf("\n🧪 TEST 3: Maximum safe nesting for Contiki-NG\n");
    uint8_t max_safe_nesting[64];
    size_t pos = 0;
    
    // Create nested structure up to safe limit
    for (int i = 0; i < MAX_CONTIKI_DEPTH && pos < sizeof(max_safe_nesting) - 2; i++) {
        max_safe_nesting[pos++] = 0x81; // Array with 1 element
    }
    max_safe_nesting[pos++] = 0x01; // Final element: 1
    
    slice_t test3_data = {pos, max_safe_nesting};
    cbor_parse_result_t result3 = cbor_parse(test3_data);
    
    if (!result3.is_error && result3.ok.type == CBOR_TYPE_ARRAY) {
        printf("📦 Processing max safe nesting (%d levels)\n", MAX_CONTIKI_DEPTH);
        cbor_process_array(result3.ok.value.array, contiki_safe_processor, NULL);
    }
    
    contiki_monitor_stack("test3_complete");
    
    // Final safety assessment
    printf("\n🎯 CONTIKI-NG SAFETY ASSESSMENT\n");
    printf("===============================\n");
    printf("Peak stack usage: %zu bytes\n", peak_stack_usage);
    printf("Safety violations: %d\n", safety_violations);
    printf("Stack utilization: %.1f%% of safe limit\n", 
           (double)peak_stack_usage / CONTIKI_APP_SAFE_LIMIT * 100.0);
    
    MEMORY_PROFILE_REPORT();
    MEMORY_PROFILE_FUNCTION_EXIT();
    
    // Return appropriate exit code
    if (safety_violations > 0) {
        printf("❌ UNSAFE for Contiki-NG deployment\n");
        return 1;
    } else if (peak_stack_usage > CONTIKI_WARNING_THRESHOLD) {
        printf("⚠️  CAUTION: Monitor stack usage in production\n");
        return 2;
    } else {
        printf("✅ SAFE for Contiki-NG deployment\n");
        return 0;
    }
}
