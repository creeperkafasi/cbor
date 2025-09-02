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
typedef struct {
    size_t len;
    uint8_t* ptr;
} slice_t;

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
} cbor_type_t;


/*--------------------------------------------------------------------------*/
/**
 * Get the Major Type (first 3 bits)
 */
cbor_major_type_t get_major_type(const uint8_t* data); 
/*--------------------------------------------------------------------------*/
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
/**
 * Get the Argument (last 5 bits)
 * The value of the argument is stored in the fields _1byte, _2byte, _4byte or _8byte depending on the size of the argument.
 * The argument byte size is stored in the .size field
 */
argument_t get_argument(const uint8_t* data);
// Check for malformed input before calling this
uint64_t argument_to_fixed(argument_t arg);
/*--------------------------------------------------------------------------*/

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

typedef enum {
    CBOR_SIMPLE_FALSE,
    CBOR_SIMPLE_TRUE,
    CBOR_SIMPLE_NULL,
    CBOR_SIMPLE_UNDEFINED,
    CBOR_SIMPLE_ERROR_RESERVED,
    CBOR_SIMPLE_ERROR_UNASSIGNED
} cbor_simple_t;

typedef struct cbor_value_t {
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
    } value;
    uint8_t* next;
} cbor_value_t;

typedef enum {
    NULL_PTR_ERROR,
    EMPTY_BUFFER_ERROR,
    MALFORMED_INPUT_ERROR,
    PARSER_TODO
} cbor_parser_error_t;

DEFINE_RESULT_TYPE(cbor_value_t, cbor_parser_error_t);
FN_RESULT (
    cbor_value_t, cbor_parser_error_t,
    parse, slice_t buf
);
/*--------------------------------------------------------------------------*/
typedef void (*pair_processor_function)(const cbor_value_t* key, const cbor_value_t* value, void* process_arg);
typedef void (*single_processor_function)(const cbor_value_t* value, void* process_arg);

uint8_t* process_array(cbor_array_t array, single_processor_function process_single, void* process_arg);
/**
 * Iterates over a map using the process_pair function
 */
uint8_t* process_map(cbor_map_t map, pair_processor_function process_pair, void* process_arg);
/*--------------------------------------------------------------------------*/


/********************************
 * 
 * ENCODING
 * 
 ********************************/

typedef enum {
    CBOR_ENCODER_NULL_PTR_ERROR,
    CBOR_ENCODER_ERROR_BUFFER_OVERFLOW,
    CBOR_ENCODER_TODO,
    CBOR_ENCODER_UNKNOWN_SIZE
} cbor_encode_error_t;

DEFINE_RESULT_TYPE(slice_t, cbor_encode_error_t);
FN_RESULT(slice_t, cbor_encode_error_t,
encode, cbor_value_t value, slice_t target);

encode_result_t encode_array(cbor_value_t *array, size_t array_len, slice_t target);

#endif /*CBOR_H*/