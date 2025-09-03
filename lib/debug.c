#include <stdio.h>
#include "debug.h"

/*--------------------------------------------------------------------------*/
void print_single(const cbor_value_t* value, void* arg) {

    #ifdef CBOR_DEBUG_REPR
    print_cbor_value(*value, (size_t)arg);
    if (value->type == CBOR_TYPE_MAP) {
        process_map(value->value.map, print_pair, (void*)((size_t)arg + (size_t)4));
    }        
    if (value->type == CBOR_TYPE_ARRAY) {
        process_array(value->value.array, print_single, (void*)((size_t)arg + (size_t)4));
    }
    #endif
}
/*--------------------------------------------------------------------------*/
void print_pair(const cbor_value_t* key, const cbor_value_t* value, void* arg) {

    #ifdef CBOR_DEBUG_REPR
    print_cbor_value(*key, (size_t)arg);
    print_cbor_value(*value, (size_t)arg);
    if (value->type == CBOR_TYPE_MAP) {
        process_map(value->value.map, print_pair, (void*)((size_t)arg + (size_t)4));
    }
    if (value->type == CBOR_TYPE_ARRAY) {
        process_array(value->value.array, print_single, (void*)((size_t)arg + (size_t)4));
    }
    #endif
}
/*--------------------------------------------------------------------------*/
void print_cbor_type(cbor_major_type_t type) {
    switch (type) {
    case CBOR_MAJOR_TYPE_UNSIGNED_INTEGER:
        printf("CBOR Type: Unsigned Integer");
        break;
    case CBOR_MAJOR_TYPE_NEGATIVE_INTEGER:
        printf("CBOR Type: Negative Integer");
        break;
    case CBOR_TYPE_BYTE_STRING:
        printf("CBOR Type: Byte String");
        break;
    case CBOR_TYPE_TEXT_STRING:
        printf("CBOR Type: Text String");
        break;
    case CBOR_TYPE_ARRAY:
        printf("CBOR Type: Array");
        break;
    case CBOR_TYPE_MAP:
        printf("CBOR Type: Map");
        break;
    default:
        printf("CBOR Type: Unknown (%d)", (int)type);
        break;
    }
}
/*--------------------------------------------------------------------------*/
void print_cbor_value(cbor_value_t value, int indent)
{
    for (int i = 0; i < indent; i++) {
        printf(".");
    }
    switch (value.type)
    {
    case CBOR_TYPE_INTEGER:
        printf("Integer: %"PRIi64"\n", value.value.integer);
        break;
    case CBOR_TYPE_BYTE_STRING:
        printf("Byte String: %.*s\n", (int)value.value.bytes.len, value.value.bytes.ptr);
        break;
    case CBOR_TYPE_TEXT_STRING:
        printf("Text String: %.*s\n", (int)value.value.bytes.len, value.value.bytes.ptr);
        break;
    case CBOR_TYPE_ARRAY:
        printf("Array: %"PRIu32"\n", value.value.array.length);
        break;
    case CBOR_TYPE_MAP:
        printf("Map: %"PRIu32"\n", value.value.map.length);
        break;
    case CBOR_TYPE_SIMPLE:
        switch (value.value.simple) {
            case CBOR_SIMPLE_FALSE:
                printf("Simple: False\n");
                break;
            case CBOR_SIMPLE_TRUE:
                printf("Simple: True\n");
                break;
            case CBOR_SIMPLE_NULL: 
                printf("Simple: Null\n");
                break;
            case CBOR_SIMPLE_UNDEFINED:
                printf("Simple: Undefined\n");
                break;
            case CBOR_SIMPLE_ERROR_RESERVED:
                printf("Simple: Error Reserved\n");
                break;
            case CBOR_SIMPLE_ERROR_UNASSIGNED:
                printf("Simple: Error Unassigned\n");
                break;
        }
        break;
    case CBOR_TYPE_FLOAT:
        
        printf("Float: %f\n", (double)value.value.floating);
        break;
    default:
        /* Other CBOR types are not printed yet */
        printf("CBOR type %d\n", (int)value.type);
        break;
    }
}
