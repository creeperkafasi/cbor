#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cbor.h"
#include "debug.h"

// uint8_t buf[] = {
// 0xA3, 0x61, 0x64, 0xA2, 0x61, 0x66, 0x63, 0x58,
// 0x59, 0x5A, 0x62, 0x73, 0x6E, 0x6F, 0x30, 0x31,
// 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
// 0x41, 0x42, 0x43, 0x44, 0x45, 0x62, 0x66, 0x6E,
// 0x02, 0x63, 0x72, 0x69, 0x64, 0x78, 0x24, 0x33,
// 0x64, 0x30, 0x62, 0x32, 0x34, 0x32, 0x65, 0x2D,
// 0x31, 0x38, 0x36, 0x36, 0x2D, 0x34, 0x61, 0x34,
// 0x31, 0x2D, 0x61, 0x38, 0x63, 0x61, 0x2D, 0x31,
// 0x33, 0x37, 0x32, 0x66, 0x31, 0x62, 0x33, 0x34,
// 0x61, 0x62, 0x37,
// };

uint8_t buf[512] = {0};

typedef struct {
    slice_t f;
    slice_t sn;
    int number;
} device_info_t;

typedef struct {
    device_info_t d;
    int fn;
    slice_t rid;
} identification_request_t;


int main() {
    slice_t cbor = {
        .len = sizeof(buf),
        .ptr = &buf[0]
    };

    cbor_value_t array[] = {
        (cbor_value_t) {
            .type = CBOR_TYPE_TEXT_STRING,
            .value.bytes = BUF2SLICE("hello"),
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_BYTE_STRING,
            .value.bytes = BUF2SLICE("world"),
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_INTEGER,
            .value.integer = 42
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_INTEGER,
            .value.integer = -727
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_SIMPLE,
            .value.simple = CBOR_SIMPLE_FALSE
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_SIMPLE,
            .value.simple = CBOR_SIMPLE_UNDEFINED
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_FLOAT,
            .value.floating = 3.14159265357989
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_TEXT_STRING,
            .value.bytes = BUF2SLICE("A longer text string that exceeds 23 characters")
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_TEXT_STRING,
            .value.bytes = BUF2SLICE("")
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_BYTE_STRING,
            .value.bytes = (slice_t){
                .ptr = (uint8_t*)"\0" "\n" "\u1234",
                .len = 3
            }
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_INTEGER,
            .value.integer = 1
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_INTEGER,
            .value.integer = 2
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_INTEGER,
            .value.integer = 3
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_INTEGER,
            .value.integer = 4
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_INTEGER,
            .value.integer = 5
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_INTEGER,
            .value.integer = 6
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_INTEGER,
            .value.integer = 7
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_INTEGER,
            .value.integer = 8
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_INTEGER,
            .value.integer = 9
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_INTEGER,
            .value.integer = 10
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_INTEGER,
            .value.integer = 11
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_INTEGER,
            .value.integer = 12
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_INTEGER,
            .value.integer = 13
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_INTEGER,
            .value.integer = 14
        },
        (cbor_value_t) {
            .type = CBOR_TYPE_INTEGER,
            .value.integer = 15
        },
    };

    encode_result_t res = encode_array(array, sizeof(array) / sizeof(array[0]), cbor);

    if (res.is_error) {
        printf("Error: %d\n", res.err);
        return res.err;
    }

    print_slice_hex(res.ok);

    return 0;
}