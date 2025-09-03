#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cbor.h"
#include "debug.h"

// Test result tracking
static int tests_passed = 0;
static int tests_failed = 0;

// Helper counters for array/map processing tests
static int element_count = 0;
static int pair_count = 0;

// Helper functions for testing array/map processing
void count_elements(const cbor_value_t* value, void* arg) {
    (void)value; (void)arg; // Suppress unused warnings
    element_count++;
}

void count_pairs(const cbor_value_t* key, const cbor_value_t* value, void* arg) {
    (void)key; (void)value; (void)arg; // Suppress unused warnings
    pair_count++;
}

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            printf("✅ PASS: %s\n", message); \
            tests_passed++; \
        } else { \
            printf("❌ FAIL: %s\n", message); \
            tests_failed++; \
        } \
    } while(0)

// Test 1: Basic integer parsing
void test_integer_parsing() {
    printf("\n=== Testing Integer Parsing ===\n");
    
    // Test positive small integer (0-23 range)
    uint8_t small_int[] = {0x05}; // Integer 5
    slice_t buf = {.len = sizeof(small_int), .ptr = small_int};
    
    cbor_parse_result_t result = cbor_parse(buf);
    TEST_ASSERT(!result.is_error, "Small integer should parse without error");
    TEST_ASSERT(result.ok.type == CBOR_TYPE_INTEGER, "Should be integer type");
    TEST_ASSERT(result.ok.value.integer == 5, "Should parse value 5");
    
    // Test larger positive integer (24-255 range)
    uint8_t medium_int[] = {0x18, 0x64}; // Integer 100
    buf = (slice_t){.len = sizeof(medium_int), .ptr = medium_int};
    
    result = cbor_parse(buf);
    TEST_ASSERT(!result.is_error, "Medium integer should parse without error");
    TEST_ASSERT(result.ok.value.integer == 100, "Should parse value 100");
    
    // Test negative integer
    uint8_t neg_int[] = {0x20}; // Integer -1
    buf = (slice_t){.len = sizeof(neg_int), .ptr = neg_int};
    
    result = cbor_parse(buf);
    TEST_ASSERT(!result.is_error, "Negative integer should parse without error");
    TEST_ASSERT(result.ok.value.integer == -1, "Should parse value -1");
}

// Test 2: String parsing
void test_string_parsing() {
    printf("\n=== Testing String Parsing ===\n");
    
    // Test text string
    uint8_t text_str[] = {0x65, 'h', 'e', 'l', 'l', 'o'}; // "hello"
    slice_t buf = {.len = sizeof(text_str), .ptr = text_str};
    
    cbor_parse_result_t result = cbor_parse(buf);
    TEST_ASSERT(!result.is_error, "Text string should parse without error");
    TEST_ASSERT(result.ok.type == CBOR_TYPE_TEXT_STRING, "Should be text string type");
    TEST_ASSERT(result.ok.value.bytes.len == 5, "String length should be 5");
    TEST_ASSERT(memcmp(result.ok.value.bytes.ptr, "hello", 5) == 0, "String content should be 'hello'");
    
    // Test byte string
    uint8_t byte_str[] = {0x44, 0x01, 0x02, 0x03, 0x04}; // 4 bytes: [1,2,3,4]
    buf = (slice_t){.len = sizeof(byte_str), .ptr = byte_str};
    
    result = cbor_parse(buf);
    TEST_ASSERT(!result.is_error, "Byte string should parse without error");
    TEST_ASSERT(result.ok.type == CBOR_TYPE_BYTE_STRING, "Should be byte string type");
    TEST_ASSERT(result.ok.value.bytes.len == 4, "Byte string length should be 4");
    
    // Test empty string
    uint8_t empty_str[] = {0x60}; // Empty text string
    buf = (slice_t){.len = sizeof(empty_str), .ptr = empty_str};
    
    result = cbor_parse(buf);
    TEST_ASSERT(!result.is_error, "Empty string should parse without error");
    TEST_ASSERT(result.ok.value.bytes.len == 0, "Empty string length should be 0");
}

// Test 3: Simple values and floats
void test_simple_values() {
    printf("\n=== Testing Simple Values ===\n");
    
    // Test false
    uint8_t false_val[] = {0xF4};
    slice_t buf = {.len = sizeof(false_val), .ptr = false_val};
    
    cbor_parse_result_t result = cbor_parse(buf);
    TEST_ASSERT(!result.is_error, "False value should parse without error");
    TEST_ASSERT(result.ok.type == CBOR_TYPE_SIMPLE, "Should be simple type");
    TEST_ASSERT(result.ok.value.simple == CBOR_SIMPLE_FALSE, "Should be false value");
    
    // Test true
    uint8_t true_val[] = {0xF5};
    buf = (slice_t){.len = sizeof(true_val), .ptr = true_val};
    
    result = cbor_parse(buf);
    TEST_ASSERT(!result.is_error, "True value should parse without error");
    TEST_ASSERT(result.ok.value.simple == CBOR_SIMPLE_TRUE, "Should be true value");
    
    // Test null
    uint8_t null_val[] = {0xF6};
    buf = (slice_t){.len = sizeof(null_val), .ptr = null_val};
    
    result = cbor_parse(buf);
    TEST_ASSERT(!result.is_error, "Null value should parse without error");
    TEST_ASSERT(result.ok.value.simple == CBOR_SIMPLE_NULL, "Should be null value");
    
    // Test undefined
    uint8_t undef_val[] = {0xF7};
    buf = (slice_t){.len = sizeof(undef_val), .ptr = undef_val};
    
    result = cbor_parse(buf);
    TEST_ASSERT(!result.is_error, "Undefined value should parse without error");
    TEST_ASSERT(result.ok.value.simple == CBOR_SIMPLE_UNDEFINED, "Should be undefined value");
}

// Test 4: Array parsing
void test_array_parsing() {
    printf("\n=== Testing Array Parsing ===\n");
    
    // Test empty array
    uint8_t empty_array[] = {0x80}; // Array of length 0
    slice_t buf = {.len = sizeof(empty_array), .ptr = empty_array};
    
    cbor_parse_result_t result = cbor_parse(buf);
    TEST_ASSERT(!result.is_error, "Empty array should parse without error");
    TEST_ASSERT(result.ok.type == CBOR_TYPE_ARRAY, "Should be array type");
    TEST_ASSERT(result.ok.value.array.length == 0, "Array length should be 0");
    
    // Test simple array [1, 2, 3]
    uint8_t simple_array[] = {0x83, 0x01, 0x02, 0x03}; // Array with 3 elements: 1, 2, 3
    buf = (slice_t){.len = sizeof(simple_array), .ptr = simple_array};
    
    result = cbor_parse(buf);
    TEST_ASSERT(!result.is_error, "Simple array should parse without error");
    TEST_ASSERT(result.ok.type == CBOR_TYPE_ARRAY, "Should be array type");
    TEST_ASSERT(result.ok.value.array.length == 3, "Array length should be 3");
    
    // Test processing array elements
    element_count = 0; // Reset counter
    
    uint8_t* end_ptr = cbor_process_array(result.ok.value.array, count_elements, NULL);
    TEST_ASSERT(end_ptr != NULL, "Array processing should succeed");
    TEST_ASSERT(element_count == 3, "Should process 3 elements");
}

// Test 5: Map parsing
void test_map_parsing() {
    printf("\n=== Testing Map Parsing ===\n");
    
    // Test empty map
    uint8_t empty_map[] = {0xA0}; // Map of length 0
    slice_t buf = {.len = sizeof(empty_map), .ptr = empty_map};
    
    cbor_parse_result_t result = cbor_parse(buf);
    TEST_ASSERT(!result.is_error, "Empty map should parse without error");
    TEST_ASSERT(result.ok.type == CBOR_TYPE_MAP, "Should be map type");
    TEST_ASSERT(result.ok.value.map.length == 0, "Map length should be 0");
    
    // Test simple map {"a": 1}
    uint8_t simple_map[] = {0xA1, 0x61, 'a', 0x01}; // Map with 1 pair: "a" -> 1
    buf = (slice_t){.len = sizeof(simple_map), .ptr = simple_map};
    
    result = cbor_parse(buf);
    TEST_ASSERT(!result.is_error, "Simple map should parse without error");
    TEST_ASSERT(result.ok.type == CBOR_TYPE_MAP, "Should be map type");
    TEST_ASSERT(result.ok.value.map.length == 1, "Map length should be 1");
    
    // Test processing map pairs
    pair_count = 0; // Reset counter
    
    uint8_t* end_ptr = cbor_process_map(result.ok.value.map, count_pairs, NULL);
    TEST_ASSERT(end_ptr != NULL, "Map processing should succeed");
    TEST_ASSERT(pair_count == 1, "Should process 1 pair");
}

int main() {
    printf("CBOR Library - Parsing Test Suite\n");
    printf("==================================\n");
    
    test_integer_parsing();
    test_string_parsing();
    test_simple_values();
    test_array_parsing();
    test_map_parsing();
    
    printf("\n=== Test Summary ===\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
