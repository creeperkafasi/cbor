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


#define ARRAY_TO_SLICE(type, array) (type) { .len = sizeof(array) / sizeof(array[0]), .ptr = array }

#define VALUES(array) (ARRAY_TO_SLICE(cbor_value_slice_t, array))

#define PAIR(first, second) (cbor_pair_t) {.first = (cbor_value_t)(first), .second = (cbor_value_t)(second)}
#define PAIRS(array) ARRAY_TO_SLICE(cbor_pair_slice_t, array)

int main(void) {
    slice_t cbor = {
        .len = sizeof(buf),
        .ptr = &buf[0]
    };

    void* stack_before = __builtin_frame_address(0);
    cbor_value_t array = {
        .type = CBOR_TYPE_VALUES,
        .value.values = VALUES((
            (cbor_value_t[]){
            {
                .type = CBOR_TYPE_TEXT_STRING,
                .value.bytes = BUF2SLICE("hello"),
            },
            {
                .type = CBOR_TYPE_PAIRS,
                .value.pairs = PAIRS((
                (cbor_pair_t[]){
                    {
                        .first = {
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = BUF2SLICE("key1"),
                        },
                        .second = {
                            .type = CBOR_TYPE_INTEGER,
                            .value.integer = 42,
                        },
                    },
                    {
                        .first = {
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = BUF2SLICE("key2"),
                        },
                        .second = {
                            .type = CBOR_TYPE_INTEGER,
                            .value.integer = 84,
                        },
                    },
                }
                ))
            },
            {
                .type = CBOR_TYPE_VALUES,
                .value.values = VALUES((
                    (cbor_value_t[]){
                    {
                        .type = CBOR_TYPE_TEXT_STRING,
                        .value.bytes = BUF2SLICE("world"),
                    },
                                    {
                        .type = CBOR_TYPE_INTEGER,
                        .value.integer = 1234,
                    },
                    }
                ))
            },
            }
        ))
    };
    void* stack_after = __builtin_frame_address(0);

    printf("Stack size: %zd bytes\n", (char*)stack_after - (char*)stack_before);

    encode_result_t res = encode(array, cbor);

    if (res.is_error) {
        printf("Error: %d\n", res.err);
        return res.err;
    }

    print_slice_hex(res.ok);

    return 0;
}
