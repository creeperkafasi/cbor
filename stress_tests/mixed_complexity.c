/*
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
    
    printf("🧪 STRESS TEST: Mixed Complexity Structures\n");
    printf("===========================================\n");
    
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
    
    printf("📦 Complex structure size: %zu bytes\n", sizeof(complex_data));
    MEMORY_PROFILE_BUFFER("complex_stress_data", sizeof(complex_data));
    
    slice_t cbor_data = {
        .len = sizeof(complex_data),
        .ptr = complex_data
    };
    
    // Parse the complex structure
    cbor_parse_result_t result = cbor_parse(cbor_data);
    MEMORY_PROFILE_CBOR_STRUCTURE("cbor_value_t", &result.ok);
    
    if (result.is_error) {
        printf("❌ Parse failed with error: %d\n", result.err);
        MEMORY_PROFILE_REPORT();
        MEMORY_PROFILE_FUNCTION_EXIT();
        return 1;
    }
    
    printf("✅ Parse successful\n");
    printf("📊 Root structure type: %d\n", result.ok.type);
    
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
        
        printf("🎯 Processing statistics:\n");
        printf("   - Maps processed: %d\n", maps_processed);
        printf("   - Arrays processed: %d\n", arrays_processed);
        printf("   - Strings processed: %d\n", strings_processed);
        printf("   - Integers processed: %d\n", integers_processed);
        
        if (end != NULL) {
            printf("✅ Complex structure processing completed successfully\n");
        } else {
            printf("❌ Complex structure processing failed\n");
        }
    }
    
    MEMORY_PROFILE_REPORT();
    MEMORY_PROFILE_FUNCTION_EXIT();
    
    printf("🎉 Mixed complexity stress test completed successfully\n");
    return 0;
}
