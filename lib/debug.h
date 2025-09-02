#ifndef CBOR_DEBUG_H
#define CBOR_DEBUG_H

#include "cbor.h"

void print_cbor_value(cbor_value_t value, int indent);
void print_cbor_type(cbor_major_type_t type);

void print_single(const cbor_value_t* value, void* arg);
void print_pair(const cbor_value_t* key, const cbor_value_t* value, void* arg);

static inline void print_slice_hex(slice_t slice) {
    for (size_t i = 0; i < (slice.len / 8 + 1); i++) {
        for (size_t j = 0; j < 8; j++) {
            if (i * 8 + j >= slice.len) break;
            printf("%02X ", slice.ptr[i * 8 + j]);
        }
        printf("\n");
    }
}

// Segfaults by writing to NULL
#define KILL_YOURSELF()         \
do {                            \
    perror("Segfaulting 🥰\n"); \
    *(uint8_t*)0 = 0;           \
} while(0)

#endif /* CBOR_DEBUG_H */