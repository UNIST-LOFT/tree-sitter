#include "tree_sitter/api.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

TSNodeObject ts_interpreter_unary(TSNode node, uint64_t var_count, TSNodeObject* vars) {
    char* op=ts_node_find_value(node);
    TSNodeObject obj=ts_interpreter_simulate(ts_node_named_child(node,0),var_count,vars);
    TSNodeObject result;
    result.name=ts_node_find_value(node);
    result.node=node;

    if (strcmp(op,"&")==0) {
        result.size=sizeof(void*);
        result.type=TSNodeObjectTypePointer;
        result.value.pointer=&obj;
    }
    else if (strcmp(op,"*")==0) {
        TS_PRINTF_ERROR("Dereference operation not supported");
    }
    else if (strcmp(op,"-")==0) {
        result.size=obj.size;
        switch (obj.type) {
            case TSNodeObjectTypeInt:
                result.type=TSNodeObjectTypeInt;
                result.value.int64=-obj.value.int64;
                break;
            case TSNodeObjectTypeUInt:
                result.type=TSNodeObjectTypeUInt;
                result.value.uint64=-obj.value.uint64;
                break;
            case TSNodeObjectTypeDouble:
                result.type=TSNodeObjectTypeDouble;
                result.value.double64=-obj.value.double64;
                break;
            default:
                TS_PRINTF_ERROR("Unknown type: %d\n", obj.type);
        }
    }
    else if (strcmp(op,"~")==0) {
        result.size=obj.size;
        switch (obj.type) {
            case TSNodeObjectTypeInt:
                result.type=TSNodeObjectTypeInt;
                result.value.int64=~obj.value.int64;
                break;
            case TSNodeObjectTypeUInt:
                result.type=TSNodeObjectTypeUInt;
                result.value.uint64=~obj.value.uint64;
                break;
            default:
                TS_PRINTF_ERROR("Unknown type: %d\n", obj.type);
        }
    }
    else if (strcmp(op,"!")==0) {
        result.size=sizeof(int);
        result.type=TSNodeObjectTypeInt;
        switch (obj.type) {
            case TSNodeObjectTypeInt:
                result.value.int64=!obj.value.int64;
                break;
            case TSNodeObjectTypeUInt:
                result.value.int64=!obj.value.uint64;
                break;
            case TSNodeObjectTypeDouble:
                result.value.int64=!obj.value.double64;
                break;
            default:
                TS_PRINTF_ERROR("Unknown type: %d\n", obj.type);
        }
    }
    else if (strcmp(op,"++")==0) {
        result.size=obj.size;
        switch (obj.type) {
            case TSNodeObjectTypeInt:
                result.type=TSNodeObjectTypeInt;
                result.value.int64=++obj.value.int64;
                result.reference=obj.reference; // Keep the reference to update the original variable
                *(int64_t*)result.reference = result.value.int64; // Update the original variable
                break;
            case TSNodeObjectTypeUInt:
                result.type=TSNodeObjectTypeUInt;
                result.value.uint64=++obj.value.uint64;
                result.reference=obj.reference; // Keep the reference to update the original variable
                *(uint64_t*)result.reference = result.value.uint64; // Update the original variable
                break;
            case TSNodeObjectTypePointer:
                result.type=TSNodeObjectTypePointer;
                result.value.pointer=(void*)((uint8_t*)obj.value.pointer+(obj.array_element_size));
                result.reference=obj.reference; // Keep the reference to update the original pointer
                *(void**)result.reference = result.value.pointer; // Update the original pointer value
                break;
            default:
                TS_PRINTF_ERROR("Unknown type: %d\n", obj.type);
        }
    }
    else if (strcmp(op,"--")==0) {
        result.size=obj.size;
        switch (obj.type) {
            case TSNodeObjectTypeInt:
                result.type=TSNodeObjectTypeInt;
                result.value.int64=--obj.value.int64;
                result.reference=obj.reference; // Keep the reference to update the original variable
                *(int64_t*)result.reference = result.value.int64; // Update the original variable
                break;
            case TSNodeObjectTypeUInt:
                result.type=TSNodeObjectTypeUInt;
                result.value.uint64=--obj.value.uint64;
                result.reference=obj.reference; // Keep the reference to update the original variable
                *(uint64_t*)result.reference = result.value.uint64; // Update the original variable
                break;
            case TSNodeObjectTypePointer:
                result.type=TSNodeObjectTypePointer;
                result.value.pointer=(void*)((uint8_t*)obj.value.pointer-(obj.array_element_size));
                result.reference=obj.reference; // Keep the reference to update the original pointer
                *(void**)result.reference = result.value.pointer; // Update the original pointer value
                break;
            default:
                TS_PRINTF_ERROR("Unknown type: %d\n", obj.type);
        }
    }
    else if (strcmp(op,"p++")==0) {
        result.size=obj.size;
        switch (obj.type) {
            case TSNodeObjectTypeInt:
                result.type=TSNodeObjectTypeInt;
                result.value.int64=++obj.value.int64;
                result.reference=obj.reference; // Keep the reference to update the original variable
                *(int64_t*)result.reference = result.value.int64; // Update the original variable
                break;
            case TSNodeObjectTypeUInt:
                result.type=TSNodeObjectTypeUInt;
                result.value.uint64=++obj.value.uint64;
                result.reference=obj.reference; // Keep the reference to update the original variable
                *(uint64_t*)result.reference = result.value.uint64; // Update the original variable
                break;
            case TSNodeObjectTypePointer:
                result.type=TSNodeObjectTypePointer;
                result.value.pointer=(void*)((uint8_t*)obj.value.pointer+(obj.array_element_size));
                result.reference=obj.reference; // Keep the reference to update the original pointer
                *(void**)result.reference = result.value.pointer; // Update the original pointer value
                break;
            default:
                TS_PRINTF_ERROR("Unknown type: %d\n", obj.type);
        }
    }
    else if (strcmp(op,"p--")==0) {
        result.size=obj.size;
        switch (obj.type) {
            case TSNodeObjectTypeInt:
                result.type=TSNodeObjectTypeInt;
                result.value.int64=--obj.value.int64;
                result.reference=obj.reference; // Keep the reference to update the original variable
                *(int64_t*)result.reference = result.value.int64; // Update the original variable
                break;
            case TSNodeObjectTypeUInt:
                result.type=TSNodeObjectTypeUInt;
                result.value.uint64=--obj.value.uint64;
                result.reference=obj.reference; // Keep the reference to update the original variable
                *(uint64_t*)result.reference = result.value.uint64; // Update the original variable
                break;
            case TSNodeObjectTypePointer:
                result.type=TSNodeObjectTypePointer;
                result.value.pointer=(void*)((uint8_t*)obj.value.pointer-(obj.array_element_size));
                result.reference=obj.reference; // Keep the reference to update the original pointer
                *(void**)result.reference = result.value.pointer; // Update the original pointer value
                break;
            default:
                TS_PRINTF_ERROR("Unknown type: %d\n", obj.type);
        }
    }
    else {
        TS_PRINTF_ERROR("Unknown unary operator: %s\n", op);
    }

    return result;
}
