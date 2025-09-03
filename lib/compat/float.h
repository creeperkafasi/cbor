#ifndef CBOR_COMPAT_FLOAT_H
#define CBOR_COMPAT_FLOAT_H

#include <stdint.h>

// Compiler intrinsics

#include <stdint.h>
#include <math.h>

// f16 to f32 conversion
static inline float __extendhfsf2(uint16_t h) {
    // Extract sign, exponent, and mantissa from half-precision
    uint32_t sign = ((uint32_t)h & 0x8000) << 16;
    int32_t exp = (h & 0x7C00) >> 10;
    uint32_t mant = h & 0x03FF;
    
    if (exp == 0) {
        // Zero or subnormal
        if (mant == 0) {
            return *(float*)&sign; // ±0.0
        }
        // Subnormal - normalize
        while (!(mant & 0x0400)) {
            mant <<= 1;
            exp--;
        }
        exp++;
        mant &= 0x03FF;
    } else if (exp == 0x1F) {
        // NaN or Infinity
        if (mant == 0) {
            // Infinity
            uint32_t inf = sign | 0x7F800000;
            return *(float*)&inf;
        } else {
            // NaN - preserve payload in quiet NaN
            uint32_t nan = sign | 0x7FC00000 | (mant << 13);
            return *(float*)&nan;
        }
    }
    
    // Convert to single precision: bias 127 vs 15
    uint32_t result = sign | ((exp + (127 - 15)) << 23) | (mant << 13);
    return *(float*)&result;
}

// f32 to f16 conversion
static inline uint16_t __truncsfhf2(float f) {
    uint32_t x = *(uint32_t*)&f;
    
    // Extract components from single-precision
    uint32_t sign = (x & 0x80000000) >> 16;
    int32_t exp = (x & 0x7F800000) >> 23;
    uint32_t mant = x & 0x007FFFFF;
    
    // Handle special cases
    if (exp == 0xFF) {
        // NaN or Infinity
        if (mant == 0) {
            return sign | 0x7C00; // Infinity
        } else {
            // NaN - convert to half-precision NaN (preserve some payload)
            return sign | 0x7E00 | (mant >> 13);
        }
    }
    
    // Adjust exponent from single to half precision (bias 127 vs 15)
    exp = exp - 127 + 15;
    
    if (exp <= 0) {
        // Underflow - becomes zero or subnormal
        if (exp < -10) {
            return sign; // Underflow to zero
        }
        // Subnormal
        mant = (mant | 0x00800000) >> (1 - exp);
        if (mant & 0x00001000) {
            mant += 0x00002000; // Round
        }
        return sign | (mant >> 13);
    }
    
    if (exp >= 0x1F) {
        // Overflow - becomes infinity
        return sign | 0x7C00;
    }
    
    // Normal number
    if (mant & 0x00001000) {
        mant += 0x00002000; // Round
        if (mant & 0x00800000) {
            mant = 0;        // Overflow in rounding
            exp++;
            if (exp >= 0x1F) {
                return sign | 0x7C00; // Overflow to infinity
            }
        }
    }
    
    return sign | (exp << 10) | (mant >> 13);
}

// Helper functions for better usability
static inline float half_to_float(uint16_t half_val) {
#ifndef __HAVE_FLOAT16
    return __extendhfsf2(half_val);
#else
    return (float)*(_Float16*)&half_val;
#endif
}

static inline uint16_t float_to_half(float float_val) {
#ifndef __HAVE_FLOAT16
    return __truncsfhf2(float_val);
#else
    _Float16 hf = (_Float16)float_val;
    return *(uint16_t*)&hf;
#endif
}

#include <stdint.h>
#include <math.h>

// f32 to f64 conversion
static inline double __extendsfdf2(float f) {
    uint32_t x = *(uint32_t*)&f;
    
    // Extract components from single-precision
    uint64_t sign = ((uint64_t)x & 0x80000000) << 32;
    int32_t exp = (x & 0x7F800000) >> 23;
    uint64_t mant = x & 0x007FFFFF;
    
    if (exp == 0xFF) {
        // NaN or Infinity
        if (mant == 0) {
            // Infinity
            uint64_t inf = sign | 0x7FF0000000000000ULL;
            return *(double*)&inf;
        } else {
            // NaN - preserve payload and set quiet bit
            uint64_t nan = sign | 0x7FF8000000000000ULL | ((uint64_t)mant << 29);
            return *(double*)&nan;
        }
    }
    
    if (exp == 0) {
        // Zero or subnormal
        if (mant == 0) {
            return *(double*)&sign; // ±0.0
        }
        // Subnormal - normalize
        while (!(mant & 0x00800000)) {
            mant <<= 1;
            exp--;
        }
        exp++;
        mant &= 0x007FFFFF;
    }
    
    // Convert to double precision: bias 1023 vs 127
    // Double precision has 52-bit mantissa vs 23-bit in single
    uint64_t result = sign | ((uint64_t)(exp + (1023 - 127)) << 52) | (mant << 29);
    return *(double*)&result;
}

// f64 to f32 conversion
static inline float __truncdfsf2(double d) {
    uint64_t x = *(uint64_t*)&d;
    
    // Extract components from double-precision
    uint32_t sign = (x & 0x8000000000000000ULL) >> 32;
    int64_t exp = (x & 0x7FF0000000000000ULL) >> 52;
    uint64_t mant = x & 0x000FFFFFFFFFFFFFULL;
    
    // Handle special cases
    if (exp == 0x7FF) {
        // NaN or Infinity
        if (mant == 0) {
            uint32_t temp = sign | 0x7F800000;
            return *(float*)&temp;
        } else {
            // NaN - convert to single-precision NaN (preserve some payload)
            uint32_t temp = sign | 0x7FC00000 | (mant >> 29);
            return *(float*)&temp;
        }
    }
    
    // Adjust exponent from double to single precision (bias 1023 vs 127)
    exp = exp - 1023 + 127;
    
    if (exp <= 0) {
        // Underflow - becomes zero or subnormal
        if (exp < -24) {
            return *(float*)&sign; // Underflow to zero
        }
        // Subnormal
        mant = (mant | 0x0010000000000000ULL) >> (1 - exp);
        if (mant & 0x0000000000800000ULL) {
            mant += 0x0000000001000000ULL; // Round
        }
        return sign | (mant >> 29);
    }
    
    if (exp >= 0xFF) {
        // Overflow - becomes infinity
        return sign | 0x7F800000;
    }
    
    // Normal number - add implicit leading 1 and align
    mant |= 0x0010000000000000ULL; // Add implicit leading 1
    
    // Round to 24 bits (23 mantissa + 1 implicit)
    uint64_t round_bit = 0x0000000000800000ULL;
    uint64_t sticky_mask = 0x00000000007FFFFFULL;
    
    if (mant & round_bit) {
        if ((mant & sticky_mask) || (mant & (round_bit << 1))) {
            mant += round_bit; // Round up
            if (mant & 0x0020000000000000ULL) {
                mant >>= 1;
                exp++;
                if (exp >= 0xFF) {
                    return sign | 0x7F800000; // Overflow to infinity
                }
            }
        }
    }
    
    // Remove implicit leading 1 and return
    return sign | (exp << 23) | ((mant & 0x001FFFFFC0000000ULL) >> 29);
}

// Helper functions for better usability
static inline double float_to_double(float float_val) {
#ifndef __HAVE_FLOAT64
    return __extendsfdf2(float_val);
#else
    return (double)float_val;
#endif
}

static inline float double_to_float(double double_val) {
#ifndef __HAVE_FLOAT64
    return __truncdfsf2(double_val);
#else
    return (float)double_val;
#endif
}

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