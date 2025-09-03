#include "cbor.h"
#include <stdio.h>
#include <stdint.h>
#if defined(__linux__) || defined(__unix__) || defined(__APPLE__) || defined(__FreeBSD__)
#include <endian.h>
#else 
#include "compat/endian.h"
#endif
#include <string.h>

#include "compat/float.h"

#define CBOR_FLOAT_PRECISION_DEFAULT CBOR_FLOAT_PRECISION_SINGLE

/********************************
 * 
 * PARSING
 * 
 ********************************/

/*--------------------------------------------------------------------------*/
cbor_major_type_t get_major_type(const uint8_t *data)
{
    return (*data) >> 5;
}
/*--------------------------------------------------------------------------*/
argument_t get_argument(const uint8_t* data) {
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
/*--------------------------------------------------------------------------*/
uint64_t argument_to_fixed(argument_t arg) {
    switch (arg.tag) {
        case ARGUMENT_1BYTE: return arg._1byte;
        case ARGUMENT_2BYTE: return arg._2byte;
        case ARGUMENT_4BYTE: return arg._4byte;
        case ARGUMENT_8BYTE: return arg._8byte;
        default: return 0;
    }
}
/*--------------------------------------------------------------------------*/
FN_RESULT (
    cbor_value_t, cbor_parser_error_t,
    parse, slice_t buf
) {
    if (buf.ptr == NULL) {
        return ERR(parse_result_t, NULL_PTR_ERROR);
    }
    cbor_major_type_t major_type = get_major_type(buf.ptr);
    cbor_value_t value = {0};  // Initialize entire struct to zero
    value.argument = get_argument(buf.ptr);
    if (value.argument.tag == ARGUMENT_MALFORMED) {
        return ERR(parse_result_t, MALFORMED_INPUT_ERROR);
    }

    switch(major_type) {
    case CBOR_MAJOR_TYPE_UNSIGNED_INTEGER:
        value.type = CBOR_TYPE_INTEGER;
        value.value.integer = (int64_t)argument_to_fixed(value.argument);
        value.next = buf.ptr + value.argument.size + 1;
        break;
    case CBOR_MAJOR_TYPE_NEGATIVE_INTEGER:
        value.type = CBOR_TYPE_INTEGER;
        value.value.integer = - 1 - (int64_t)argument_to_fixed(value.argument);
        value.next = buf.ptr + value.argument.size + 1;
        break;
    case CBOR_MAJOR_TYPE_BYTE_STRING:
        value.type = CBOR_TYPE_BYTE_STRING;
        __attribute__((fallthrough));
    case CBOR_MAJOR_TYPE_TEXT_STRING:
        value.type = CBOR_TYPE_TEXT_STRING;
        value.next = buf.ptr + value.argument.size + argument_to_fixed(value.argument) + 1;
        if (value.argument.tag == ARGUMENT_NONE) {
            return ERR(parse_result_t, PARSER_TODO); // TODO indefinite length
        }
        else {
            value.value.bytes.len = argument_to_fixed(value.argument);
            value.value.bytes.ptr = buf.ptr + value.argument.size + 1;
        }
        break;
    case CBOR_MAJOR_TYPE_ARRAY:
        value.type = CBOR_TYPE_ARRAY;
        if (value.argument.tag == ARGUMENT_NONE) {
            return ERR(parse_result_t, PARSER_TODO); // TODO indefinite length
        }
        else {
            value.value.array = (cbor_array_t) {
                .length = argument_to_fixed(value.argument),
                .inside = buf.ptr + value.argument.size + 1,
                .max_size = buf.ptr + buf.len - value.next,
            };
            value.next = NULL;
        }
        break;
    case CBOR_MAJOR_TYPE_MAP:
        value.type = CBOR_TYPE_MAP;
        if (value.argument.tag == ARGUMENT_NONE) {
            return ERR(parse_result_t, PARSER_TODO); // TODO indefinite length
        }
        else {
            value.value.map = (cbor_map_t){
                .length = argument_to_fixed(value.argument),
                .inside = buf.ptr + value.argument.size + 1,
                .max_size = buf.ptr + buf.len - value.next
            };
        }
        value.next = NULL;
        break;
    case CBOR_MAJOR_TYPE_SIMPLE:
        // Simples and floats
        switch (value.argument.tag) {
        case ARGUMENT_1BYTE:
            // Simple value (true/false/null/simple) - TODO
            switch (value.argument._1byte) {
                case 20:
                    value.value.simple = CBOR_SIMPLE_FALSE;
                    break;
                case 21:
                    value.value.simple = CBOR_SIMPLE_TRUE;
                    break;
                case 22:
                    value.value.simple = CBOR_SIMPLE_NULL;
                    break;
                case 23:
                    value.value.simple = CBOR_SIMPLE_UNDEFINED;
                    break;
                default:
                    if ((value.argument._1byte >= 24) || (value.argument._1byte <= 31)) {
                        value.value.simple = CBOR_SIMPLE_ERROR_RESERVED;
                    }
                    else {
                        value.value.simple = CBOR_SIMPLE_ERROR_UNASSIGNED;
                    }
            }
            value.type = CBOR_TYPE_SIMPLE;
            break;
        case ARGUMENT_2BYTE:
            // f16
            // TODO: not every device has _Float16
#if __HAVE_FLOAT16
            value.value.floating = (float)*(_Float16*)&value.argument._2byte;
#else
            // TODO
#endif
            value.type = CBOR_TYPE_FLOAT;
            break;
        case ARGUMENT_4BYTE:
            // f32

            value.value.floating = (float)*(Float32*)&value.argument._4byte;
            value.type = CBOR_TYPE_FLOAT;
            break;
        case ARGUMENT_8BYTE:
            // f64
            value.value.floating = (float)*(Float64*)&value.argument._8byte;
            value.type = CBOR_TYPE_FLOAT;
            break;
        default:
            /* malformed or unhandled */
            break;
        }
        value.next = buf.ptr + value.argument.size + 1;
        break;
    default:
        return ERR(parse_result_t, PARSER_TODO); // TODO
    }
    return OK(parse_result_t, value);
}
/*--------------------------------------------------------------------------*/
uint8_t* process_array(cbor_array_t array, single_processor_function process_single, void* process_arg) {
    if (array.inside == NULL) {
        return NULL;
    }
    
    if (array.length == 0) return array.inside;

    uint8_t* current = array.inside;
    for (size_t i = 0; i < array.length; i++) {
        
        slice_t element_slice = (slice_t) {
            .len = array.max_size - (current - array.inside),
            .ptr = current,
        };

        parse_result_t element = parse(element_slice);

        if (element.is_error) {
            printf("ELEMENT RETURNED ERROR: %d\n", element.err);
        }

        if (process_single != NULL) {
            // printf("Current: %3d", current - buf);
            process_single(&element.ok, process_arg);
        }

        if (element.ok.next == NULL) {
            if (element.ok.type == CBOR_TYPE_MAP) {
                element.ok.next = process_map(element.ok.value.map, NULL, process_arg);
            } 
            else if (element.ok.type == CBOR_TYPE_ARRAY) {
                element.ok.next = process_array(element.ok.value.array, NULL, process_arg);
            } 
        }

        current = element.ok.next;

    }
    return current;
}
/*--------------------------------------------------------------------------*/
uint8_t* process_map(cbor_map_t map, pair_processor_function process_pair, void* process_arg) {
    if (map.inside == NULL) {
        return NULL;
    }
    if (map.length == 0) return map.inside;

    uint8_t* current = map.inside;
    for (size_t i = 0; i < map.length; i++) {
        // printf("Current: %3d\n", current - buf);

        slice_t key_slice = {
            .len = map.max_size - (current - map.inside),
            .ptr = current
        };
        parse_result_t key_v = parse(key_slice);

        if (key_v.is_error) {
            printf("KEY RETURNED ERROR: %d\n", key_v.err);
            return NULL;
        }

        slice_t value_slice = {
            .len = map.max_size - (current - map.inside),
            .ptr = key_v.ok.next
        };
        parse_result_t value_v = parse(value_slice);

        if (value_v.is_error) {
            printf("VALUE RETURNED ERROR: %d\n", value_v.err);
            return NULL;
        }

        if (process_pair != NULL) {
            process_pair((const cbor_value_t*)&key_v.ok, (const cbor_value_t*)&value_v.ok, process_arg);
        }

        if (value_v.ok.next == NULL) {
            if (value_v.ok.type == CBOR_TYPE_MAP) {
                value_v.ok.next = process_map(value_v.ok.value.map, NULL, process_arg);
            }
            else if (value_v.ok.type == CBOR_TYPE_ARRAY) {
                value_v.ok.next = process_array(value_v.ok.value.array, NULL, process_arg);
            }
        }
        current = value_v.ok.next;
    }
    return current;
}

/********************************
 * 
 * ENCODING
 * 
 ********************************/


uint8_t write_len_header(size_t len, cbor_major_type_t major_type, slice_t target) {
    uint8_t header_size = 0;
    if (len <= 23) {
        target.ptr[0] = (uint8_t)((major_type << 5) | len);
        header_size = 1;
    }
    else if (len <= UINT8_MAX) {
        target.ptr[0] = (uint8_t)((major_type << 5) | 24);
        target.ptr[1] = (uint8_t)len;
        header_size = 2;
    }
    else if (len <= UINT16_MAX) {
        target.ptr[0] = (uint8_t)((major_type << 5) | 25);
        uint16_t bytes = htobe16((uint16_t)len);
        memcpy(&target.ptr[1], &bytes, sizeof(bytes));
        header_size = 3;
    }
    else if (len <= UINT32_MAX) {
        target.ptr[0] = (uint8_t)((major_type << 5) | 26);
        uint32_t bytes = htobe32((uint32_t)len);
        memcpy(&target.ptr[1], &bytes, sizeof(bytes));
        header_size = 5;
    }
    #if SIZE_MAX > UINT32_MAX
    else if (len <= UINT64_MAX) {
        target.ptr[0] = (uint8_t)((major_type << 5) | 27);
        uint64_t bytes = htobe64((uint64_t)len);
        memcpy(&target.ptr[1], &bytes, sizeof(bytes));
        header_size = 9;
    }
    #endif
    
    return header_size;
}

encode_result_t cbor_encode_integer(int64_t integer, slice_t target) {
    cbor_major_type_t major_type = CBOR_MAJOR_TYPE_UNSIGNED_INTEGER;
    if (integer < 0) {
        integer = -1 - integer;
        major_type = CBOR_MAJOR_TYPE_NEGATIVE_INTEGER;
    }
    uint64_t ui = (uint64_t)integer;

    if (ui <= 23) {
        // 1 byte
        if (target.len < 1) return ERR(encode_result_t, CBOR_ENCODER_ERROR_BUFFER_OVERFLOW);
        target.ptr[0] = (uint8_t)((major_type << 5) | ui);
        target.len = 1;
        return OK(encode_result_t, target);
    }
    else if (ui <= UINT8_MAX) {
        // 2 bytes
        if (target.len < 2) return ERR(encode_result_t, CBOR_ENCODER_ERROR_BUFFER_OVERFLOW);
        target.ptr[0] = (uint8_t)((major_type << 5) | 24);
        target.ptr[1] = (uint8_t)ui;
        target.len = 2;
        return OK(encode_result_t, target);
    }
    else if (ui <= UINT16_MAX) {
        // 3 bytes
        if (target.len < 3) return ERR(encode_result_t, CBOR_ENCODER_ERROR_BUFFER_OVERFLOW);
        target.ptr[0] = (uint8_t)((major_type << 5) | 25);
        uint16_t bytes = htobe16((uint16_t)ui);
        memcpy(&target.ptr[1], &bytes, sizeof(bytes));
        target.len = 3;
        return OK(encode_result_t, target);
    }
    else if (ui <= UINT32_MAX) {
        // 5 bytes
        if (target.len < 5) return ERR(encode_result_t, CBOR_ENCODER_ERROR_BUFFER_OVERFLOW);
        target.ptr[0] = (uint8_t)((major_type << 5) | 26);
        uint32_t bytes = htobe32((uint32_t)ui);
        memcpy(&target.ptr[1], &bytes, sizeof(bytes));
        target.len = 5;
        return OK(encode_result_t, target);
    }
    else {
        // 9 bytes
        if (target.len < 9) return ERR(encode_result_t, CBOR_ENCODER_ERROR_BUFFER_OVERFLOW);
        target.ptr[0] = (uint8_t)((major_type << 5) | 27);
        uint64_t bytes = htobe64((uint64_t)ui);
        memcpy(&target.ptr[1], &bytes, sizeof(bytes));
        target.len = 9;
        return OK(encode_result_t, target);
    }
}


encode_result_t cbor_encode_string(slice_t string, cbor_type_t type, slice_t target) {
    uint8_t header_size = 0;
    if (string.len <= 23) {
        header_size = 1;
    }
    else if (string.len <= UINT8_MAX) {
        header_size = 2;
    }
    else if (string.len <= UINT16_MAX) {
        header_size = 3;
    }
    else if (string.len <= UINT32_MAX) {
        header_size = 5;
    }
    #if SIZE_MAX > UINT32_MAX
    else if (string.len <= UINT64_MAX) {
        header_size = 9;
    }
    #endif

    if (target.len < (header_size + string.len)) {
        return ERR(encode_result_t, CBOR_ENCODER_ERROR_BUFFER_OVERFLOW);
    }

    cbor_major_type_t major_type = CBOR_MAJOR_TYPE_TEXT_STRING;
    if (type == CBOR_TYPE_BYTE_STRING) {
        major_type = CBOR_MAJOR_TYPE_BYTE_STRING;
    }

    write_len_header(string.len, major_type, target);

    memcpy(&target.ptr[header_size], string.ptr, string.len);

    target.len = header_size + string.len;
    return OK(encode_result_t, target);
}

encode_result_t cbor_encode_simple(cbor_simple_t simple, slice_t target) {
    if (target.len < 1) {
        return ERR(encode_result_t, CBOR_ENCODER_ERROR_BUFFER_OVERFLOW);
    }

    uint8_t simple_value;

    switch (simple)
    {
    case CBOR_SIMPLE_FALSE:
        simple_value = 20;
        break;
    case CBOR_SIMPLE_TRUE:
        simple_value = 21;
        break;
    case CBOR_SIMPLE_NULL:
        simple_value = 22;
        break;
    case CBOR_SIMPLE_UNDEFINED:
        simple_value = 23;
        break;
    default:
        return ERR(encode_result_t, CBOR_ENCODER_TODO);
    }

    target.ptr[0] = (uint8_t)((CBOR_MAJOR_TYPE_SIMPLE << 5) | simple_value);
    target.len = 1;
    return OK(encode_result_t, target);
}

enum cbor_float_precision {
    CBOR_FLOAT_PRECISION_HALF,
    CBOR_FLOAT_PRECISION_SINGLE,
    CBOR_FLOAT_PRECISION_DOUBLE
};

encode_result_t cbor_encode_float(float value, enum cbor_float_precision precision, slice_t target) {
    if (target.len < 5) {
        return ERR(encode_result_t, CBOR_ENCODER_ERROR_BUFFER_OVERFLOW);
    }

    switch (precision) {
        case CBOR_FLOAT_PRECISION_HALF:
            return ERR(encode_result_t, CBOR_ENCODER_TODO);
        case CBOR_FLOAT_PRECISION_SINGLE:
            target.ptr[0] = (uint8_t)((CBOR_MAJOR_TYPE_SIMPLE << 5) | 26);
            uint32_t bytes;
            memcpy(&bytes, &value, sizeof(value));
            bytes = htobe32(bytes);
            memcpy(&target.ptr[1], &bytes, sizeof(bytes));
            target.len = 5;
            break;
        case CBOR_FLOAT_PRECISION_DOUBLE:
            return ERR(encode_result_t, CBOR_ENCODER_TODO);
    }

    return OK(encode_result_t, target);
}

encode_result_t cbor_encode_value_array(cbor_value_slice_t values, slice_t target) {
    slice_t current = target;
    size_t total_len = 0;

    if (!values.ptr) {
        return ERR(encode_result_t, CBOR_ENCODER_NULL_PTR_ERROR);
    }

    // Encode the header
    uint8_t header_size = write_len_header(values.len, CBOR_MAJOR_TYPE_ARRAY, current);
    current.ptr += header_size;
    current.len -= header_size;
    total_len += header_size;

    // Encode the elements
    for (size_t i = 0; i < values.len; i++) {
        encode_result_t encoded = encode(values.ptr[i], current);
        if (encoded.is_error) {
            return encoded;
        }
        current.ptr += encoded.ok.len;
        current.len -= encoded.ok.len;
        total_len += encoded.ok.len;
    }

    /**
     *    |<               target  '         '        >|
     *    .                        |<current>|         .
     *    |<        returned slice .        >|         .
     */
    
    return OK(encode_result_t, ((slice_t){
        .ptr = target.ptr,
        .len = total_len
    }));
}

encode_result_t cbor_encode_value_map(cbor_pair_slice_t pairs, slice_t target) {
    slice_t current = target;
    size_t total_len = 0;

    if (!pairs.ptr) {
        return ERR(encode_result_t, CBOR_ENCODER_NULL_PTR_ERROR);
    }

    // Encode the header
    uint8_t header_size = write_len_header(pairs.len, CBOR_MAJOR_TYPE_MAP, current);
    current.ptr += header_size;
    current.len -= header_size;
    total_len += header_size;

    // Encode the elements
    for (size_t i = 0; i < pairs.len; i++) {

        encode_result_t encoded_first = encode(pairs.ptr[i].first, current);
        if (encoded_first.is_error) {
            return encoded_first;
        }
        current.ptr += encoded_first.ok.len;
        current.len -= encoded_first.ok.len;
        total_len += encoded_first.ok.len;

        encode_result_t encoded_second = encode(pairs.ptr[i].second, current);
        if (encoded_second.is_error) {
            return encoded_second;
        }
        current.ptr += encoded_second.ok.len;
        current.len -= encoded_second.ok.len;
        total_len += encoded_second.ok.len;
    }

    /**
     *    |<               target  '         '        >|
     *    .                        |<current>|         .
     *    |<        returned slice .        >|         .
     */

    return OK(encode_result_t, ((slice_t){
        .ptr = target.ptr,
        .len = total_len
    }));
}

FN_RESULT(slice_t, cbor_encode_error_t,
encode, cbor_value_t value, slice_t target) {
    switch (value.type)
    {
        case CBOR_TYPE_INTEGER:
            return cbor_encode_integer(value.value.integer, target);
        case CBOR_TYPE_BYTE_STRING:
        case CBOR_TYPE_TEXT_STRING:
            return cbor_encode_string(value.value.bytes, value.type, target);
        case CBOR_TYPE_ARRAY:
            return ERR(encode_result_t, CBOR_ENCODER_UNKNOWN_SIZE);
        case CBOR_TYPE_MAP:
            return ERR(encode_result_t, CBOR_ENCODER_UNKNOWN_SIZE);
        case CBOR_TYPE_TAG:
            return ERR(encode_result_t, CBOR_ENCODER_TODO);
        case CBOR_TYPE_SIMPLE:
            return cbor_encode_simple(value.value.simple, target);
        case CBOR_TYPE_FLOAT:
            return cbor_encode_float(value.value.floating, CBOR_FLOAT_PRECISION_DEFAULT, target);
        case CBOR_ENCODE_TYPE_VALUES:
            return cbor_encode_value_array(value.value.values, target);
        case CBOR_ENCODE_TYPE_PAIRS:
            return cbor_encode_value_map(value.value.pairs, target);
        case CBOR_ENCODE_TYPE_CUSTOM_ENCODER:
            {
                custom_encoder_result_t result = value.value.custom_encoder.encoder(target, value.value.custom_encoder.argument);
                if (result.is_error) {
                    return ERR(encode_result_t, CBOR_ENCODER_TODO);
                }
                return OK(encode_result_t, result.ok);
            }
        default:
            return ERR(encode_result_t, CBOR_ENCODER_TODO);
    }
    return ERR(encode_result_t, CBOR_ENCODER_TODO);
}
