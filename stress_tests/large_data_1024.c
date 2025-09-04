/*
 * Generated stress test: Large data structures (1024 bytes)
 *        // Custom processor to count elements
        void count_elements(const cbor_value_t* value, void* arg) {
            (void)value; // Suppress unused parameter warning
            int* count = (int*)arg;
            (*count)++;
            MEMORY_PROFILE_CBOR_STRUCTURE("array_element", value);
            
            // Print progress every 100 elements
            if (*count % 100 == 0) {
                printf("📈 Processed %d elements...\\n", *count);
            }
        }ry allocation and buffer handling
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cbor.h"
#include "memory_profiler.h"

// Large buffer for stress testing
static uint8_t large_buffer[1024];

int main() {
    MEMORY_PROFILE_FUNCTION_ENTER("large_data_stress_main");
    memory_profile_init();
    
    printf("🧪 STRESS TEST: Large Data Structures (1024 bytes)\n");
    printf("=========================================================\n");
    
    MEMORY_PROFILE_BUFFER("large_stress_buffer", sizeof(large_buffer));
    
    // Fill buffer with a large CBOR array containing many integers
    size_t pos = 0;
    
    // Array header for large array (use indefinite length)
    large_buffer[pos++] = 0x9F; // Indefinite length array
    
    // Fill with integers up to available space
    int element_count = 0;
    while (pos < sizeof(large_buffer) - 2) { // Leave space for break code
        large_buffer[pos++] = 0x19; // 2-byte positive integer
        large_buffer[pos++] = (element_count >> 8) & 0xFF;
        large_buffer[pos++] = element_count & 0xFF;
        element_count++;
    }
    
    // Break code for indefinite array
    large_buffer[pos++] = 0xFF;
    
    printf("📦 Generated array with %d elements in %zu bytes\n", element_count, pos);
    
    slice_t cbor_data = {
        .len = pos,
        .ptr = large_buffer
    };
    
    // Parse the large structure
    cbor_parse_result_t result = cbor_parse(cbor_data);
    MEMORY_PROFILE_CBOR_STRUCTURE("cbor_value_t", &result.ok);
    
    if (result.is_error) {
        printf("❌ Parse failed with error: %d\n", result.err);
        MEMORY_PROFILE_REPORT();
        MEMORY_PROFILE_FUNCTION_EXIT();
        return 1;
    }
    
    printf("✅ Parse successful\n");
    printf("📊 Array length: %u\n", result.ok.value.array.length);
    
    // Process all elements to stress memory system
    if (result.ok.type == CBOR_TYPE_ARRAY) {
        int processed_count = 0;
        
        // Custom processor to count elements
        void count_elements(const cbor_value_t* value, void* arg) {
            int* count = (int*)arg;
            (*count)++;
            MEMORY_PROFILE_CBOR_STRUCTURE("array_element", value);
            
            // Print progress every 100 elements
            if (*count % 100 == 0) {
                printf("📈 Processed %d elements...\n", *count);
            }
        }
        
        uint8_t* end = cbor_process_array(result.ok.value.array, count_elements, &processed_count);
        
        printf("🎯 Total elements processed: %d\n", processed_count);
        
        if (end != NULL) {
            printf("✅ Large array processing completed successfully\n");
        } else {
            printf("❌ Large array processing failed\n");
        }
    }
    
    MEMORY_PROFILE_REPORT();
    MEMORY_PROFILE_FUNCTION_EXIT();
    
    printf("🎉 Large data stress test completed successfully\n");
    return 0;
}
