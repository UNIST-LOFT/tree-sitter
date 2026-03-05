#include "tree_sitter/api.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

#define HANDLE_ASSIGN_OP_INT(op, lhs, rhs, rhs_field) \
    switch ((lhs).size) { \
        case 1: \
            *(int64_t*)(lhs).reference op (int8_t)(rhs).value.rhs_field; \
            (lhs).value.int64 op (int8_t)(rhs).value.rhs_field; \
            break; \
        case 2: \
            *(int64_t*)(lhs).reference op (int16_t)(rhs).value.rhs_field; \
            (lhs).value.int64 op (int16_t)(rhs).value.rhs_field; \
            break; \
        case 4: \
            *(int64_t*)(lhs).reference op (int32_t)(rhs).value.rhs_field; \
            (lhs).value.int64 op (int32_t)(rhs).value.rhs_field; \
            break; \
        case 8: \
            *(int64_t*)(lhs).reference op (int64_t)(rhs).value.rhs_field; \
            (lhs).value.int64 op (int64_t)(rhs).value.rhs_field; \
            break; \
        default: \
            TS_PRINTF_ERROR("size of LHS of assignment %" PRIu64, (lhs).size); \
    }

#define HANDLE_ASSIGN_OP_UINT(op, lhs, rhs, rhs_field) \
    switch ((lhs).size) { \
        case 1: \
            *(uint64_t*)(lhs).reference op (uint8_t)(rhs).value.rhs_field; \
            (lhs).value.uint64 op (uint8_t)(rhs).value.rhs_field; \
            break; \
        case 2: \
            *(uint64_t*)(lhs).reference op (uint16_t)(rhs).value.rhs_field; \
            (lhs).value.uint64 op (uint16_t)(rhs).value.rhs_field; \
            break; \
        case 4: \
            *(uint64_t*)(lhs).reference op (uint32_t)(rhs).value.rhs_field; \
            (lhs).value.uint64 op (uint32_t)(rhs).value.rhs_field; \
            break; \
        case 8: \
            *(uint64_t*)(lhs).reference op (uint64_t)(rhs).value.rhs_field; \
            (lhs).value.uint64 op (uint64_t)(rhs).value.rhs_field; \
            break; \
        default: \
            TS_PRINTF_ERROR("size of LHS of assignment %" PRIu64, (lhs).size); \
    }

#define HANDLE_ASSIGN_OP_DOUBLE(op, lhs, rhs, rhs_field) \
    switch ((lhs).size) { \
        case 4: \
            *(long double*)(lhs).reference op (float)(rhs).value.rhs_field; \
            (lhs).value.double64 op (float)(rhs).value.rhs_field; \
            break; \
        case 8: \
            *(long double*)(lhs).reference op (double)(rhs).value.rhs_field; \
            (lhs).value.double64 op (double)(rhs).value.rhs_field; \
            break; \
        default: \
            TS_PRINTF_ERROR("size of LHS of assignment %" PRIu64, (lhs).size); \
    }

#define HANDLE_ASSIGN_OP(op, lhs, rhs) \
    switch ((lhs).type) { \
        case TSNodeObjectTypeInt: \
            switch ((rhs).type) { \
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
                    TS_PRINTF_ERROR("Unsupported RHS type in assignment to int: %d\n", (rhs).type); \
            } \
            break; \
        case TSNodeObjectTypeUInt: \
            switch ((rhs).type) { \
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
                    TS_PRINTF_ERROR("Unsupported RHS type in assignment to uint: %d\n", (rhs).type); \
            } \
            break; \
        case TSNodeObjectTypeDouble: \
            switch ((rhs).type) { \
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
                    TS_PRINTF_ERROR("Unsupported RHS type in assignment to double: %d\n", (rhs).type); \
            } \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in assignment: %d\n", (lhs).type); \
    }

#define HANDLE_ASSIGN(op, lhs, rhs) \
    switch ((lhs).type) { \
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
            TS_PRINTF_ERROR("Unsupported type in assignment: %d\n", (lhs).type); \
    }

#define HANDLE_ASSIGN_ADD(op, lhs, rhs) \
    switch ((lhs).type) { \
        case TSNodeObjectTypeInt: \
        case TSNodeObjectTypeUInt: \
        case TSNodeObjectTypeDouble: \
            HANDLE_ASSIGN_OP(op, lhs, rhs) \
            break; \
        case TSNodeObjectTypePointer: \
            switch ((rhs).type) { \
                case TSNodeObjectTypeInt: \
                    *(uint64_t**)(lhs).reference op ((int64_t)(rhs).value.int64) * (rhs).size; \
                    (lhs).value.pointer op (int64_t)(((int64_t)(rhs).value.int64) * (rhs).size); \
                    break; \
                case TSNodeObjectTypeUInt: \
                    *(uint64_t**)(lhs).reference op ((uint64_t)(rhs).value.uint64) * (rhs).size; \
                    (lhs).value.pointer op (int64_t)(((uint64_t)(rhs).value.uint64) * (rhs).size); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in pointer arithmetic: %d\n", (rhs).type); \
            } \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in assignment: %d\n", (lhs).type); \
    }

#define HANDLE_ASSIGN_NO_DOUBLE(op, lhs, rhs) \
    switch ((lhs).type) { \
        case TSNodeObjectTypeInt: \
            switch ((rhs).type) { \
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
                    TS_PRINTF_ERROR("Unsupported RHS type in assignment to int: %d\n", (rhs).type); \
            } \
            break; \
        case TSNodeObjectTypeUInt: \
            switch ((rhs).type) { \
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
                    TS_PRINTF_ERROR("Unsupported RHS type in assignment to uint: %d\n", (rhs).type); \
            } \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in assignment: %d\n", (lhs).type); \
    }

TSNodeObject ts_interpreter_assign(TSNode node, uint64_t var_count, TSNodeObject* vars) {
    TSNode left = ts_node_named_child(node, 0);
    TSNode right = ts_node_named_child(node, 1);

    // find LHS from vars
    char* left_value = ts_node_find_value(left);
    TSNodeObject left_obj;
    int32_t found = 0;
    for (size_t i=0;i<var_count;i++) {
        if (strcmp(left_value,vars[i].name) == 0) {
            left_obj = vars[i];
            found = 1;
        }
    }
    if (!found) {
        TS_PRINTF_ERROR("Variable %s not found in variable list\n", left_value);
    }

    // Assign
    char* op = ts_node_find_value(node);
    TSNodeObject right_value = ts_interpreter_simulate(right, var_count, vars);
    if (strcmp(op, "=") == 0) {
        HANDLE_ASSIGN(=, left_obj, right_value);
    }
    else if (strcmp(op, "+=") == 0) {
        HANDLE_ASSIGN_ADD(+=, left_obj, right_value);
    }
    else if (strcmp(op, "-=") == 0) {
        HANDLE_ASSIGN_OP(-=, left_obj, right_value);
    }
    else if (strcmp(op, "*=") == 0) {
        HANDLE_ASSIGN_OP(*=, left_obj, right_value);
    }
    else if (strcmp(op, "/=") == 0) {
        HANDLE_ASSIGN_OP(/=, left_obj, right_value);
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