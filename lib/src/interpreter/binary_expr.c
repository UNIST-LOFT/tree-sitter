#include "tree_sitter/api.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

/* Arithmetic */
#define HANDLE_ARITH_OP_INT(op, lhs, rhs, rhs_field, result) \
    switch ((lhs).size) { \
        case 1: \
            result.type = TSNodeObjectTypeInt; \
            result.value.int64 = (int8_t)(lhs).value.int64 op (int8_t)(rhs).value.rhs_field; \
            break; \
        case 2: \
            result.type = TSNodeObjectTypeInt; \
            result.value.int64 = (int16_t)(lhs).value.int64 op (int16_t)(rhs).value.rhs_field; \
            break; \
        case 4: \
            result.type = TSNodeObjectTypeInt; \
            result.value.int64 = (int32_t)(lhs).value.int64 op (int32_t)(rhs).value.rhs_field; \
            break; \
        case 8: \
            result.type = TSNodeObjectTypeInt; \
            result.value.int64 = (int64_t)(lhs).value.int64 op (int64_t)(rhs).value.rhs_field; \
            break; \
        default: \
            TS_PRINTF_ERROR("size of LHS of binary op %" PRIu64, (lhs).size); \
    }

#define HANDLE_ARITH_OP_UINT(op, lhs, rhs, rhs_field, result) \
    switch ((lhs).size) { \
        case 1: \
            result.type = TSNodeObjectTypeUInt; \
            result.value.uint64 = (uint8_t)(lhs).value.uint64 op (uint8_t)(rhs).value.rhs_field; \
            break; \
        case 2: \
            result.type = TSNodeObjectTypeUInt; \
            result.value.uint64 = (uint16_t)(lhs).value.uint64 op (uint16_t)(rhs).value.rhs_field; \
            break; \
        case 4: \
            result.type = TSNodeObjectTypeUInt; \
            result.value.uint64 = (uint32_t)(lhs).value.uint64 op (uint32_t)(rhs).value.rhs_field; \
            break; \
        case 8: \
            result.type = TSNodeObjectTypeUInt; \
            result.value.uint64 = (uint64_t)(lhs).value.uint64 op (uint64_t)(rhs).value.rhs_field; \
            break; \
        default: \
            TS_PRINTF_ERROR("size of LHS of binary op %" PRIu64, (lhs).size); \
    }

#define HANDLE_ARITH_OP_DOUBLE(op, lhs, rhs, rhs_field, result) \
    switch ((lhs).size) { \
        case 4: \
            result.type = TSNodeObjectTypeDouble; \
            result.value.double64 = (float)(lhs).value.double64 op (float)(rhs).value.rhs_field; \
            break; \
        case 8: \
            result.type = TSNodeObjectTypeDouble; \
            result.value.double64 = (double)(lhs).value.double64 op (double)(rhs).value.rhs_field; \
            break; \
        default: \
            TS_PRINTF_ERROR("size of LHS of binary op %" PRIu64, (lhs).size); \
    }

#define HANDLE_ARITH_OP(op, lhs, rhs, result) \
    switch ((lhs).type) { \
        case TSNodeObjectTypeInt: \
            switch ((rhs).type) { \
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
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to int: %d\n", (rhs).type); \
            } \
            break; \
        case TSNodeObjectTypeUInt: \
            switch ((rhs).type) { \
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
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to uint: %d\n", (rhs).type); \
            } \
            break; \
        case TSNodeObjectTypeDouble: \
            switch ((rhs).type) { \
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
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to double: %d\n", (rhs).type); \
            } \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in binary op: %d\n", (lhs).type); \
    }

#define HANDLE_ARITH_SUB(op, lhs, rhs, result) \
    switch ((lhs).type) { \
        case TSNodeObjectTypeInt: \
        case TSNodeObjectTypeUInt: \
        case TSNodeObjectTypeDouble: \
            HANDLE_ARITH_OP(op, lhs, rhs, result) \
            break; \
        case TSNodeObjectTypePointer: \
            switch ((rhs).type) { \
                case TSNodeObjectTypeInt: \
                    result.type = TSNodeObjectTypePointer; \
                    result.size = (lhs).size; \
                    result.value.pointer = (uint8_t*)(lhs).value.pointer op ((int64_t)(rhs).value.int64) * (rhs).size; \
                    break; \
                case TSNodeObjectTypeUInt: \
                    result.type = TSNodeObjectTypePointer; \
                    result.size = (lhs).size; \
                    result.value.pointer = (uint8_t*)(lhs).value.pointer op ((uint64_t)(rhs).value.uint64) * (rhs).size; \
                    break; \
                case TSNodeObjectTypePointer: \
                    result.type = TSNodeObjectTypeInt; \
                    result.size = (lhs).size; \
                    result.value.int64 = (void*)(lhs).value.pointer op (rhs).value.pointer; \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in pointer arithmetic: %d\n", (rhs).type); \
            } \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in binary op: %d\n", (lhs).type); \
    }

#define HANDLE_ARITH_ADD(op, lhs, rhs, result) \
    switch ((lhs).type) { \
        case TSNodeObjectTypeInt: \
        case TSNodeObjectTypeUInt: \
        case TSNodeObjectTypeDouble: \
            HANDLE_ARITH_OP(op, lhs, rhs, result) \
            break; \
        case TSNodeObjectTypePointer: \
            switch ((rhs).type) { \
                case TSNodeObjectTypeInt: \
                    result.type = TSNodeObjectTypePointer; \
                    result.size = (lhs).size; \
                    result.value.pointer = (uint8_t*)(lhs).value.pointer op ((int64_t)(rhs).value.int64) * (rhs).size; \
                    break; \
                case TSNodeObjectTypeUInt: \
                    result.type = TSNodeObjectTypePointer; \
                    result.size = (lhs).size; \
                    result.value.pointer = (uint8_t*)(lhs).value.pointer op ((uint64_t)(rhs).value.uint64) * (rhs).size; \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in pointer arithmetic: %d\n", (rhs).type); \
            } \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in binary op: %d\n", (lhs).type); \
    }

#define HANDLE_ARITH_NO_DOUBLE(op, lhs, rhs, result) \
    switch ((lhs).type) { \
        case TSNodeObjectTypeInt: \
            switch ((rhs).type) { \
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
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to int: %d\n", (rhs).type); \
            } \
            break; \
        case TSNodeObjectTypeUInt: \
            switch ((rhs).type) { \
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
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to uint: %d\n", (rhs).type); \
            } \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in binary op: %d\n", (lhs).type); \
    }

/* Conditional */
#define HANDLE_COND_OP_INT(op, lhs, rhs, rhs_field, result) \
    switch ((lhs).size) { \
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
            TS_PRINTF_ERROR("size of LHS of binary op %" PRIu64, (lhs).size); \
    }

#define HANDLE_COND_OP_UINT(op, lhs, rhs, rhs_field, result) \
    switch ((lhs).size) { \
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
            TS_PRINTF_ERROR("size of LHS of binary op %" PRIu64, (lhs).size); \
    }

#define HANDLE_COND_OP_DOUBLE(op, lhs, rhs, rhs_field, result) \
    switch ((lhs).size) { \
        case 4: \
            result.value.int64 = (float)(lhs).value.double64 op (float)(rhs).value.rhs_field; \
            break; \
        case 8: \
            result.value.int64 = (double)(lhs).value.double64 op (double)(rhs).value.rhs_field; \
            break; \
        default: \
            TS_PRINTF_ERROR("size of LHS of binary op %" PRIu64, (lhs).size); \
    }

#define HANDLE_COND_OP(op, lhs, rhs, result) \
    switch ((lhs).type) { \
        case TSNodeObjectTypeInt: \
            switch ((rhs).type) { \
                case TSNodeObjectTypeInt: \
                    HANDLE_COND_OP_INT(op, lhs, rhs, int64, result); \
                    break; \
                case TSNodeObjectTypeUInt: \
                    HANDLE_COND_OP_INT(op, lhs, rhs, uint64, result); \
                    break; \
                case TSNodeObjectTypeDouble: \
                    HANDLE_COND_OP_INT(op, lhs, rhs, double64, result); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to int: %d\n", (rhs).type); \
            } \
            break; \
        case TSNodeObjectTypeUInt: \
            switch ((rhs).type) { \
                case TSNodeObjectTypeInt: \
                    HANDLE_COND_OP_UINT(op, lhs, rhs, int64, result); \
                    break; \
                case TSNodeObjectTypeUInt: \
                    HANDLE_COND_OP_UINT(op, lhs, rhs, uint64, result); \
                    break; \
                case TSNodeObjectTypeDouble: \
                    HANDLE_COND_OP_UINT(op, lhs, rhs, double64, result); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to uint: %d\n", (rhs).type); \
            } \
            break; \
        case TSNodeObjectTypeDouble: \
            switch ((rhs).type) { \
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
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to double: %d\n", (rhs).type); \
            } \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in binary op: %d\n", (lhs).type); \
    }

#define HANDLE_COND(op, lhs, rhs, result) \
    switch ((lhs).type) { \
        case TSNodeObjectTypeInt: \
        case TSNodeObjectTypeUInt: \
        case TSNodeObjectTypeDouble: \
            HANDLE_COND_OP(op, lhs, rhs, result) \
            break; \
        case TSNodeObjectTypePointer: \
            switch ((rhs).type) { \
                case TSNodeObjectTypePointer: \
                    result.value.int64 = (void*)(lhs).value.pointer op (rhs).value.pointer; \
                    break; \
                case TSNodeObjectTypeInt: \
                case TSNodeObjectTypeUInt: \
                    result.value.int64 = (void*)(lhs).value.pointer op NULL; \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to pointer: %d\n", (rhs).type); \
            } \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in binary op: %d\n", (lhs).type); \
    }

#define HANDLE_COND_COMPARE(op, lhs, rhs, result) \
    switch ((lhs).type) { \
        case TSNodeObjectTypeInt: \
        case TSNodeObjectTypeUInt: \
        case TSNodeObjectTypeDouble: \
            HANDLE_COND_OP(op, lhs, rhs, result) \
            break; \
        case TSNodeObjectTypePointer: \
            switch ((rhs).type) { \
                case TSNodeObjectTypePointer: \
                    result.value.int64 = (void*)(lhs).value.pointer op (rhs).value.pointer; \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to pointer: %d\n", (rhs).type); \
            } \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in binary op: %d\n", (lhs).type); \
    }

/* Bit-wise*/
#define HANDLE_BITWISE(op, lhs, rhs, result) \
    switch ((lhs).type) { \
        case TSNodeObjectTypeInt: \
            switch ((rhs).type) { \
                case TSNodeObjectTypeInt: \
                    HANDLE_ARITH_OP_INT(op, lhs, rhs, int64, result); \
                    break; \
                case TSNodeObjectTypeUInt: \
                    HANDLE_ARITH_OP_INT(op, lhs, rhs, uint64, result); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to int: %d\n", (rhs).type); \
            } \
            break; \
        case TSNodeObjectTypeUInt: \
            switch ((rhs).type) { \
                case TSNodeObjectTypeInt: \
                    HANDLE_ARITH_OP_UINT(op, lhs, rhs, int64, result); \
                    break; \
                case TSNodeObjectTypeUInt: \
                    HANDLE_ARITH_OP_UINT(op, lhs, rhs, uint64, result); \
                    break; \
                default: \
                    TS_PRINTF_ERROR("Unsupported RHS type in binary op to uint: %d\n", (rhs).type); \
            } \
            break; \
        default: \
            TS_PRINTF_ERROR("Unsupported type in binary op: %d\n", (lhs).type); \
    }

uint64_t size_max(uint64_t a, uint64_t b) {
    return a>b?a:b;
}

TSNodeObject ts_interpreter_binary(TSNode node, uint64_t var_count, TSNodeObject* vars) {
    char* op=ts_node_find_value(node);
    TSNodeObject obj1=ts_interpreter_simulate(ts_node_named_child(node,0),var_count,vars);
    TSNodeObject obj2=ts_interpreter_simulate(ts_node_named_child(node,1),var_count,vars);
    TSNodeObject result;
    result.name=ts_node_find_value(node);
    result.node=node;
    result.size=size_max(obj1.size,obj2.size);

    /* Arithmetic */
    if (strcmp(op,"+")==0) {
        HANDLE_ARITH_ADD(+, obj1, obj2, result);
    }
    else if (strcmp(op,"-")==0) {
        // Suppress warning about ptr - ptr. Some codes subtract between two ptrs to get the offset or length.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"
        HANDLE_ARITH_SUB(-, obj1, obj2, result);
#pragma GCC diagnostic pop
    }
    else if (strcmp(op,"*")==0) {
        HANDLE_ARITH_OP(*, obj1, obj2, result);
    }
    else if (strcmp(op,"/")==0) {
        HANDLE_ARITH_OP(/, obj1, obj2, result);
    }
    else if (strcmp(op,"%%")==0) {
        HANDLE_ARITH_NO_DOUBLE(%, obj1, obj2, result);
    }

    /* Comparison */
    else if (strcmp(op,"==")==0) {
        result.size=sizeof(int);
        result.type=TSNodeObjectTypeInt;
        HANDLE_COND(==, obj1, obj2, result);
    }
    else if (strcmp(op,"!=")==0) {
        result.size=sizeof(int);
        result.type=TSNodeObjectTypeInt;
        HANDLE_COND(!=, obj1, obj2, result);
    }
    else if (strcmp(op,"<")==0) {
        result.size=sizeof(int);
        result.type=TSNodeObjectTypeInt;
        HANDLE_COND_COMPARE(<, obj1, obj2, result);
    }
    else if (strcmp(op,">")==0) {
        result.size=sizeof(int);
        result.type=TSNodeObjectTypeInt;
        HANDLE_COND_COMPARE(>, obj1, obj2, result);
    }
    else if (strcmp(op,"<=")==0) {
        result.size=sizeof(int);
        result.type=TSNodeObjectTypeInt;
        HANDLE_COND_COMPARE(<=, obj1, obj2, result);
    }
    else if (strcmp(op,">=")==0) {
        result.size=sizeof(int);
        result.type=TSNodeObjectTypeInt;
        HANDLE_COND_COMPARE(>=, obj1, obj2, result);
    }

    /* Relational */
    else if (strcmp(op,"&&")==0) {
        result.size=sizeof(int);
        result.type=TSNodeObjectTypeInt;
        switch (obj1.type) {
            case TSNodeObjectTypeInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.int64 && obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.int64 && obj2.value.uint64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type in logical and: %d\n", obj2.type);
                }
                break;
            case TSNodeObjectTypeUInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.uint64 && obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.uint64 && obj2.value.uint64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type in logical and: %d\n", obj2.type);
                }
                break;
            default:
                TS_PRINTF_ERROR("Unknown type in logical and: %d\n", obj1.type);
        }
    }
    else if (strcmp(op,"||")==0) {
        result.size=sizeof(int);
        result.type=TSNodeObjectTypeInt;
        switch (obj1.type) {
            case TSNodeObjectTypeInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.int64 || obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.int64 || obj2.value.uint64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type in logical or: %d\n", obj2.type);
                }
                break;
            case TSNodeObjectTypeUInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.uint64 || obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.uint64 || obj2.value.uint64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type in logical or: %d\n", obj2.type);
                }
                break;
            default:
                TS_PRINTF_ERROR("Unknown type in logical or: %d\n", obj1.type);
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

    return result;
}
