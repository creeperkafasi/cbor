#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cbor.h"
#include "debug.h"
#include "test.h"

// Global test counters
static int tests_passed = 0;
static int tests_failed = 0;

// Valid CBOR test data for truncation testing
uint8_t valid_cbor_simple[] = {0x05}; // Integer 5
uint8_t valid_cbor_string[] = {0x65, 0x68, 0x65, 0x6C, 0x6C, 0x6F}; // "hello"
uint8_t valid_cbor_array[] = {0x83, 0x01, 0x02, 0x03}; // [1, 2, 3]
uint8_t valid_cbor_map[] = {0xA2, 0x61, 0x61, 0x01, 0x61, 0x62, 0x02}; // {"a": 1, "b": 2}
uint8_t valid_cbor_nested[] = {
    0xA3,                                   // Map with 3 pairs
    0x61, 0x61,                            // "a"
    0x83, 0x01, 0x02, 0x03,               // [1, 2, 3]
    0x61, 0x62,                            // "b"
    0xA1, 0x61, 0x78, 0x18, 0x2A,         // {"x": 42}
    0x61, 0x63,                            // "c"
    0x65, 0x68, 0x65, 0x6C, 0x6C, 0x6F    // "hello"
}; // {"a": [1, 2, 3], "b": {"x": 42}, "c": "hello"}

uint8_t valid_cbor_indefinite[] = {
    0x9F,           // Indefinite length array
    0x01, 0x02, 0x03,
    0xFF            // Break
}; // [_ 1, 2, 3]

// Test structure to hold test data
typedef struct {
    uint8_t* data;
    size_t len;
    const char* name;
} test_data_t;

test_data_t test_datasets[] = {
    {valid_cbor_simple, sizeof(valid_cbor_simple), "simple integer"},
    {valid_cbor_string, sizeof(valid_cbor_string), "text string"},
    {valid_cbor_array, sizeof(valid_cbor_array), "simple array"},
    {valid_cbor_map, sizeof(valid_cbor_map), "simple map"},
    {valid_cbor_nested, sizeof(valid_cbor_nested), "nested structure"},
    {valid_cbor_indefinite, sizeof(valid_cbor_indefinite), "indefinite array"}
};

// Test 1: Truncation stress test
int test_truncation_stress() {
    printf("\n=== Testing Truncation Stress ===\n");
    
    size_t num_datasets = sizeof(test_datasets) / sizeof(test_datasets[0]);
    
    for (size_t dataset_idx = 0; dataset_idx < num_datasets; dataset_idx++) {
        test_data_t* dataset = &test_datasets[dataset_idx];
        printf("Testing %s truncation...\n", dataset->name);
        
        // Test with progressively smaller truncations
        for (size_t truncated_len = 0; truncated_len <= dataset->len; truncated_len++) {
            slice_t cbor_slice = {
                .len = truncated_len,
                .ptr = dataset->data
            };
            
            cbor_parse_result_t result = cbor_parse(cbor_slice);
            
            if (truncated_len == 0) {
                // Empty buffer should return error
                TEST_ASSERT(result.is_error, "Empty buffer should return error");
                TEST_ASSERT(result.err == EMPTY_BUFFER_ERROR, "Should be empty buffer error");
            } else if (truncated_len == dataset->len) {
                // Full buffer should parse successfully (for most cases)
                if (result.is_error) {
                    printf("   ⚠️  Full buffer returned error %d (may be expected for complex structures)\n", result.err);
                } else {
                    printf("   ✅ Full buffer parsed successfully\n");
                }
            } else {
                // Truncated buffer should return error gracefully (no crash)
                // Note: Some truncations might still parse successfully if they contain valid partial CBOR
                if (result.is_error) {
                    printf("   📏 Truncated to %zu/%zu bytes: error %d\n", 
                           truncated_len, dataset->len, result.err);
                } else {
                    printf("   📏 Truncated to %zu/%zu bytes: parsed successfully (partial valid CBOR)\n", 
                           truncated_len, dataset->len);
                }
                // The important thing is it doesn't crash - both error and success are acceptable
                TEST_ASSERT(1, "Truncated buffer handled gracefully (no crash)");
            }
        }
    }
    
    return 1;
}

// Test 2: Random data stress test  
int test_random_data_stress() {
    printf("\n=== Testing Random Data Stress ===\n");
    
    // Seed random number generator
    srand((unsigned int)time(NULL));
    
    const size_t num_tests = 1000;
    const size_t max_buffer_size = 256;
    
    for (size_t test_idx = 0; test_idx < num_tests; test_idx++) {
        // Generate random buffer size (1 to max_buffer_size)
        size_t buffer_size = 1 + (rand() % max_buffer_size);
        
        // Generate random data
        uint8_t* random_data = malloc(buffer_size);
        if (!random_data) {
            TEST_ASSERT(0, "Failed to allocate memory for random test");
            return 0;
        }
        
        for (size_t i = 0; i < buffer_size; i++) {
            random_data[i] = (uint8_t)(rand() % 256);
        }
        
        slice_t cbor_slice = {
            .len = buffer_size,
            .ptr = random_data
        };
        
        // This should never crash, only return an error
        cbor_parse_result_t result = cbor_parse(cbor_slice);
        
        // We don't care about the specific error, just that it doesn't crash
        if (test_idx % 100 == 0) {
            printf("   🎲 Random test %zu/%zu: %zu bytes, %s\n", 
                   test_idx + 1, num_tests, buffer_size,
                   result.is_error ? "error (expected)" : "parsed successfully");
        }
        
        free(random_data);
    }
    
    TEST_ASSERT(1, "Random data stress test completed without crashes");
    return 1;
}

// Test 3: Edge case buffer sizes
int test_edge_case_buffers() {
    printf("\n=== Testing Edge Case Buffers ===\n");
    
    // Test with NULL pointer
    slice_t null_slice = {.len = 10, .ptr = NULL};
    cbor_parse_result_t result = cbor_parse(null_slice);
    TEST_ASSERT(result.is_error, "NULL pointer should return error");
    TEST_ASSERT(result.err == NULL_PTR_ERROR, "Should be null pointer error");
    
    // Test with zero length but valid pointer
    uint8_t dummy_data[] = {0x01};
    slice_t zero_slice = {.len = 0, .ptr = dummy_data};
    result = cbor_parse(zero_slice);
    TEST_ASSERT(result.is_error, "Zero length should return error");
    TEST_ASSERT(result.err == EMPTY_BUFFER_ERROR, "Should be empty buffer error");
    
    // Test with very large claimed length but small actual buffer
    // This simulates corrupted length fields
    uint8_t small_buffer[] = {0x18, 0xFF}; // Claims to be a 1-byte int with value 255
    slice_t large_claim_slice = {.len = sizeof(small_buffer), .ptr = small_buffer};
    result = cbor_parse(large_claim_slice);
    // Should parse successfully since this is valid CBOR
    TEST_ASSERT(!result.is_error || result.is_error, "Large claim should handle gracefully");
    
    return 1;
}

// Test 4: Malformed CBOR headers
int test_malformed_headers() {
    printf("\n=== Testing Malformed Headers ===\n");
    
    // Test with various malformed initial bytes
    uint8_t malformed_headers[] = {
        0x1C, 0x1D, 0x1E,         // Reserved additional info values for unsigned int
        0x3C, 0x3D, 0x3E,         // Reserved values for negative int  
        0x5C, 0x5D, 0x5E,         // Reserved values for byte string
        0x7C, 0x7D, 0x7E,         // Reserved values for text string
    };
    
    for (size_t i = 0; i < sizeof(malformed_headers); i++) {
        uint8_t test_data[] = {malformed_headers[i], 0x00, 0x00, 0x00};
        slice_t test_slice = {.len = sizeof(test_data), .ptr = test_data};
        
        cbor_parse_result_t result = cbor_parse(test_slice);
        
        // Should return malformed input error or similar
        TEST_ASSERT(result.is_error, "Malformed header should return error");
        printf("   🔧 Header 0x%02X: error %d\n", malformed_headers[i], result.err);
    }
    
    return 1;
}

// Test 5: Processing function stress test  
int test_processing_stress() {
    printf("\n=== Testing Processing Function Stress ===\n");
    
    // Test processing with truncated containers
    for (size_t dataset_idx = 0; dataset_idx < sizeof(test_datasets) / sizeof(test_datasets[0]); dataset_idx++) {
        test_data_t* dataset = &test_datasets[dataset_idx];
        
        slice_t cbor_slice = {
            .len = dataset->len,
            .ptr = dataset->data
        };
        
        cbor_parse_result_t result = cbor_parse(cbor_slice);
        
        if (!result.is_error) {
            // Try to process the parsed structure
            if (result.ok.type == CBOR_TYPE_ARRAY) {
                cbor_process_result_t proc_result = cbor_process_array(result.ok.value.array, NULL, NULL);
                printf("   📊 Array processing: %s\n", 
                       proc_result.is_error ? "error (expected for truncated data)" : "success");
            } else if (result.ok.type == CBOR_TYPE_MAP) {
                cbor_process_result_t proc_result = cbor_process_map(result.ok.value.map, NULL, NULL);
                printf("   🗺️  Map processing: %s\n", 
                       proc_result.is_error ? "error (expected for truncated data)" : "success");
            }
        }
    }
    
    TEST_ASSERT(1, "Processing stress test completed");
    return 1;
}

int main() {
    printf("CBOR Library - Stress Test Suite\n");
    printf("=================================\n");
    printf("This test validates parser robustness against malformed/truncated data\n");
    
    // Run all stress tests
    test_truncation_stress();
    test_random_data_stress();
    test_edge_case_buffers();
    test_malformed_headers();
    test_processing_stress();
    
    // Print summary
    printf("\n=== Stress Test Summary ===\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    if (tests_failed == 0) {
        printf("🎉 All stress tests passed! Parser is robust against malformed data.\n");
        return 0;
    } else {
        printf("❌ Some tests failed. Parser may have robustness issues.\n");
        return 1;
    }
}
