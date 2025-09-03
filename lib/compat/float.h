#ifndef CBOR_COMPAT_FLOAT_H
#define CBOR_COMPAT_FLOAT_H

#include <stdint.h>

// Compiler intrinsics

// f16 bytes to f32
float __extendhfsf2(uint16_t h);

// f32 to f16
float __truncsfhf2(uint32_t h);

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    // C23 or later - use standard types
    typedef _Float32 Float32;
    typedef _Float64 Float64;
#else
    // Fallback to standard C types
    typedef float Float32;
    typedef double Float64;
#endif

#endif /* CBOR_COMPAT_FLOAT_H */