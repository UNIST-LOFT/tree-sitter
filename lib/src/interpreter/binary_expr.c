#include "tree_sitter/api.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

/* Arithmetic */
#define HANDLE_ARITH_OP_INT(op, lhs, rhs, rhs_field, result) \
    switch ((lhs).type.size) { \
        case 1: \
            result.value.int64 = (int8_t)(lhs).value.int64 op (int8_t)(rhs).value.rhs_field; \
            break; \
        case 2: \
            result.value.int64 = (int16_t)(lhs).value.int64 op (int16_t)(rhs).value.rhs_field; \
            break; \
        case 4: \
            result.value.int64 = (int32_t)(lhs).value.int64 op (int32_t)(rhs).value.rhs_field; \
            break; \
        case 8: \
            result.value.int64 = (int64_t)(lhs).value.int64 op (int64_t)(rhs).value.rhs_field; \
            break; \
        default: \
            TS_PRINTF_ERROR("size of LHS of binary op %" PRIu32, (lhs).type.size); \
    }

#define HANDLE_ARITH_OP_UINT(op, lhs, rhs, rhs_field, result) \
    switch ((lhs).type.size) { \
        case 1: \
            result.value.uint64 = (uint8_t)(lhs).value.uint64 op (uint8_t)(rhs).value.rhs_field; \
            break; \
        case 2: \
            result.value.uint64 = (uint16_t)(lhs).value.uint64 op (uint16_t)(rhs).value.rhs_field; \
            break; \
        case 4: \
            result.value.uint64 = (uint32_t)(lhs).value.uint64 op (uint32_t)(rhs).value.rhs_field; \
            break; \
        case 8: \
            result.value.uint64 = (uint64_t)(lhs).value.uint64 op (uint64_t)(rhs).value.rhs_field; \
            break; \
        default: \
            TS_PRINTF_ERROR("size of LHS of binary op %" PRIu32, (lhs).type.size); \
    }

#define HANDLE_ARITH_OP_DOUBLE(op, lhs, rhs, rhs_field, result) \
    switch ((lhs).type.size) { \
        case 4: \
            result.value.double64 = (float)(lhs).value.double64 op (float)(rhs).value.rhs_field; \
            break; \
        case 8: \
            result.value.double64 = (double)(lhs).value.double64 op (double)(rhs).value.rhs_field; \
            break; \
        default: \
            TS_PRINTF_ERROR("size of LHS of binary op %" PRIu32, (lhs).type.size); \
    }

#define HANDLE_ARITH_OP(op, lhs, rhs, result) \
    switch ((lhs).type.category) { \
        case TSNodeObjectTypeInt: \
            switch ((rhs).type.category) { \
                case TSNodeObjectTypeInt: \
                    HANDLE_ARITH_OP_INT(op, lhs, rhs, int64, result); \
                    break; \
                case TSNodeObjectTypeUInt: \
                    HANDLE_ARITH_OP_INT(op, lhs, rhs, uint64, result); \
                    break; \
                case TSNodeObjectTypeDouble: \
                    HANDLE_ARITH_OP_INT(op, lhs, rhs, double64, result); \
                    break; \
                case TSNodeObjectTypePointer: \
                    HANDLE_ARITH_OP_INT(op, lhs, rhs, pointer, result); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to int: %s\n", (rhs).type.name); \
            } \
            break; \
        case TSNodeObjectTypeUInt: \
            switch ((rhs).type.category) { \
                case TSNodeObjectTypeInt: \
                    HANDLE_ARITH_OP_UINT(op, lhs, rhs, int64, result); \
                    break; \
                case TSNodeObjectTypeUInt: \
                    HANDLE_ARITH_OP_UINT(op, lhs, rhs, uint64, result); \
                    break; \
                case TSNodeObjectTypeDouble: \
                    HANDLE_ARITH_OP_UINT(op, lhs, rhs, double64, result); \
                    break; \
                case TSNodeObjectTypePointer: \
                    HANDLE_ARITH_OP_UINT(op, lhs, rhs, pointer, result); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to uint: %s\n", (rhs).type.name); \
            } \
            break; \
        case TSNodeObjectTypeDouble: \
            switch ((rhs).type.category) { \
                case TSNodeObjectTypeInt: \
                    HANDLE_ARITH_OP_DOUBLE(op, lhs, rhs, int64, result); \
                    break; \
                case TSNodeObjectTypeUInt: \
                    HANDLE_ARITH_OP_DOUBLE(op, lhs, rhs, uint64, result); \
                    break; \
                case TSNodeObjectTypeDouble: \
                    HANDLE_ARITH_OP_DOUBLE(op, lhs, rhs, double64, result); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to double: %s\n", (rhs).type.name); \
            } \
            break; \
        case TSNodeObjectTypePointer: \
            switch ((rhs).type.category) { \
                case TSNodeObjectTypeInt: \
                    result.type = (lhs).type; \
                    result.value.int64 = (int64_t)(lhs).value.pointer op (rhs).value.int64; \
                    result.reference = (void*)&result.value.int64; \
                    break; \
                case TSNodeObjectTypeUInt: \
                    result.type = (lhs).type; \
                    result.value.uint64 = (uint64_t)(lhs).value.pointer op (rhs).value.uint64; \
                    result.reference = (void*)&(result.value.uint64); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op with pointer LHS: %s\n", (rhs).type.name); \
            } \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in binary op: %s\n", (lhs).type.name); \
    }

#define HANDLE_ARITH_SUB(op, lhs, rhs, result) \
    switch ((lhs).type.category) { \
        case TSNodeObjectTypeInt: \
        case TSNodeObjectTypeUInt: \
        case TSNodeObjectTypeDouble: \
            HANDLE_ARITH_OP(op, lhs, rhs, result) \
            break; \
        case TSNodeObjectTypePointer: \
            switch ((rhs).type.category) { \
                case TSNodeObjectTypeInt: \
                    result.type = (lhs).type; \
                    result.value.pointer = (void*)((uint8_t*)((lhs).value.pointer) op \
                        (((int64_t)(rhs).value.int64) * (lhs).array_element_type.size)); \
                    result.reference = (void*)&(result.value.pointer); \
                    result.array_element_type = (lhs).array_element_type; \
                    break; \
                case TSNodeObjectTypeUInt: \
                    result.type = (lhs).type; \
                    result.value.pointer = (void*)((uint8_t*)((lhs).value.pointer) op \
                        (((uint64_t)(rhs).value.uint64) * (lhs).array_element_type.size)); \
                    result.reference = (void*)&(result.value.pointer); \
                    result.array_element_type = (lhs).array_element_type; \
                    break; \
                case TSNodeObjectTypePointer: \
                    result.type = (lhs).type; \
                    result.value.int64 = (int64_t)((void*)(lhs).value.pointer op (rhs).value.pointer); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in pointer arithmetic: %s\n", (rhs).type.name); \
            } \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in binary op: %s\n", (lhs).type.name); \
    }

#define HANDLE_ARITH_ADD(op, lhs, rhs, result) \
    switch ((lhs).type.category) { \
        case TSNodeObjectTypeInt: \
        case TSNodeObjectTypeUInt: \
        case TSNodeObjectTypeDouble: \
            HANDLE_ARITH_OP(op, lhs, rhs, result) \
            break; \
        case TSNodeObjectTypePointer: \
            switch ((rhs).type.category) { \
                case TSNodeObjectTypeInt: \
                    result.type = (lhs).type; \
                    result.value.pointer = (void*)((uint8_t*)((lhs).value.pointer) op \
                        (((int64_t)(rhs).value.int64) * (lhs).array_element_type.size)); \
                    result.reference = (void*)&(result.value.pointer); \
                    result.array_element_type = (lhs).array_element_type; \
                    break; \
                case TSNodeObjectTypeUInt: \
                    result.type = (lhs).type; \
                    result.value.pointer = (void*)((uint8_t*)((lhs).value.pointer) op \
                        (((uint64_t)(rhs).value.uint64) * (lhs).array_element_type.size)); \
                    result.reference = (void*)&(result.value.pointer); \
                    result.array_element_type = (lhs).array_element_type; \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in pointer arithmetic: %s\n", (rhs).type.name); \
            } \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in binary op: %s\n", (lhs).type.name); \
    }

#define HANDLE_ARITH_NO_DOUBLE(op, lhs, rhs, result) \
    switch ((lhs).type.category) { \
        case TSNodeObjectTypeInt: \
            switch ((rhs).type.category) { \
                case TSNodeObjectTypeInt: \
                    HANDLE_ARITH_OP_INT(op, lhs, rhs, int64, result); \
                    break; \
                case TSNodeObjectTypeUInt: \
                    HANDLE_ARITH_OP_INT(op, lhs, rhs, uint64, result); \
                    break; \
                case TSNodeObjectTypeDouble: \
                    HANDLE_ARITH_OP_INT(op, lhs, rhs, double64, result); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to int: %s\n", (rhs).type.name); \
            } \
            break; \
        case TSNodeObjectTypeUInt: \
            switch ((rhs).type.category) { \
                case TSNodeObjectTypeInt: \
                    HANDLE_ARITH_OP_UINT(op, lhs, rhs, int64, result); \
                    break; \
                case TSNodeObjectTypeUInt: \
                    HANDLE_ARITH_OP_UINT(op, lhs, rhs, uint64, result); \
                    break; \
                case TSNodeObjectTypeDouble: \
                    HANDLE_ARITH_OP_UINT(op, lhs, rhs, double64, result); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to uint: %s\n", (rhs).type.name); \
            } \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in binary op: %s\n", (lhs).type.name); \
    }

/* Conditional */
#define HANDLE_COND_OP_INT(op, lhs, rhs, rhs_field, result) \
    switch ((lhs).type.size) { \
        case 1: \
            result.value.int64 = (int8_t)(lhs).value.int64 op (int8_t)(rhs).value.rhs_field; \
            break; \
        case 2: \
            result.value.int64 = (int16_t)(lhs).value.int64 op (int16_t)(rhs).value.rhs_field; \
            break; \
        case 4: \
            result.value.int64 = (int32_t)(lhs).value.int64 op (int32_t)(rhs).value.rhs_field; \
            break; \
        case 8: \
            result.value.int64 = (int64_t)(lhs).value.int64 op (int64_t)(rhs).value.rhs_field; \
            break; \
        default: \
            TS_PRINTF_ERROR("size of LHS of binary op %" PRIu32, (lhs).type.size); \
    }

#define HANDLE_COND_OP_UINT(op, lhs, rhs, rhs_field, result) \
    switch ((lhs).type.size) { \
        case 1: \
            result.value.int64 = (uint8_t)(lhs).value.uint64 op (uint8_t)(rhs).value.rhs_field; \
            break; \
        case 2: \
            result.value.int64 = (uint16_t)(lhs).value.uint64 op (uint16_t)(rhs).value.rhs_field; \
            break; \
        case 4: \
            result.value.int64 = (uint32_t)(lhs).value.uint64 op (uint32_t)(rhs).value.rhs_field; \
            break; \
        case 8: \
            result.value.int64 = (uint64_t)(lhs).value.uint64 op (uint64_t)(rhs).value.rhs_field; \
            break; \
        default: \
            TS_PRINTF_ERROR("size of LHS of binary op %" PRIu32, (lhs).type.size); \
    }

#define HANDLE_COND_OP_DOUBLE(op, lhs, rhs, rhs_field, result) \
    switch ((lhs).type.size) { \
        case 4: \
            result.value.int64 = (float)(lhs).value.double64 op (float)(rhs).value.rhs_field; \
            break; \
        case 8: \
            result.value.int64 = (double)(lhs).value.double64 op (double)(rhs).value.rhs_field; \
            break; \
        default: \
            TS_PRINTF_ERROR("size of LHS of binary op %" PRIu32, (lhs).type.size); \
    }

#define HANDLE_COND_OP(op, lhs, rhs, result) \
    switch ((lhs).type.category) { \
        case TSNodeObjectTypeInt: \
            switch ((rhs).type.category) { \
                case TSNodeObjectTypeInt: \
                    HANDLE_COND_OP_INT(op, lhs, rhs, int64, result); \
                    break; \
                case TSNodeObjectTypeUInt: \
                    HANDLE_COND_OP_INT(op, lhs, rhs, uint64, result); \
                    break; \
                case TSNodeObjectTypeDouble: \
                    HANDLE_COND_OP_INT(op, lhs, rhs, double64, result); \
                    break; \
                case TSNodeObjectTypePointer: \
                    HANDLE_COND_OP_INT(op, lhs, rhs, pointer, result); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to int: %s\n", (rhs).type.name); \
            } \
            break; \
        case TSNodeObjectTypeUInt: \
            switch ((rhs).type.category) { \
                case TSNodeObjectTypeInt: \
                    HANDLE_COND_OP_UINT(op, lhs, rhs, int64, result); \
                    break; \
                case TSNodeObjectTypeUInt: \
                    HANDLE_COND_OP_UINT(op, lhs, rhs, uint64, result); \
                    break; \
                case TSNodeObjectTypeDouble: \
                    HANDLE_COND_OP_UINT(op, lhs, rhs, double64, result); \
                    break; \
                case TSNodeObjectTypePointer: \
                    HANDLE_COND_OP_UINT(op, lhs, rhs, pointer, result); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to uint: %s\n", (rhs).type.name); \
            } \
            break; \
        case TSNodeObjectTypeDouble: \
            switch ((rhs).type.category) { \
                case TSNodeObjectTypeInt: \
                    HANDLE_COND_OP_DOUBLE(op, lhs, rhs, int64, result); \
                    break; \
                case TSNodeObjectTypeUInt: \
                    HANDLE_COND_OP_DOUBLE(op, lhs, rhs, uint64, result); \
                    break; \
                case TSNodeObjectTypeDouble: \
                    HANDLE_COND_OP_DOUBLE(op, lhs, rhs, double64, result); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to double: %s\n", (rhs).type.name); \
            } \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in binary op: %s\n", (lhs).type.name); \
    }

#define HANDLE_COND(op, lhs, rhs, result) \
    switch ((lhs).type.category) { \
        case TSNodeObjectTypeInt: \
        case TSNodeObjectTypeUInt: \
        case TSNodeObjectTypeDouble: \
            HANDLE_COND_OP(op, lhs, rhs, result) \
            break; \
        case TSNodeObjectTypePointer: \
            switch ((rhs).type.category) { \
                case TSNodeObjectTypePointer: \
                    result.value.int64 = (void*)(lhs).value.pointer op (rhs).value.pointer; \
                    break; \
                case TSNodeObjectTypeInt: \
                    result.value.int64 = (void*)(lhs).value.pointer op (void*)(intptr_t)(rhs).value.int64; \
                    break; \
                case TSNodeObjectTypeUInt: \
                    result.value.int64 = (void*)(lhs).value.pointer op (void*)(uintptr_t)(rhs).value.uint64; \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to pointer: %s\n", (rhs).type.name); \
            } \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in binary op: %s\n", (lhs).type.name); \
    }

/* Bit-wise*/
#define HANDLE_BITWISE(op, lhs, rhs, result) \
    switch ((lhs).type.category) { \
        case TSNodeObjectTypeInt: \
            switch ((rhs).type.category) { \
                case TSNodeObjectTypeInt: \
                    HANDLE_ARITH_OP_INT(op, lhs, rhs, int64, result); \
                    break; \
                case TSNodeObjectTypeUInt: \
                    HANDLE_ARITH_OP_INT(op, lhs, rhs, uint64, result); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to int: %s\n", (rhs).type.name); \
            } \
            break; \
        case TSNodeObjectTypeUInt: \
            switch ((rhs).type.category) { \
                case TSNodeObjectTypeInt: \
                    HANDLE_ARITH_OP_UINT(op, lhs, rhs, int64, result); \
                    break; \
                case TSNodeObjectTypeUInt: \
                    HANDLE_ARITH_OP_UINT(op, lhs, rhs, uint64, result); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to uint: %s\n", (rhs).type.name); \
            } \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in binary op: %s\n", (lhs).type.name); \
    }

uint64_t size_max(uint64_t a, uint64_t b) {
    return a>b?a:b;
}

TSNodeObject ts_interpreter_binary(TSNode node, uint64_t var_count, TSNodeObject* vars, TSTypeInfo* type_info_table) {
    char* op=ts_node_find_value(node);
    TSNodeObject obj1=ts_interpreter_simulate(ts_node_named_child(node,0),var_count,vars,type_info_table);
    TSNodeObject obj2;
    if (strcmp(op,"&&")!=0 && strcmp(op,"||")!=0) {
        obj2=ts_interpreter_simulate(ts_node_named_child(node,1),var_count,vars,type_info_table);
    }
    TSNodeObject result;
    result.name=ts_node_find_value(node);
    result.node=node;
    if (obj1.type.size > 0 && obj2.type.size > 0) {
        result.type=obj1.type;
    }
    else {
        result.type=obj2.type;
    }

    /* Arithmetic */
    if (strcmp(op,"+")==0) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        HANDLE_ARITH_ADD(+, obj1, obj2, result);
#pragma GCC diagnostic pop
    }
    else if (strcmp(op,"-")==0) {
        // Suppress warning about ptr - ptr. Some codes subtract between two ptrs to get the offset or length.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        HANDLE_ARITH_SUB(-, obj1, obj2, result);
#pragma GCC diagnostic pop
    }
    else if (strcmp(op,"*")==0) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        HANDLE_ARITH_OP(*, obj1, obj2, result);
#pragma GCC diagnostic pop
    }
    else if (strcmp(op,"/")==0) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        HANDLE_ARITH_OP(/, obj1, obj2, result);
#pragma GCC diagnostic pop
    }
    else if (strcmp(op,"%")==0) {
        HANDLE_ARITH_NO_DOUBLE(%, obj1, obj2, result);
    }

    /* Comparison */
    else if (strcmp(op,"==")==0) {
        result.type = ts_interpreter_get_type_info("int", sizeof(int), TSNodeObjectTypeInt);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        HANDLE_COND(==, obj1, obj2, result);
#pragma GCC diagnostic pop
    }
    else if (strcmp(op,"!=")==0) {
        result.type = ts_interpreter_get_type_info("int", sizeof(int), TSNodeObjectTypeInt);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        HANDLE_COND(!=, obj1, obj2, result);
#pragma GCC diagnostic pop
    }
    else if (strcmp(op,"<")==0) {
        result.type = ts_interpreter_get_type_info("int", sizeof(int), TSNodeObjectTypeInt);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        HANDLE_COND(<, obj1, obj2, result);
#pragma GCC diagnostic pop
    }
    else if (strcmp(op,">")==0) {
        result.type = ts_interpreter_get_type_info("int", sizeof(int), TSNodeObjectTypeInt);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        HANDLE_COND(>, obj1, obj2, result);
#pragma GCC diagnostic pop
    }
    else if (strcmp(op,"<=")==0) {
        result.type = ts_interpreter_get_type_info("int", sizeof(int), TSNodeObjectTypeInt);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        HANDLE_COND(<=, obj1, obj2, result);
#pragma GCC diagnostic pop
    }
    else if (strcmp(op,">=")==0) {
        result.type = ts_interpreter_get_type_info("int", sizeof(int), TSNodeObjectTypeInt);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        HANDLE_COND(>=, obj1, obj2, result);
#pragma GCC diagnostic pop
    }

    /* Relational */
    else if (strcmp(op,"&&")==0) {
        result.type = ts_interpreter_get_type_info("int", sizeof(int), TSNodeObjectTypeInt);
        switch (obj1.type.category) {
            case TSNodeObjectTypeInt:
                if (!obj1.value.int64) {
                    result.value.int64=0;
                    break;
                }
                obj2 = ts_interpreter_simulate(ts_node_named_child(node,1),var_count,vars,type_info_table);
                switch (obj2.type.category) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.int64 && obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.int64 && obj2.value.uint64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.int64 && obj2.value.pointer;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type in logical and: %d\n", obj2.type.category);
                }
                break;
            case TSNodeObjectTypeUInt:
                if (!obj1.value.uint64) {
                    result.value.int64=0;
                    break;
                }
                obj2 = ts_interpreter_simulate(ts_node_named_child(node,1),var_count,vars,type_info_table);
                switch (obj2.type.category) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.uint64 && obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.uint64 && obj2.value.uint64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.uint64 && obj2.value.pointer;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type in logical and: %d\n", obj2.type.category);
                }
                break;
            case TSNodeObjectTypePointer:
                if (!obj1.value.pointer) {
                    result.value.int64=0;
                    break;
                }
                obj2 = ts_interpreter_simulate(ts_node_named_child(node,1),var_count,vars,type_info_table);
                switch (obj2.type.category) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.pointer && obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.pointer && obj2.value.uint64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.pointer && obj2.value.pointer;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type in logical and: %d\n", obj2.type.category);
                }
                break;
            default:
                TS_PRINTF_ERROR("Unknown type in logical and: %d\n", obj1.type.category);
        }
    }
    else if (strcmp(op,"||")==0) {
        result.type = ts_interpreter_get_type_info("int", sizeof(int), TSNodeObjectTypeInt);
        switch (obj1.type.category) {
            case TSNodeObjectTypeInt:
                if (obj1.value.int64) {
                    result.value.int64=1;
                    break;
                }
                obj2 = ts_interpreter_simulate(ts_node_named_child(node,1),var_count,vars,type_info_table);
                switch (obj2.type.category) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.int64 || obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.int64 || obj2.value.uint64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.int64 || obj2.value.pointer;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type in logical or: %d\n", obj2.type.category);
                }
                break;
            case TSNodeObjectTypeUInt:
                if (obj1.value.uint64) {
                    result.value.int64=1;
                    break;
                }
                obj2 = ts_interpreter_simulate(ts_node_named_child(node,1),var_count,vars,type_info_table);
                switch (obj2.type.category) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.uint64 || obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.uint64 || obj2.value.uint64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.uint64 || obj2.value.pointer;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type in logical or: %d\n", obj2.type.category);
                }
                break;
            case TSNodeObjectTypePointer:
                if (obj1.value.pointer) {
                    result.value.int64=1;
                    break;
                }
                obj2 = ts_interpreter_simulate(ts_node_named_child(node,1),var_count,vars,type_info_table);
                switch (obj2.type.category) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.pointer || obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.pointer || obj2.value.uint64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.pointer || obj2.value.pointer;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type in logical or: %d\n", obj2.type.category);
                }
                break;
            default:
                TS_PRINTF_ERROR("Unknown type in logical or: %d\n", obj1.type.category);
        }
    }

    /* Bit-wise */
    else if (strcmp(op, "&") == 0) {
        HANDLE_BITWISE(&, obj1, obj2, result);
    }
    else if (strcmp(op, "|") == 0) {
        HANDLE_BITWISE(|, obj1, obj2, result);
    }
    else if (strcmp(op, "^") == 0) {
        HANDLE_BITWISE(^, obj1, obj2, result);
    }
    else if (strcmp(op, "<<") == 0) {
        HANDLE_BITWISE(<<, obj1, obj2, result);
    }
    else if (strcmp(op, ">>") == 0) {
        HANDLE_BITWISE(>>, obj1, obj2, result);
    }

    else {
        TS_PRINTF_ERROR("Unknown operator: %s\n", op);
    }

    switch (result.type.category) {
        case TSNodeObjectTypeInt:
            result.reference = malloc(sizeof(int64_t));
            *((int64_t*)result.reference) = result.value.int64;
            break;
        case TSNodeObjectTypeUInt:
            result.reference = malloc(sizeof(uint64_t));
            *((uint64_t*)result.reference) = result.value.uint64;
            break;
        case TSNodeObjectTypeDouble:
            result.reference = malloc(sizeof(double));
            *((double*)result.reference) = result.value.double64;
            break;
        case TSNodeObjectTypePointer:
            result.reference = malloc(sizeof(void*));
            *((void**)result.reference) = result.value.pointer;
            break;
        default:
            TS_PRINTF_ERROR("Unknown result type: %d\n", result.type.category);
    }

    return result;
}
