#ifndef CBOR_COMPAT_FLOAT_H
#define CBOR_COMPAT_FLOAT_H

#include <stdint.h>

// Compiler intrinsics

// f16 bytes to f32
float __extendhfsf2(uint16_t h);

// f32 to f16
float __truncsfhf2(uint32_t h);

#endif /* CBOR_COMPAT_FLOAT_H */