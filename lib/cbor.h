#ifndef CBOR_H
#define CBOR_H

#include <stddef.h>
#include <stdint.h>

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__) || defined(__FreeBSD__)
#   include <endian.h>
#else
#   include "compat/endian.h"
#endif

#include "result.h"
#include "config.h"

/*--------------------------------------------------------------------------*/
/* Basic Types and Constants */
/*--------------------------------------------------------------------------*/

typedef enum {
    CBOR_MAJOR_TYPE_UNSIGNED_INTEGER = 0,
    CBOR_MAJOR_TYPE_NEGATIVE_INTEGER = 1,
    CBOR_MAJOR_TYPE_BYTE_STRING      = 2,
    CBOR_MAJOR_TYPE_TEXT_STRING      = 3,
    CBOR_MAJOR_TYPE_ARRAY            = 4,
    CBOR_MAJOR_TYPE_MAP              = 5,
    CBOR_MAJOR_TYPE_TAG              = 6,
    CBOR_MAJOR_TYPE_SIMPLE           = 7,
} cbor_major_type_t;

typedef enum {
    CBOR_ERROR = -1,
    CBOR_TYPE_INTEGER          = 0,
    CBOR_TYPE_BYTE_STRING      = 2,
    CBOR_TYPE_TEXT_STRING      = 3,
    CBOR_TYPE_ARRAY            = 4,
    CBOR_TYPE_MAP              = 5,
    CBOR_TYPE_TAG              = 6,
    CBOR_TYPE_SIMPLE           = 7,
    CBOR_TYPE_FLOAT            = 8,

    CBOR_ENCODE_TYPE_VALUES,
    CBOR_ENCODE_TYPE_PAIRS,
    CBOR_ENCODE_TYPE_VALUES_INDEFINITE,
    CBOR_ENCODE_TYPE_PAIRS_INDEFINITE,
    CBOR_ENCODE_TYPE_BYTE_STRING_INDEFINITE,
    CBOR_ENCODE_TYPE_TEXT_STRING_INDEFINITE,
    CBOR_ENCODE_TYPE_CUSTOM_ENCODER
} cbor_type_t;

typedef enum {
    CBOR_SIMPLE_FALSE,
    CBOR_SIMPLE_TRUE,
    CBOR_SIMPLE_NULL,
    CBOR_SIMPLE_UNDEFINED,
    CBOR_SIMPLE_ERROR_RESERVED,
    CBOR_SIMPLE_ERROR_UNASSIGNED
} cbor_simple_t;

typedef enum {
    CBOR_ENCODER_NULL_PTR_ERROR,
    CBOR_ENCODER_ERROR_BUFFER_OVERFLOW,
    CBOR_ENCODER_TODO,
    CBOR_ENCODER_UNKNOWN_SIZE
} cbor_encode_error_t;

typedef enum {
    NULL_PTR_ERROR,
    EMPTY_BUFFER_ERROR,
    MALFORMED_INPUT_ERROR,
    PARSER_TODO
} cbor_parser_error_t;

/*--------------------------------------------------------------------------*/
/* Core Data Structures */
/*--------------------------------------------------------------------------*/

typedef struct {
    size_t len;
    uint8_t* ptr;
} slice_t;

typedef struct argument_t {
    enum argument_t_tag {
        ARGUMENT_NONE,  // Indefinite
        ARGUMENT_1BYTE,
        ARGUMENT_2BYTE,
        ARGUMENT_4BYTE,
        ARGUMENT_8BYTE,
        ARGUMENT_MALFORMED,
    } tag;
    uint8_t size;
    union {
        uint8_t _1byte;
        uint16_t _2byte;
        uint32_t _4byte;
        uint64_t _8byte;
    };
} argument_t;

typedef struct {
    uint32_t length;
    uint8_t* inside;
    size_t max_size;
} cbor_map_t;

typedef struct {
    uint32_t length;
    uint8_t* inside;
    size_t max_size;
} cbor_array_t;

typedef struct {
    uint8_t* key;
    uint8_t* value;
} cbor_map_entry_t;

/* Forward declarations for recursive types */
typedef struct cbor_value_s cbor_value_t;
typedef struct cbor_pair_s cbor_pair_t;

typedef struct {
    size_t len;
    cbor_value_t *ptr;
} cbor_value_slice_t;

typedef struct {
    size_t len;
    cbor_pair_t* ptr;
} cbor_pair_slice_t;

/*--------------------------------------------------------------------------*/
/* Encoder Types */
/*--------------------------------------------------------------------------*/

DEFINE_RESULT_TYPE(slice_t, cbor_encode_error_t);
typedef RESULT_TYPE_NAME(slice_t, cbor_encode_error_t) custom_encoder_result_t;

typedef custom_encoder_result_t (*custom_encoder_function_t)(slice_t target, void* arg);

typedef struct {
    custom_encoder_function_t encoder;
    void* argument;
} cbor_custom_encoder_t;

/*--------------------------------------------------------------------------*/
/* Main CBOR Value Structure */
/*--------------------------------------------------------------------------*/

/**
 * Size on 64-bit systems: 56 bytes
 * Size on 32-bit systems: 48 bytes
 */
typedef struct cbor_value_s {
    cbor_type_t type;
    argument_t argument;
    union {
        int64_t integer;
        slice_t bytes;
        int64_t length;
        cbor_map_t map;
        cbor_array_t array;
        float floating;
        cbor_simple_t simple;
        cbor_value_slice_t values;
        cbor_pair_slice_t pairs;
        cbor_custom_encoder_t custom_encoder;
    } value;
    uint8_t *next;
} cbor_value_t;

typedef struct cbor_pair_s {
    cbor_value_t first;
    cbor_value_t second;
} cbor_pair_t;

/*--------------------------------------------------------------------------*/
/* Utility Macros */
/*--------------------------------------------------------------------------*/

#define SLICE2BUF(buf, slice) \
    do { \
        assert(sizeof(buf) >= (slice).len + 1); \
        memcpy((buf), (slice).ptr, (slice).len); \
        (buf)[(slice).len] = 0; \
    } while (0)

#define BUF2SLICE(buf) \
    ((slice_t) { \
        .len = strlen((char*)(buf)), \
        .ptr = (uint8_t*)(buf) \
    })

#define STR2SLICE(str) \
    ((slice_t) { \
        .len = sizeof(str) - 1, \
        .ptr = (uint8_t*)(str) \
    })

#define ARRAY_TO_SLICE(type, array) (type) { .len = sizeof(array) / sizeof(array[0]), .ptr = array }

#define VALUES(array) (ARRAY_TO_SLICE(cbor_value_slice_t, array))

#define VALUES_INDEFINITE(array) (ARRAY_TO_SLICE(cbor_value_slice_t, array))

#define PAIR(first, second) (cbor_pair_t) {.first = (cbor_value_t)(first), .second = (cbor_value_t)(second)}
#define PAIRS(array) ARRAY_TO_SLICE(cbor_pair_slice_t, array)

#define PAIRS_INDEFINITE(array) ARRAY_TO_SLICE(cbor_pair_slice_t, array)

/*--------------------------------------------------------------------------*/
/* Indefinite Length Value Constructors */
/*--------------------------------------------------------------------------*/

#define CBOR_INDEFINITE_ARRAY(values_array) \
    ((cbor_value_t) { \
        .type = CBOR_ENCODE_TYPE_VALUES_INDEFINITE, \
        .value.values = VALUES_INDEFINITE(values_array) \
    })

#define CBOR_INDEFINITE_MAP(pairs_array) \
    ((cbor_value_t) { \
        .type = CBOR_ENCODE_TYPE_PAIRS_INDEFINITE, \
        .value.pairs = PAIRS_INDEFINITE(pairs_array) \
    })

#define CBOR_INDEFINITE_BYTE_STRING(chunks_array) \
    ((cbor_value_t) { \
        .type = CBOR_ENCODE_TYPE_BYTE_STRING_INDEFINITE, \
        .value.values = VALUES_INDEFINITE(chunks_array) \
    })

#define CBOR_INDEFINITE_TEXT_STRING(chunks_array) \
    ((cbor_value_t) { \
        .type = CBOR_ENCODE_TYPE_TEXT_STRING_INDEFINITE, \
        .value.values = VALUES_INDEFINITE(chunks_array) \
    })

/*--------------------------------------------------------------------------*/
/* Function Declarations */
/*--------------------------------------------------------------------------*/

/* Parser Functions */
static inline cbor_major_type_t cbor_get_major_type(const uint8_t* data) {
    return (*data) >> 5;
}

static inline argument_t cbor_get_argument(const uint8_t* data) {
    uint8_t argument = (*data) & 0x1F;

    if (argument < 24) {
        return (argument_t){
            .tag = ARGUMENT_1BYTE,
            ._1byte = argument,
            .size = 0
        };
    }
    
    if (argument == 24) {
        return (argument_t){
            .tag = ARGUMENT_1BYTE,
            ._1byte = *(data + 1),
            .size = 1
        };
    }
    
    if (argument == 25) {
        uint16_t temp = *(uint16_t*)(data + 1);
        return (argument_t){
            .tag = ARGUMENT_2BYTE,
            ._2byte = be16toh(temp),
            .size = 2
        };
    }
    
    if (argument == 26) {
        uint32_t temp = *(uint32_t*)(data + 1);
        return (argument_t){
            .tag = ARGUMENT_4BYTE,
            ._4byte = be32toh(temp),
            .size = 4
        };
    }
    
    if (argument == 27) {
        uint64_t temp = *(uint64_t*)(data + 1);
        return (argument_t){
            .tag = ARGUMENT_8BYTE,
            ._8byte = be64toh(temp),
            .size = 8
        };
    }
    
    if (argument == 31) {
        return (argument_t){
            .tag = ARGUMENT_NONE,
            .size = 0
        };
    }
    
    return (argument_t){
        .tag = ARGUMENT_MALFORMED
    };
}

static inline uint64_t cbor_argument_to_fixed(argument_t arg) {
    switch (arg.tag) {
        case ARGUMENT_1BYTE: return arg._1byte;
        case ARGUMENT_2BYTE: return arg._2byte;
        case ARGUMENT_4BYTE: return arg._4byte;
        case ARGUMENT_8BYTE: return arg._8byte;
        default: return 0;
    }
}

DEFINE_RESULT_TYPE(cbor_value_t, cbor_parser_error_t);
FN_RESULT (
    cbor_value_t, cbor_parser_error_t,
    cbor_parse, slice_t buf
);

/* Processing Functions */
typedef void (*pair_processor_function)(const cbor_value_t* key, const cbor_value_t* value, void* process_arg);
typedef void (*single_processor_function)(const cbor_value_t* value, void* process_arg);

uint8_t* cbor_process_array(cbor_array_t array, single_processor_function process_single, void* process_arg);
uint8_t* cbor_process_map(cbor_map_t map, pair_processor_function process_pair, void* process_arg);
uint8_t* cbor_process_indefinite_string(cbor_array_t string_chunks, cbor_type_t expected_type, single_processor_function process_single, void* process_arg);

/* Encoding Functions */
FN_RESULT(slice_t, cbor_encode_error_t,
cbor_encode, cbor_value_t value, slice_t target);

/* Indefinite Length Encoding Functions */
cbor_encode_result_t cbor_encode_value_array_indefinite(cbor_value_slice_t values, slice_t target);
cbor_encode_result_t cbor_encode_value_map_indefinite(cbor_pair_slice_t pairs, slice_t target);

cbor_encode_result_t cbor_encode_pair (cbor_value_t first, cbor_value_t second, slice_t target);

// raw function that writes only the major type and the argument!!!
uint8_t cbor_write_len_header(size_t len, cbor_major_type_t major_type, slice_t target);

#endif /*CBOR_H*/