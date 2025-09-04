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

custom_encoder_result_t encode_communication_interfaces(slice_t target, void* arg) {
    (void)arg; // Suppress unused parameter warning
    // Stub: encode empty array
    cbor_value_t value = {
        .type = CBOR_ENCODE_TYPE_VALUES,
        .value.values = {.len = 0, .ptr = NULL}
    };

    cbor_encode_result_t encoded = cbor_encode(value, target);

    if (encoded.is_error) {
        return ERR(custom_encoder_result_t, encoded.err);
    }

    return OK(custom_encoder_result_t, encoded.ok);
}

custom_encoder_result_t encode_serial_ports(slice_t target, void* arg) {
    (void)arg; // Suppress unused parameter warning
    // Stub: encode empty array
    cbor_value_t value = {
        .type = CBOR_ENCODE_TYPE_VALUES,
        .value.values = {.len = 0, .ptr = NULL}
    };

    cbor_encode_result_t encoded = cbor_encode(value, target);

    if (encoded.is_error) {
        return ERR(custom_encoder_result_t, encoded.err);
    }

    return OK(custom_encoder_result_t, encoded.ok);
}

custom_encoder_result_t encode_io_interfaces(slice_t target, void* arg) {
    (void)arg; // Suppress unused parameter warning
    // Stub: encode empty array
    cbor_value_t value = {
        .type = CBOR_ENCODE_TYPE_VALUES,
        .value.values = {.len = 0, .ptr = NULL}
    };

    cbor_encode_result_t encoded = cbor_encode(value, target);

    if (encoded.is_error) {
        return ERR(custom_encoder_result_t, encoded.err);
    }

    return OK(custom_encoder_result_t, encoded.ok);
}

custom_encoder_result_t encode_meters(slice_t target, void* arg) {
    (void)arg; // Suppress unused parameter warning
    // Stub: encode empty array
    cbor_value_t value = {
        .type = CBOR_ENCODE_TYPE_VALUES,
        .value.values = {.len = 0, .ptr = NULL}
    };

    cbor_encode_result_t encoded = cbor_encode(value, target);

    if (encoded.is_error) {
        return ERR(custom_encoder_result_t, encoded.err);
    }

    return OK(custom_encoder_result_t, encoded.ok);
}

custom_encoder_result_t encode_identify_bitmap(slice_t target, void* arg) {
    identify_bitmap_t* bitmap = (identify_bitmap_t*)arg;
    slice_t current = target;

    // Count number of set bits
    size_t count = 0;
    #define X(name, key) if (*bitmap & (IDENTIFY_MASK_ ## key)) count++;
    IDENTIFY_PARAMETERS
    #undef X

    uint8_t header_size = cbor_write_len_header(count, CBOR_MAJOR_TYPE_MAP, target);
    size_t full_length = header_size;
    current.ptr += header_size;
    current.len -= header_size;

    cbor_encode_result_t encoded;
    size_t encoded_count = 0; // Track how many pairs we've actually encoded
    for (enum identify_response_shifts i = 0; i < IDENTIFY_PARAMETERS_COUNT; i++) {
        if (*bitmap & 1) {
            switch (i)
            {
                case IDENTIFY_SHIFT_REGISTERED:
                    encoded = cbor_encode_pair((cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE(CBOR_KEY_REGISTERED),
                        }, (cbor_value_t){
                            .type = CBOR_TYPE_SIMPLE,
                            .value.simple = CBOR_SIMPLE_TRUE,
                        }, current);
                    break;
                case IDENTIFY_SHIFT_BRAND:
                    encoded = cbor_encode_pair((cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE(CBOR_KEY_BRAND),
                        }, (cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE("ExampleBrand"),
                        }, current);
                    break;
                case IDENTIFY_SHIFT_MODEL:
                    encoded = cbor_encode_pair((cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE(CBOR_KEY_MODEL),
                        }, (cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE("ExampleModel"),
                        }, current);
                    break;
                case IDENTIFY_SHIFT_TYPE:
                    encoded = cbor_encode_pair((cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE(CBOR_KEY_TYPE),
                        }, (cbor_value_t){
                            .type = CBOR_TYPE_INTEGER,
                            .value.integer = 0,
                        }, current);
                    break;
                case IDENTIFY_SHIFT_PROTOCOLVERSION:
                    encoded = cbor_encode_pair((cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE(CBOR_KEY_PROTOCOLVERSION),
                        }, (cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE("1.0.0"),
                        }, current);
                    break;
                case IDENTIFY_SHIFT_MANUFACTUREDATE:
                    encoded = cbor_encode_pair((cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE(CBOR_KEY_MANUFACTUREDATE),
                        }, (cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE("2023-05-23"),
                        }, current);
                    break;
                case IDENTIFY_SHIFT_FIRMWARE:
                    encoded = cbor_encode_pair((cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE(CBOR_KEY_FIRMWARE),
                        }, (cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE("1.01"),
                        }, current);
                    break;
                case IDENTIFY_SHIFT_SIGNAL:
                    encoded = cbor_encode_pair((cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE(CBOR_KEY_SIGNAL),
                        }, (cbor_value_t){
                            .type = CBOR_TYPE_INTEGER,
                            .value.integer = 13,
                        }, current);
                    break;
                case IDENTIFY_SHIFT_HEARTBEATPERIOD:
                    encoded = cbor_encode_pair((cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE(CBOR_KEY_HEARTBEATPERIOD),
                        }, (cbor_value_t){
                            .type = CBOR_TYPE_INTEGER,
                            .value.integer = 10,
                        }, current);
                    break;
                case IDENTIFY_SHIFT_DEVICEDATE:
                    encoded = cbor_encode_pair((cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE(CBOR_KEY_DEVICEDATE),
                        }, (cbor_value_t){
                            .type = CBOR_TYPE_INTEGER,
                            .value.integer = 1672531200, // Unix timestamp example
                        }, current);
                    break;
                case IDENTIFY_SHIFT_RESTARTPERIOD:
                    encoded = cbor_encode_pair((cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE(CBOR_KEY_RESTARTPERIOD),
                        }, (cbor_value_t){
                            .type = CBOR_TYPE_INTEGER,
                            .value.integer = 8,
                        }, current);
                    break;
                case IDENTIFY_SHIFT_READDATALIFESPAN:
                    encoded = cbor_encode_pair((cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE(CBOR_KEY_READDATALIFESPAN),
                        }, (cbor_value_t){
                            .type = CBOR_TYPE_INTEGER,
                            .value.integer = 24,
                        }, current);
                    break;
                case IDENTIFY_SHIFT_RETRYINTERVAL:
                    encoded = cbor_encode_pair((cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE(CBOR_KEY_RETRYINTERVAL),
                        }, (cbor_value_t){
                            .type = CBOR_TYPE_INTEGER,
                            .value.integer = 10,
                        }, current);
                    break;
                case IDENTIFY_SHIFT_RETRYCOUNT:
                    encoded = cbor_encode_pair((cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE(CBOR_KEY_RETRYCOUNT),
                        }, (cbor_value_t){
                            .type = CBOR_TYPE_INTEGER,
                            .value.integer = 3,
                        }, current);
                    break;
                case IDENTIFY_SHIFT_MAXPACKAGESIZE:
                    encoded = cbor_encode_pair((cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE(CBOR_KEY_MAXPACKAGESIZE),
                        }, (cbor_value_t){
                            .type = CBOR_TYPE_INTEGER,
                            .value.integer = 65536,
                        }, current);
                    break;
                case IDENTIFY_SHIFT_COMMUNICATIONINTERFACES:
                    encoded = cbor_encode_pair((cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE(CBOR_KEY_COMMUNICATIONINTERFACES),
                        }, (cbor_value_t){
                            .type = CBOR_ENCODE_TYPE_CUSTOM_ENCODER,
                            .value.custom_encoder = {
                                .encoder = encode_communication_interfaces,
                                .argument = NULL
                            }
                        }, current);
                    break;
                case IDENTIFY_SHIFT_SERIALPORTS:
                    encoded = cbor_encode_pair((cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE(CBOR_KEY_SERIALPORTS),
                        }, (cbor_value_t){
                            .type = CBOR_ENCODE_TYPE_CUSTOM_ENCODER,
                            .value.custom_encoder = {
                                .encoder = encode_serial_ports,
                                .argument = NULL
                            }
                        }, current);
                    break;
                case IDENTIFY_SHIFT_IOINTERFACES:
                    encoded = cbor_encode_pair((cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE(CBOR_KEY_IOINTERFACES),
                        }, (cbor_value_t){
                            .type = CBOR_ENCODE_TYPE_CUSTOM_ENCODER,
                            .value.custom_encoder = {
                                .encoder = encode_io_interfaces,
                                .argument = NULL
                            }
                        }, current);
                    break;
                case IDENTIFY_SHIFT_METERS:
                    encoded = cbor_encode_pair((cbor_value_t){
                            .type = CBOR_TYPE_TEXT_STRING,
                            .value.bytes = STR2SLICE(CBOR_KEY_METERS),
                        }, (cbor_value_t){
                            .type = CBOR_ENCODE_TYPE_CUSTOM_ENCODER,
                            .value.custom_encoder = {
                                .encoder = encode_meters,
                                .argument = NULL
                            }
                        }, current);
                    break;
                default:
                    return ERR(custom_encoder_result_t, CBOR_ENCODER_TODO);
            }
            if (encoded.is_error) {
                return ERR(custom_encoder_result_t, encoded.err);
            }
            current.ptr += encoded.ok.len;
            current.len -= encoded.ok.len;
            full_length += encoded.ok.len;
            encoded_count++;
            
            // Early exit optimization: if we've encoded all expected pairs, we're done
            if (encoded_count == count) {
                break;
            }
        }
        *bitmap >>= 1;
    }

    return OK(custom_encoder_result_t, ((slice_t){ .len = full_length, .ptr = target.ptr }));
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
                {
                    {
                        .type = CBOR_TYPE_TEXT_STRING,
                        .value.bytes = STR2SLICE("r"),
                    },
                    {
                        .type = CBOR_ENCODE_TYPE_CUSTOM_ENCODER,
                        .value.custom_encoder = {
                            .encoder = encode_identify_bitmap,
                            .argument = &req->request_bitmap
                        }
                    }
                }
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

    identification_request_t request = {
        .d = {
            .f = BUF2SLICE("XYZ"),
            .sn = BUF2SLICE("123456789"),
        },
        .fn = 42,
        .rid = 1756887865,
        .request_bitmap = 0 
            | IDENTIFY_MASK_BRAND
            | IDENTIFY_MASK_MODEL
    };


    cbor_encode_result_t res = cbor_encode((cbor_value_t) {
        .type = CBOR_ENCODE_TYPE_VALUES,
        .value.values = VALUES((
            (cbor_value_t[]){
                {
                    .type = CBOR_ENCODE_TYPE_CUSTOM_ENCODER,
                    .value.custom_encoder = {
                        .encoder = encode_identification_request,
                        .argument = &request
                    }
                },
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
