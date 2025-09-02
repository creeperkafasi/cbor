#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cbor.h"
#include "debug.h"

uint8_t buf[] = {
0xA3, 0x61, 0x64, 0xA2, 0x61, 0x66, 0x63, 0x58,
0x59, 0x5A, 0x62, 0x73, 0x6E, 0x6F, 0x30, 0x31,
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x41, 0x42, 0x43, 0x44, 0x45, 0x62, 0x66, 0x6E,
0x02, 0x63, 0x72, 0x69, 0x64, 0x78, 0x24, 0x33,
0x64, 0x30, 0x62, 0x32, 0x34, 0x32, 0x65, 0x2D,
0x31, 0x38, 0x36, 0x36, 0x2D, 0x34, 0x61, 0x34,
0x31, 0x2D, 0x61, 0x38, 0x63, 0x61, 0x2D, 0x31,
0x33, 0x37, 0x32, 0x66, 0x31, 0x62, 0x33, 0x34,
0x61, 0x62, 0x37,
};

typedef struct {
    slice_t f;
    slice_t sn; 
} device_info_t;

typedef struct {
    device_info_t d;
    int fn;
    slice_t rid;
} identification_request_t;

void process_device_info(const cbor_value_t *key, const cbor_value_t *value, void *arg) {
    device_info_t* device = arg;

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

void process_identification_request(const cbor_value_t *key, const cbor_value_t *value, void *arg) {
    identification_request_t* request = arg;

    char keystr[key->value.bytes.len + 1];
    memcpy(keystr, key->value.bytes.ptr, key->value.bytes.len);
    keystr[key->value.bytes.len] = 0;

    if (!strcmp(keystr, "d")) {
        process_map(value->value.map, process_device_info, &request->d);
    }
    if (!strcmp(keystr, "fn")) {
        request->fn = value->value.integer;
    }
    if (!strcmp(keystr, "rid")) {
        request->rid = value->value.bytes;
    }
}

int main() {
    slice_t cbor = {
        .len = sizeof(buf),
        .ptr = &buf[0]
    };

    parse_result_t res = parse(cbor);
    printf("Is Error  : %s\n", res.is_error ? "true" : "false");
    if (res.is_error) {
        printf("Error: %d\n", res.err);
        return 1;
    }

    identification_request_t request;

    process_map(res.ok.value.map, process_identification_request, &request);

    printf("\nIdentification Request:\n");
    printf("  fn: %d\n", request.fn);

    /* rid */
    printf("  rid (%zu bytes): ", request.rid.len);
    for (size_t i = 0; i < request.rid.len; ++i) {
        printf("%02x", request.rid.ptr[i]);
    }
    printf("\n");

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
}