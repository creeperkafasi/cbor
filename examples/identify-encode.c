#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cbor.h"
#include "debug.h"

#include "identify.h"

uint8_t buf[512] = {0};

custom_encoder_result_t encode_device_info(slice_t target, void* arg) {

    device_info_t* device = (device_info_t*)arg;

    cbor_value_t value = {
        .type = CBOR_ENCODE_TYPE_PAIRS,
        .value.pairs = PAIRS((
            (cbor_pair_t[]){
                {
                    {
                        .type = CBOR_TYPE_TEXT_STRING,
                        .value.bytes = STR2SLICE("f"),
                    },
                    {
                        .type = CBOR_TYPE_TEXT_STRING,
                        .value.bytes = device->f,
                    },
                },
                {
                    {
                        .type = CBOR_TYPE_TEXT_STRING,
                        .value.bytes = STR2SLICE("sn"),
                    },
                    {
                        .type = CBOR_TYPE_TEXT_STRING,
                        .value.bytes = device->sn,
                    },
                },
            }
        ))
    };

    cbor_encode_result_t encoded = cbor_encode(value, target);

    if (encoded.is_error) {
        return ERR(custom_encoder_result_t, encoded.err);
    }

    return OK(custom_encoder_result_t, encoded.ok);

}

custom_encoder_result_t encode_identification_request(slice_t target, void* arg) {
    identification_request_t* req = (identification_request_t*)arg;
    cbor_value_t  value = (cbor_value_t){
        .type = CBOR_ENCODE_TYPE_PAIRS,
        .value.pairs = PAIRS((
            (cbor_pair_t[]){
                {
                    {
                        .type = CBOR_TYPE_TEXT_STRING,
                        .value.bytes = STR2SLICE("d"),
                    },
                    {
                        .type = CBOR_ENCODE_TYPE_CUSTOM_ENCODER,
                        .value.custom_encoder = {
                            .encoder = encode_device_info,
                            .argument = &req->d
                        }
                    }
                },
                {
                    {
                        .type = CBOR_TYPE_TEXT_STRING,
                        .value.bytes = STR2SLICE("fn"),
                    },
                    {
                        .type = CBOR_TYPE_INTEGER,
                        .value.integer = req->fn,
                    },
                },
                {
                    {
                        .type = CBOR_TYPE_TEXT_STRING,
                        .value.bytes = STR2SLICE("rid"),
                    },
                    {
                        .type = CBOR_TYPE_INTEGER,
                        .value.integer = req->rid,
                    },
                },
            }
        ))
    };

    cbor_encode_result_t encoded = cbor_encode(value, target);

    if (encoded.is_error) {
        return ERR(custom_encoder_result_t, encoded.err);
    }

    return OK(custom_encoder_result_t, encoded.ok);
}


int main(void) {
    slice_t cbor = {
        .len = sizeof(buf),
        .ptr = &buf[0]
    };

    identification_request_t request1 = {
        .d = {
            .f = BUF2SLICE("XYZ"),
            .sn = BUF2SLICE("123456789"),
        },
        .fn = 42,
        .rid = 1756887865,
    };

    identification_request_t request2 = {
        .d = {
            .f = BUF2SLICE("XYZ"),
            .sn = BUF2SLICE("234567890"),
        },
        .fn = 12,
        .rid = 1756887865,
    };

    cbor_encode_result_t res = cbor_encode((cbor_value_t) {
        .type = CBOR_ENCODE_TYPE_VALUES,
        .value.values = VALUES((
            (cbor_value_t[]){
                {
                    .type = CBOR_ENCODE_TYPE_CUSTOM_ENCODER,
                    .value.custom_encoder = {
                        .encoder = encode_identification_request,
                        .argument = &request1
                    }
                },
                {
                    .type = CBOR_ENCODE_TYPE_CUSTOM_ENCODER,
                    .value.custom_encoder = {
                        .encoder = encode_identification_request,
                        .argument = &request2
                    }
                }
            }
        )),
    }, cbor);

    if (res.is_error) {
        printf("Error: %d\n", res.err);
        return res.err;
    }

    print_slice_hex(res.ok);

    return 0;
}
