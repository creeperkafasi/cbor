#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cbor.h"
#include "debug.h"
#include "memory_profiler.h"

#include "identify.h"

uint8_t buf[] = {
0xA4, 0x61, 0x64, 0xA2, 0x61, 0x66, 0x63, 0x58,
0x59, 0x5A, 0x62, 0x73, 0x6E, 0x6F, 0x30, 0x31,
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x41, 0x42, 0x43, 0x44, 0x45, 0x62, 0x66, 0x6E,
0x02, 0x63, 0x72, 0x69, 0x64, 0x1A, 0x68, 0xB9,
0x5A, 0xA7, 0x61, 0x72, 0xA1, 0x6A, 0x70, 0x61,
0x72, 0x61, 0x6D, 0x65, 0x74, 0x65, 0x72, 0x73,
0x83, 0x63, 0x72, 0x64, 0x73, 0x62, 0x66, 0x77,
0x63, 0x6D, 0x65, 0x73,
};

void process_device_info(const cbor_value_t *key, const cbor_value_t *value, void *arg) {
    device_info_t* device = arg;

    if (key->type != CBOR_TYPE_TEXT_STRING) return;
    char keystr[key->value.bytes.len + 1];
    memcpy(keystr, key->value.bytes.ptr, key->value.bytes.len);
    keystr[key->value.bytes.len] = 0;

    if (!strcmp(keystr, "f")) {
        device->f = value->value.bytes;
    }
    if (!strcmp(keystr, "sn")) {
        device->sn = value->value.bytes;
    }
}


void process_identify_parameters(const cbor_value_t *element, void *process_arg) {
    identify_bitmap_t* bitmap = process_arg;

    if (element->type != CBOR_TYPE_TEXT_STRING) return;
    char keystr[element->value.bytes.len + 1];
    memcpy(keystr, element->value.bytes.ptr, element->value.bytes.len);
    keystr[element->value.bytes.len] = 0;

    #define X(name, key) if (!strcmp(keystr, CBOR_KEY_ ## key)) { \
    printf("%s\n", keystr); *bitmap |= IDENTIFY_MASK_ ## key; }
    IDENTIFY_PARAMETERS
    #undef X
}

void process_identify_parameters_container(const cbor_value_t *key, const cbor_value_t *value, void *arg) {
    identify_bitmap_t* bitmap = arg;

    if (key->type != CBOR_TYPE_TEXT_STRING) return;
    char keystr[key->value.bytes.len + 1];
    memcpy(keystr, key->value.bytes.ptr, key->value.bytes.len);
    keystr[key->value.bytes.len] = 0;

    if (!strcmp(keystr, "parameters")) {
        memset(bitmap, 0, sizeof(identify_bitmap_t));
        cbor_process_array(value->value.array, process_identify_parameters, bitmap);
    }
}

void process_identification_request(const cbor_value_t *key, const cbor_value_t *value, void *arg) {
    identification_request_t* request = arg;

    if (key->type != CBOR_TYPE_TEXT_STRING) return;
    char keystr[key->value.bytes.len + 1];
    memcpy(keystr, key->value.bytes.ptr, key->value.bytes.len);
    keystr[key->value.bytes.len] = 0;

    if (!strcmp(keystr, "d")) {
        cbor_process_map(value->value.map, process_device_info, &request->d);
    }
    if (!strcmp(keystr, "fn")) {
        request->fn = value->value.integer;
    }
    if (!strcmp(keystr, "rid")) {
        request->rid = value->value.integer;
    }
    if (!strcmp(keystr, "r")) {
        cbor_process_map(value->value.map, process_identify_parameters_container, &request->request_bitmap);
    }
}

int main() {
    MEMORY_PROFILE_FUNCTION_ENTER("main");
    
    // Initialize memory profiling
    memory_profile_init();
    
    slice_t cbor = {
        .len = sizeof(buf),
        .ptr = &buf[0]
    };
    MEMORY_PROFILE_BUFFER("identify_cbor_data", sizeof(buf));
    MEMORY_PROFILE_CBOR_STRUCTURE("slice_t", &cbor);

    cbor_parse_result_t res = cbor_parse(cbor);
    MEMORY_PROFILE_CBOR_STRUCTURE("cbor_value_t", &res.ok);
    printf("Is Error  : %s\n", res.is_error ? "true" : "false");
    if (res.is_error) {
        printf("Error: %d\n", res.err);
        MEMORY_PROFILE_FUNCTION_EXIT();
        return 1;
    }

    identification_request_t request;
    MEMORY_PROFILE_CBOR_STRUCTURE("identification_request_t", &request);

    cbor_process_map(res.ok.value.map, process_identification_request, &request);

    printf("\nIdentification Request:\n");

    /* device */
    printf("  device:\n");

    /* f */
    printf("    f (%zu bytes): ", request.d.f.len);
    if (request.d.f.len) {
        char tmp[request.d.f.len + 1];
        memcpy(tmp, request.d.f.ptr, request.d.f.len);
        tmp[request.d.f.len] = '\0';
        printf("%s", tmp);
    } else {
        printf("<empty>");
    }
    printf("  (hex: ");
    for (size_t i = 0; i < request.d.f.len; ++i) {
        printf("%02x", request.d.f.ptr[i]);
    }
    printf(")\n");

    /* sn */
    printf("    sn (%zu bytes): ", request.d.sn.len);
    if (request.d.sn.len) {
        char tmp2[request.d.sn.len + 1];
        memcpy(tmp2, request.d.sn.ptr, request.d.sn.len);
        tmp2[request.d.sn.len] = '\0';
        printf("%s", tmp2);
    } else {
        printf("<empty>");
    }
    printf("  (hex: ");
    for (size_t i = 0; i < request.d.sn.len; ++i) {
        printf("%02x", request.d.sn.ptr[i]);
    }
    printf(")\n");

    /* fn */
    printf("  fn: %d\n", request.fn);

    /* rid */
    printf("  rid: %lld\n", (long long)request.rid);

    /* r */
    printf("  r (bitmap 0x%08lX):\n", (unsigned long)request.request_bitmap);
    #define X(name, key) if (request.request_bitmap & IDENTIFY_MASK_ ## key) { printf("    - %s\n", CBOR_KEY_ ## key); }
    IDENTIFY_PARAMETERS
    #undef X

    // Generate memory report
    MEMORY_PROFILE_REPORT();
    
    MEMORY_PROFILE_FUNCTION_EXIT();
}