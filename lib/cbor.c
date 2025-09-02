#include "cbor.h"
#include <stdio.h>
#include <stdint.h>


/*--------------------------------------------------------------------------*/
cbor_major_type_t get_major_type(const uint8_t *data)
{
    return (*data) >> 5;
}
/*--------------------------------------------------------------------------*/
argument_t get_argument(const uint8_t* data) {
    uint8_t argument = (*data) & 0b00011111;

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
                    value.value.simple = CBOR_SIMPLE_FALSE;
                    break;
                case 22:
                    value.value.simple = CBOR_SIMPLE_FALSE;
                    break;
                case 23:
                    value.value.simple = CBOR_SIMPLE_FALSE;
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
            value.value.floating = (float)*(_Float32*)&value.argument._4byte;
            value.type = CBOR_TYPE_FLOAT;
            break;
        case ARGUMENT_8BYTE:
            // f64
            value.value.floating = (float)*(_Float64*)&value.argument._8byte;
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