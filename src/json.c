#include "cbor/json.h"
#include "cbor/debug.h"

static inline void indent_space(FILE* target, int count) {
    for(int i = 0; i < count; i++) {
        fprintf(target, " ");
    }
}

/*--------------------------------------------------------------------------*/
cbor_custom_processor_result_t print_single_json(const cbor_value_t* value, void* arg) {
    if (!arg) return ERR(cbor_custom_processor_result_t, 0);

    json_print_state_t* state = arg;
    
    indent_space(state->target, state->indent);
    if (value->type == CBOR_TYPE_MAP) {
        json_print_state_t new_state;
        memcpy(&new_state, state, sizeof(new_state));
        new_state.indent += state->indent_step;
        int counter = value->value.map.length;
        new_state.counter = &counter;
        fprintf(state->target, "{");
        if (state->newline) fprintf(state->target, "\n");
        cbor_process_map(value->value.map, print_pair_json, &new_state);
        indent_space(state->target, state->indent);
        fprintf(state->target, "}");
    }        
    else if (value->type == CBOR_TYPE_ARRAY) {
        json_print_state_t new_state;
        memcpy(&new_state, state, sizeof(new_state));
        new_state.indent += state->indent_step;
        int counter = value->value.array.length;
        new_state.counter = &counter;
        fprintf(state->target, "[");
        if (state->newline) fprintf(state->target, "\n");
        cbor_process_array(value->value.array, print_single_json, &new_state);
        indent_space(state->target, state->indent);
        fprintf(state->target, "]");
    }
    else {
        print_cbor_value_json(*value, state->indent, state->target);
    }
    (*state->counter)--;
    if (*state->counter > 0) fprintf(state->target, ",");
    if (state->newline) fprintf(state->target, "\n");
    fflush(state->target);
    return CBOR_CUSTOM_PROCESSOR_OK();
}
/*--------------------------------------------------------------------------*/
cbor_custom_processor_result_t print_pair_json(const cbor_value_t* key, const cbor_value_t* value, void* arg) {
    if (!arg) return ERR(cbor_custom_processor_result_t, 0);
    
    json_print_state_t* state = arg;
    
    indent_space(state->target, state->indent);
    print_cbor_value_json(*key, state->indent, state->target);
    fprintf(state->target, " : ");
    if (value->type == CBOR_TYPE_MAP) {
        json_print_state_t new_state;
        memcpy(&new_state, state, sizeof(json_print_state_t));
        new_state.indent += state->indent_step;
        int counter = value->value.map.length;
        new_state.counter = &counter;
        fprintf(state->target, "{");
        if (state->newline) fprintf(state->target, "\n");
        cbor_process_map(value->value.map, print_pair_json, &new_state);
        indent_space(state->target, state->indent);
        fprintf(state->target, "}");
    }
    else if (value->type == CBOR_TYPE_ARRAY) {
        json_print_state_t new_state;
        memcpy(&new_state, state, sizeof(new_state));
        new_state.indent += state->indent_step;
        int counter = value->value.array.length;
        new_state.counter = &counter;
        fprintf(state->target, "[");
        if (state->newline) fprintf(state->target, "\n");
        cbor_process_array(value->value.array, print_single_json, &new_state);
        indent_space(state->target, state->indent);
        fprintf(state->target, "]");
    }
    else {
        print_cbor_value_json(*value, state->indent, state->target);
    }
    (*state->counter)--;
    if (*state->counter > 0) fprintf(state->target, ",");
    if (state->newline) fprintf(state->target, "\n");
    fflush(state->target);
    return CBOR_CUSTOM_PROCESSOR_OK();
}
/*--------------------------------------------------------------------------*/
void print_cbor_value_json(cbor_value_t value, int indent, FILE* target)
{
    switch (value.type)
    {
    case CBOR_TYPE_INTEGER:
        fprintf(target, "%"PRIi64, (long long int)value.value.integer);
        break;
    case CBOR_TYPE_BYTE_STRING:
        fprintf(target, "b\"%.*s\"", (int)value.value.bytes.len, value.value.bytes.ptr);
        break;
    case CBOR_TYPE_TEXT_STRING:
        fprintf(target, "\"%.*s\"", (int)value.value.bytes.len, value.value.bytes.ptr);
        break;
    case CBOR_TYPE_ARRAY:
        break;
    case CBOR_TYPE_MAP:
        break;
    case CBOR_TYPE_SIMPLE:
        switch (value.value.simple) {
            case CBOR_SIMPLE_FALSE:
                fprintf(target, "false");
                break;
            case CBOR_SIMPLE_TRUE:
                fprintf(target, "true");
                break;
            case CBOR_SIMPLE_NULL: 
                fprintf(target, "null");
                break;
            case CBOR_SIMPLE_UNDEFINED:
                fprintf(target, "undefined");
                break;
            case CBOR_SIMPLE_ERROR_RESERVED:
                fprintf(target, "\"Simple: Error Reserved\"");
                break;
            case CBOR_SIMPLE_ERROR_UNASSIGNED:
                fprintf(target, "\"Simple: Error Unassigned\"");
                break;
        }
        break;
    case CBOR_TYPE_FLOAT:
        fprintf(target, "%f", (double)value.value.floating);
        break;
    default:
        /* Other CBOR types are not printed yet */
        fprintf(target, "\"CBOR type %d\"", (int)value.type);
        break;
    }

    fflush(target);
}