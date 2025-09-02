/**
 * result.h - Rust-like result type macro
 * 
 * Author:
 * Deniz Tunç
 * @creeperkafasi
 */

#ifndef RESULT_H
#define RESULT_H

/*--------------------------------------------------------------------------*/
#define RESULT_TYPE_NAME(oktype, errtype) result_ ## oktype ## _ ## errtype ## _t

#define RESULT_STRUCT(type, errortype)            \
struct {                                          \
    uint8_t is_error;                             \
    union {                                       \
        type ok;                                  \
        errortype err;                            \
    };                                            \
}

#define DEFINE_RESULT_TYPE(type, errortype) \
typedef RESULT_STRUCT(type, errortype) RESULT_TYPE_NAME(type, errortype)

#define OK(result_type, value) \
(result_type) {.is_error=0, .ok = value}

#define ERR(result_type, value) \
(result_type) {.is_error=1, .err = value}

#define FN_RESULT(type, errortype, name, ...)               \
typedef RESULT_TYPE_NAME(type, errortype) name ## _result_t; \
name ## _result_t name (__VA_ARGS__)
/*--------------------------------------------------------------------------*/

#endif /*RESULT_H*/