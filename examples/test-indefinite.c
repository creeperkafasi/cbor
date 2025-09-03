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

// Helper function to compare byte arrays
int compare_bytes(const uint8_t* actual, const uint8_t* expected, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (actual[i] != expected[i]) {
            printf("Mismatch at byte %zu: got 0x%02X, expected 0x%02X\n", i, actual[i], expected[i]);
            return 0;
        }
    }
    return 1;
}

void print_bytes(const uint8_t* data, size_t len) {
    printf("Bytes: ");
    for (size_t i = 0; i < len; i++) {
        printf("0x%02X ", data[i]);
    }
    printf("\n");
}

// Callback functions for testing
void count_elements(const cbor_value_t* value, void* arg) {
    (void)value; // Unused
    int* count = (int*)arg;
    (*count)++;
}

void count_pairs(const cbor_value_t* key, const cbor_value_t* value, void* arg) {
    (void)key; (void)value; // Unused
    int* count = (int*)arg;
    (*count)++;
}

// Test indefinite length array encoding
void test_indefinite_array_encoding() {
    printf("\n=== Testing Indefinite Array Encoding ===\n");
    
    uint8_t buffer[64];
    slice_t target = {.len = sizeof(buffer), .ptr = buffer};
    
    // Create array elements
    cbor_value_t elements[] = {
        {.type = CBOR_TYPE_INTEGER, .value.integer = 1},
        {.type = CBOR_TYPE_INTEGER, .value.integer = 2},
        {.type = CBOR_TYPE_INTEGER, .value.integer = 3}
    };
    
    // Create indefinite length array
    cbor_value_t indefinite_array = CBOR_INDEFINITE_ARRAY(elements);
    
    cbor_encode_result_t result = cbor_encode(indefinite_array, target);
    TEST_ASSERT(!result.is_error, "Indefinite array should encode without error");
    
    if (!result.is_error) {
        // Expected: 0x9F (indefinite array start), 0x01, 0x02, 0x03, 0xFF (break)
        uint8_t expected[] = {0x9F, 0x01, 0x02, 0x03, 0xFF};
        printf("Expected encoding: ");
        print_bytes(expected, sizeof(expected));
        printf("Actual encoding:   ");
        print_bytes(result.ok.ptr, result.ok.len);
        
        TEST_ASSERT(result.ok.len == sizeof(expected), "Indefinite array length should match expected");
        TEST_ASSERT(compare_bytes(result.ok.ptr, expected, sizeof(expected)), "Indefinite array bytes should match expected");
    }
}

// Test indefinite length map encoding
void test_indefinite_map_encoding() {
    printf("\n=== Testing Indefinite Map Encoding ===\n");
    
    uint8_t buffer[64];
    slice_t target = {.len = sizeof(buffer), .ptr = buffer};
    
    // Create map pairs
    cbor_value_t key1 = {.type = CBOR_TYPE_TEXT_STRING, .value.bytes = STR2SLICE("a")};
    cbor_value_t val1 = {.type = CBOR_TYPE_INTEGER, .value.integer = 1};
    cbor_value_t key2 = {.type = CBOR_TYPE_TEXT_STRING, .value.bytes = STR2SLICE("b")};
    cbor_value_t val2 = {.type = CBOR_TYPE_INTEGER, .value.integer = 2};
    
    cbor_pair_t pairs[] = {
        {.first = key1, .second = val1},
        {.first = key2, .second = val2}
    };
    
    // Create indefinite length map
    cbor_value_t indefinite_map = CBOR_INDEFINITE_MAP(pairs);
    
    cbor_encode_result_t result = cbor_encode(indefinite_map, target);
    TEST_ASSERT(!result.is_error, "Indefinite map should encode without error");
    
    if (!result.is_error) {
        // Expected: 0xBF (indefinite map start), 0x61 0x61, 0x01, 0x61 0x62, 0x02, 0xFF (break)
        uint8_t expected[] = {0xBF, 0x61, 0x61, 0x01, 0x61, 0x62, 0x02, 0xFF};
        printf("Expected encoding: ");
        print_bytes(expected, sizeof(expected));
        printf("Actual encoding:   ");
        print_bytes(result.ok.ptr, result.ok.len);
        
        TEST_ASSERT(result.ok.len == sizeof(expected), "Indefinite map length should match expected");
        TEST_ASSERT(compare_bytes(result.ok.ptr, expected, sizeof(expected)), "Indefinite map bytes should match expected");
    }
}

// Test parsing of indefinite length array
void test_indefinite_array_parsing() {
    printf("\n=== Testing Indefinite Array Parsing ===\n");
    
    // CBOR data: indefinite array [1, 2, 3]
    uint8_t cbor_data[] = {0x9F, 0x01, 0x02, 0x03, 0xFF};
    slice_t input = {.len = sizeof(cbor_data), .ptr = cbor_data};
    
    cbor_parse_result_t result = cbor_parse(input);
    TEST_ASSERT(!result.is_error, "Indefinite array should parse without error");
    
    if (!result.is_error) {
        TEST_ASSERT(result.ok.type == CBOR_TYPE_ARRAY, "Parsed value should be an array");
        TEST_ASSERT(result.ok.value.array.length == UINT32_MAX, "Array should be marked as indefinite length");
        
        // Test processing the array
        int element_count = 0;
        
        uint8_t* end = cbor_process_array(result.ok.value.array, count_elements, &element_count);
        TEST_ASSERT(end != NULL, "Array processing should succeed");
        TEST_ASSERT(element_count == 3, "Should process 3 elements");
        TEST_ASSERT(end == cbor_data + sizeof(cbor_data), "Should end at the correct position");
    }
}

// Test parsing of indefinite length map
void test_indefinite_map_parsing() {
    printf("\n=== Testing Indefinite Map Parsing ===\n");
    
    // CBOR data: indefinite map {"a": 1, "b": 2}
    uint8_t cbor_data[] = {0xBF, 0x61, 0x61, 0x01, 0x61, 0x62, 0x02, 0xFF};
    slice_t input = {.len = sizeof(cbor_data), .ptr = cbor_data};
    
    cbor_parse_result_t result = cbor_parse(input);
    TEST_ASSERT(!result.is_error, "Indefinite map should parse without error");
    
    if (!result.is_error) {
        TEST_ASSERT(result.ok.type == CBOR_TYPE_MAP, "Parsed value should be a map");
        TEST_ASSERT(result.ok.value.map.length == UINT32_MAX, "Map should be marked as indefinite length");
        
        // Test processing the map
        int pair_count = 0;
        
        uint8_t* end = cbor_process_map(result.ok.value.map, count_pairs, &pair_count);
        TEST_ASSERT(end != NULL, "Map processing should succeed");
        TEST_ASSERT(pair_count == 2, "Should process 2 pairs");
        TEST_ASSERT(end == cbor_data + sizeof(cbor_data), "Should end at the correct position");
    }
}

// Test indefinite length text string encoding
void test_indefinite_text_string_encoding() {
    printf("\n=== Testing Indefinite Text String Encoding ===\n");
    
    uint8_t buffer[64];
    slice_t target = {.len = sizeof(buffer), .ptr = buffer};
    
    // Create text string chunks
    cbor_value_t chunks[] = {
        {.type = CBOR_TYPE_TEXT_STRING, .value.bytes = STR2SLICE("hello")},
        {.type = CBOR_TYPE_TEXT_STRING, .value.bytes = STR2SLICE("world")}
    };
    
    // Create indefinite length text string
    cbor_value_t indefinite_string = CBOR_INDEFINITE_TEXT_STRING(chunks);
    
    cbor_encode_result_t result = cbor_encode(indefinite_string, target);
    TEST_ASSERT(!result.is_error, "Indefinite text string should encode without error");
    
    if (!result.is_error) {
        // Expected: 0x7F (indefinite text string), 0x65 "hello", 0x65 "world", 0xFF (break)
        uint8_t expected[] = {0x7F, 0x65, 0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x65, 0x77, 0x6F, 0x72, 0x6C, 0x64, 0xFF};
        printf("Expected encoding: ");
        print_bytes(expected, sizeof(expected));
        printf("Actual encoding:   ");
        print_bytes(result.ok.ptr, result.ok.len);
        
        TEST_ASSERT(result.ok.len == sizeof(expected), "Indefinite text string length should match expected");
        TEST_ASSERT(compare_bytes(result.ok.ptr, expected, sizeof(expected)), "Indefinite text string bytes should match expected");
    }
}

// Test indefinite length byte string encoding
void test_indefinite_byte_string_encoding() {
    printf("\n=== Testing Indefinite Byte String Encoding ===\n");
    
    uint8_t buffer[64];
    slice_t target = {.len = sizeof(buffer), .ptr = buffer};
    
    // Create byte string chunks
    uint8_t data1[] = {0x01, 0x02};
    uint8_t data2[] = {0x03, 0x04};
    
    cbor_value_t chunks[] = {
        {.type = CBOR_TYPE_BYTE_STRING, .value.bytes = {.len = sizeof(data1), .ptr = data1}},
        {.type = CBOR_TYPE_BYTE_STRING, .value.bytes = {.len = sizeof(data2), .ptr = data2}}
    };
    
    // Create indefinite length byte string
    cbor_value_t indefinite_string = CBOR_INDEFINITE_BYTE_STRING(chunks);
    
    cbor_encode_result_t result = cbor_encode(indefinite_string, target);
    TEST_ASSERT(!result.is_error, "Indefinite byte string should encode without error");
    
    if (!result.is_error) {
        // Expected: 0x5F (indefinite byte string), 0x42 0x01 0x02, 0x42 0x03 0x04, 0xFF (break)
        uint8_t expected[] = {0x5F, 0x42, 0x01, 0x02, 0x42, 0x03, 0x04, 0xFF};
        printf("Expected encoding: ");
        print_bytes(expected, sizeof(expected));
        printf("Actual encoding:   ");
        print_bytes(result.ok.ptr, result.ok.len);
        
        TEST_ASSERT(result.ok.len == sizeof(expected), "Indefinite byte string length should match expected");
        TEST_ASSERT(compare_bytes(result.ok.ptr, expected, sizeof(expected)), "Indefinite byte string bytes should match expected");
    }
}

// Test indefinite length text string parsing
void test_indefinite_text_string_parsing() {
    printf("\n=== Testing Indefinite Text String Parsing ===\n");
    
    // CBOR data: indefinite text string "hello" + "world"
    uint8_t cbor_data[] = {0x7F, 0x65, 0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x65, 0x77, 0x6F, 0x72, 0x6C, 0x64, 0xFF};
    slice_t input = {.len = sizeof(cbor_data), .ptr = cbor_data};
    
    cbor_parse_result_t result = cbor_parse(input);
    TEST_ASSERT(!result.is_error, "Indefinite text string should parse without error");
    
    if (!result.is_error) {
        TEST_ASSERT(result.ok.type == CBOR_TYPE_TEXT_STRING, "Parsed value should be a text string");
        TEST_ASSERT(result.ok.argument.tag == ARGUMENT_NONE, "String should be marked as indefinite length");
        
        // Test processing the string chunks
        int chunk_count = 0;
        
        uint8_t* end = cbor_process_indefinite_string(result.ok.value.array, CBOR_TYPE_TEXT_STRING, count_elements, &chunk_count);
        TEST_ASSERT(end != NULL, "String processing should succeed");
        TEST_ASSERT(chunk_count == 2, "Should process 2 chunks");
        TEST_ASSERT(end == cbor_data + sizeof(cbor_data), "Should end at the correct position");
    }
}

// Test indefinite length byte string parsing
void test_indefinite_byte_string_parsing() {
    printf("\n=== Testing Indefinite Byte String Parsing ===\n");
    
    // CBOR data: indefinite byte string [0x01, 0x02] + [0x03, 0x04]
    uint8_t cbor_data[] = {0x5F, 0x42, 0x01, 0x02, 0x42, 0x03, 0x04, 0xFF};
    slice_t input = {.len = sizeof(cbor_data), .ptr = cbor_data};
    
    cbor_parse_result_t result = cbor_parse(input);
    TEST_ASSERT(!result.is_error, "Indefinite byte string should parse without error");
    
    if (!result.is_error) {
        TEST_ASSERT(result.ok.type == CBOR_TYPE_BYTE_STRING, "Parsed value should be a byte string");
        TEST_ASSERT(result.ok.argument.tag == ARGUMENT_NONE, "String should be marked as indefinite length");
        
        // Test processing the string chunks
        int chunk_count = 0;
        
        uint8_t* end = cbor_process_indefinite_string(result.ok.value.array, CBOR_TYPE_BYTE_STRING, count_elements, &chunk_count);
        TEST_ASSERT(end != NULL, "String processing should succeed");
        TEST_ASSERT(chunk_count == 2, "Should process 2 chunks");
        TEST_ASSERT(end == cbor_data + sizeof(cbor_data), "Should end at the correct position");
    }
}

int main() {
    printf("Testing Indefinite Length CBOR Support\n");
    printf("=====================================\n");
    
    test_indefinite_array_encoding();
    test_indefinite_map_encoding();
    test_indefinite_text_string_encoding();
    test_indefinite_byte_string_encoding();
    
    test_indefinite_array_parsing();
    test_indefinite_map_parsing();
    test_indefinite_text_string_parsing();
    test_indefinite_byte_string_parsing();
    
    printf("\n=== Test Results ===\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    if (tests_failed == 0) {
        printf("🎉 All tests passed!\n");
        return 0;
    } else {
        printf("❌ Some tests failed!\n");
        return 1;
    }
}
