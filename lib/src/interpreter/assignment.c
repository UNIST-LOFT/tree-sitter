#include "tree_sitter/api.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

/* Every store below goes through a pointer of the destination's own width. Storing eight bytes into
 * a narrower destination -- what these did before -- reaches past it: `max_ctby = ...` on a 4-byte
 * u32 also cleared the 4 bytes above it, which in vvc_get_ctb_info_in_slice was min_ctby, the lower
 * bound of the loop that followed. The value is truncated to that width, as a C assignment does. */
#define HANDLE_ASSIGN_OP_INT(op, lhs, rhs, rhs_field) \
    switch ((lhs).type.size) { \
        case 1: \
            *(int8_t*)(lhs).reference op (int8_t)(rhs).value.rhs_field; \
            (lhs).value.int64 op (int8_t)(rhs).value.rhs_field; \
            break; \
        case 2: \
            *(int16_t*)(lhs).reference op (int16_t)(rhs).value.rhs_field; \
            (lhs).value.int64 op (int16_t)(rhs).value.rhs_field; \
            break; \
        case 4: \
            *(int32_t*)(lhs).reference op (int32_t)(rhs).value.rhs_field; \
            (lhs).value.int64 op (int32_t)(rhs).value.rhs_field; \
            break; \
        case 8: \
            *(int64_t*)(lhs).reference op (int64_t)(rhs).value.rhs_field; \
            (lhs).value.int64 op (int64_t)(rhs).value.rhs_field; \
            break; \
        default: \
            TS_PRINTF_ERROR("size of LHS of assignment %" PRIu32, (lhs).type.size); \
    }

#define HANDLE_ASSIGN_OP_UINT(op, lhs, rhs, rhs_field) \
    switch ((lhs).type.size) { \
        case 1: \
            *(uint8_t*)(lhs).reference op (uint8_t)(rhs).value.rhs_field; \
            (lhs).value.uint64 op (uint8_t)(rhs).value.rhs_field; \
            break; \
        case 2: \
            *(uint16_t*)(lhs).reference op (uint16_t)(rhs).value.rhs_field; \
            (lhs).value.uint64 op (uint16_t)(rhs).value.rhs_field; \
            break; \
        case 4: \
            *(uint32_t*)(lhs).reference op (uint32_t)(rhs).value.rhs_field; \
            (lhs).value.uint64 op (uint32_t)(rhs).value.rhs_field; \
            break; \
        case 8: \
            *(uint64_t*)(lhs).reference op (uint64_t)(rhs).value.rhs_field; \
            (lhs).value.uint64 op (uint64_t)(rhs).value.rhs_field; \
            break; \
        default: \
            TS_PRINTF_ERROR("size of LHS of assignment %" PRIu32, (lhs).type.size); \
    }

#define HANDLE_ASSIGN_OP_DOUBLE(op, lhs, rhs, rhs_field) \
    switch ((lhs).type.size) { \
        case 4: \
            *(float*)(lhs).reference op (float)(rhs).value.rhs_field; \
            (lhs).value.double64 op (float)(rhs).value.rhs_field; \
            break; \
        case 8: \
            *(double*)(lhs).reference op (double)(rhs).value.rhs_field; \
            (lhs).value.double64 op (double)(rhs).value.rhs_field; \
            break; \
        case 16: \
            *(long double*)(lhs).reference op (long double)(rhs).value.rhs_field; \
            (lhs).value.double64 op (long double)(rhs).value.rhs_field; \
            break; \
        default: \
            TS_PRINTF_ERROR("size of LHS of assignment %" PRIu32, (lhs).type.size); \
    }

#define HANDLE_ASSIGN_OP(op, lhs, rhs) \
    switch ((lhs).type.category) { \
        case TSNodeObjectTypeInt: \
            switch ((rhs).type.category) { \
                case TSNodeObjectTypeInt: \
                    HANDLE_ASSIGN_OP_INT(op, lhs, rhs, int64); \
                    break; \
                case TSNodeObjectTypeUInt: \
                    HANDLE_ASSIGN_OP_INT(op, lhs, rhs, uint64); \
                    break; \
                case TSNodeObjectTypeDouble: \
                    HANDLE_ASSIGN_OP_INT(op, lhs, rhs, double64); \
                    break; \
                case TSNodeObjectTypePointer: \
                    HANDLE_ASSIGN_OP_INT(op, lhs, rhs, pointer); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in assignment to int: %s\n", (rhs).type.name); \
            } \
            break; \
        case TSNodeObjectTypeUInt: \
            switch ((rhs).type.category) { \
                case TSNodeObjectTypeInt: \
                    HANDLE_ASSIGN_OP_UINT(op, lhs, rhs, int64); \
                    break; \
                case TSNodeObjectTypeUInt: \
                    HANDLE_ASSIGN_OP_UINT(op, lhs, rhs, uint64); \
                    break; \
                case TSNodeObjectTypeDouble: \
                    HANDLE_ASSIGN_OP_UINT(op, lhs, rhs, double64); \
                    break; \
                case TSNodeObjectTypePointer: \
                    HANDLE_ASSIGN_OP_UINT(op, lhs, rhs, pointer); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in assignment to uint: %s\n", (rhs).type.name); \
            } \
            break; \
        /* A pointer RHS has no case under a double LHS: a pointer cannot be cast to a floating type, \
           and `double d = p;` is not an assignment C accepts either */ \
        case TSNodeObjectTypeDouble: \
            switch ((rhs).type.category) { \
                case TSNodeObjectTypeInt: \
                    HANDLE_ASSIGN_OP_DOUBLE(op, lhs, rhs, int64); \
                    break; \
                case TSNodeObjectTypeUInt: \
                    HANDLE_ASSIGN_OP_DOUBLE(op, lhs, rhs, uint64); \
                    break; \
                case TSNodeObjectTypeDouble: \
                    HANDLE_ASSIGN_OP_DOUBLE(op, lhs, rhs, double64); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in assignment to double: %s\n", (rhs).type.name); \
            } \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in assignment: %s\n", (lhs).type.name); \
    }

#define HANDLE_ASSIGN(op, lhs, rhs) \
    switch ((lhs).type.category) { \
        case TSNodeObjectTypeInt: \
        case TSNodeObjectTypeUInt: \
        case TSNodeObjectTypeDouble: \
            HANDLE_ASSIGN_OP(op, lhs, rhs) \
            break; \
        case TSNodeObjectTypePointer: \
            *(void**)(lhs).reference op (rhs).value.pointer; \
            (lhs).value.pointer op (rhs).value.pointer; \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in assignment: %s\n", (lhs).type.name); \
    }

/* `ptr += n` / `ptr -= n`, scaled the way C scales it: by the size of what the pointer points at,
 * once.
 *
 * The two scalings this had before multiplied instead by the size of the *integer operand's* type
 * and then again by the width of the `uint64_t*` the store went through. On a byte pointer with a
 * size_t operand that is 64x: `out += unescapedSize` in xmlEscapeText (libxml2-417062198) moved
 * `out` 192 bytes for a 3-byte copy, off the end of the 51-byte buffer and into a chunk an earlier
 * xmlRealloc had freed, so the `*out = 0` after the loop reported a heap-use-after-free.
 *
 * A step is byte arithmetic on the pointer's current value, so it is computed on an unsigned char*
 * and stored back verbatim -- both into the variable and into the cached copy, which the two
 * separate statements used to leave disagreeing (24 vs 192 in that same expression). An array's
 * `reference` already *is* its elements (MetaproVarTypeArray, see ts_interpreter_load_variable), and
 * `arr += n` is not assignable in C anyway, so that case leaves the storage alone rather than
 * writing an address over the first element. */
#define HANDLE_ASSIGN_STEP(op, lhs, rhs, step) \
    do { \
        uint32_t _elem_size = (lhs).array_element_type.size != 0 ? (lhs).array_element_type.size : 1; \
        unsigned char* _stepped = (unsigned char*)(lhs).value.pointer; \
        _stepped op (int64_t)(step) * (int64_t)_elem_size; \
        if ((lhs).reference != NULL && (lhs).reference != (lhs).value.pointer) { \
            *(void**)(lhs).reference = (void*)_stepped; \
        } \
        (lhs).value.pointer = (void*)_stepped; \
    } while (0)

#define HANDLE_ASSIGN_PTR_STEP(op, lhs, rhs) \
    switch ((rhs).type.category) { \
        case TSNodeObjectTypeInt: \
            HANDLE_ASSIGN_STEP(op, lhs, rhs, (rhs).value.int64); \
            break; \
        case TSNodeObjectTypeUInt: \
            HANDLE_ASSIGN_STEP(op, lhs, rhs, (uint64_t)(rhs).value.uint64); \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported RHS type in pointer arithmetic: %s\n", (rhs).type.name); \
    }

#define HANDLE_ASSIGN_ADD(op, lhs, rhs) \
    switch ((lhs).type.category) { \
        case TSNodeObjectTypeInt: \
        case TSNodeObjectTypeUInt: \
        case TSNodeObjectTypeDouble: \
            HANDLE_ASSIGN_OP(op, lhs, rhs) \
            break; \
        case TSNodeObjectTypePointer: \
            HANDLE_ASSIGN_PTR_STEP(op, lhs, rhs) \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in assignment: %s\n", (lhs).type.name); \
    }

#define HANDLE_ASSIGN_NO_DOUBLE(op, lhs, rhs) \
    switch ((lhs).type.category) { \
        case TSNodeObjectTypeInt: \
            switch ((rhs).type.category) { \
                case TSNodeObjectTypeInt: \
                    HANDLE_ASSIGN_OP_INT(op, lhs, rhs, int64); \
                    break; \
                case TSNodeObjectTypeUInt: \
                    HANDLE_ASSIGN_OP_INT(op, lhs, rhs, uint64); \
                    break; \
                case TSNodeObjectTypeDouble: \
                    HANDLE_ASSIGN_OP_INT(op, lhs, rhs, double64); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in assignment to int: %s\n", (rhs).type.name); \
            } \
            break; \
        case TSNodeObjectTypeUInt: \
            switch ((rhs).type.category) { \
                case TSNodeObjectTypeInt: \
                    HANDLE_ASSIGN_OP_UINT(op, lhs, rhs, int64); \
                    break; \
                case TSNodeObjectTypeUInt: \
                    HANDLE_ASSIGN_OP_UINT(op, lhs, rhs, uint64); \
                    break; \
                case TSNodeObjectTypeDouble: \
                    HANDLE_ASSIGN_OP_UINT(op, lhs, rhs, double64); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in assignment to uint: %s\n", (rhs).type.name); \
            } \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in assignment: %s\n", (lhs).type.name); \
    }

TSNodeObject ts_interpreter_assign(TSNode node, uint64_t var_count, TSNodeObject* vars, TSTypeInfo* type_info_table) {
    TSNode left = ts_node_named_child(node, 0);
    TSNode right = ts_node_named_child(node, 1);

    // Handle LHS
    // It may have simple variable or subscript
    TSNodeObject left_obj = ts_interpreter_simulate(left, var_count, vars, type_info_table);

    // Assign
    char* op = ts_node_find_value(node);
    TSNodeObject right_value = ts_interpreter_simulate(right, var_count, vars, type_info_table);
    if (strcmp(op, "=") == 0) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        HANDLE_ASSIGN(=, left_obj, right_value);
#pragma GCC diagnostic pop
    }
    else if (strcmp(op, "+=") == 0) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        HANDLE_ASSIGN_ADD(+=, left_obj, right_value);
#pragma GCC diagnostic pop
    }
    else if (strcmp(op, "-=") == 0) {
        // A pointer steps back by the same rule it steps forward by; HANDLE_ASSIGN_OP has no
        // pointer case at all, so `p -= n` used to abort as an unsupported assignment type.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        HANDLE_ASSIGN_ADD(-=, left_obj, right_value);
#pragma GCC diagnostic pop
    }
    else if (strcmp(op, "*=") == 0) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        HANDLE_ASSIGN_OP(*=, left_obj, right_value);
#pragma GCC diagnostic pop
    }
    else if (strcmp(op, "/=") == 0) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        HANDLE_ASSIGN_OP(/=, left_obj, right_value);
#pragma GCC diagnostic pop
    }
    else if (strcmp(op, "%%=") == 0) {
        HANDLE_ASSIGN_NO_DOUBLE(%=, left_obj, right_value);
    }
    else if (strcmp(op, "&=") == 0) {
        HANDLE_ASSIGN_NO_DOUBLE(&=, left_obj, right_value);
    }
    else if (strcmp(op, "|=") == 0) {
        HANDLE_ASSIGN_NO_DOUBLE(|=, left_obj, right_value);
    }
    else if (strcmp(op, "^=") == 0) {
        HANDLE_ASSIGN_NO_DOUBLE(^=, left_obj, right_value);
    }
    else if (strcmp(op, "<<=") == 0) {
        HANDLE_ASSIGN_NO_DOUBLE(<<=, left_obj, right_value);
    }
    else if (strcmp(op, ">>=") == 0) {
        HANDLE_ASSIGN_NO_DOUBLE(>>=, left_obj, right_value);
    }
    else {
        TS_PRINTF_ERROR("Unsupported assignment operator: %s\n", op);
    }
    return left_obj;
}