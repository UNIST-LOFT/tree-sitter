#include "tree_sitter/api.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

TSNodeObject ts_interpreter_unary(TSNode node, uint64_t var_count, TSNodeObject* vars, TSTypeInfo* type_info_table) {
    char* op=ts_node_find_value(node);
    TSNodeObject obj=ts_interpreter_simulate(ts_node_named_child(node,0),var_count,vars,type_info_table);
    TSNodeObject result;
    result.name=ts_node_find_value(node);
    result.node=node;

    if (strcmp(op,"&")==0) {
        result.array_element_type = ts_interpreter_get_type_info(obj.type.name, obj.type.size, obj.type.category);
        result.type = ts_interpreter_get_pointer_type_info(result.array_element_type);
        result.value.pointer = (void*)obj.reference; // reference is already &ed
        result.reference = &obj.reference; // ref of reference
    }
    else if (strcmp(op,"*")==0) {
        result.type=obj.array_element_type;
        result.reference=obj.value.pointer; // reference is already *ed
        switch(obj.array_element_type.category) {
            case TSNodeObjectTypeInt:
                result.value.int64=*((int64_t*)obj.value.pointer);
                break;
            case TSNodeObjectTypeUInt:
                result.value.uint64=*((uint64_t*)obj.value.pointer);
                break;
            case TSNodeObjectTypeDouble:
                result.value.double64=*((double*)obj.value.pointer);
                break;
            case TSNodeObjectTypePointer:
                result.value.pointer=*((void**)obj.value.pointer);
                break;
            default:
                // Just set reference only for struct and general types
                break;
        }
    }
    else if (strcmp(op,"+")==0) {
        result.type = obj.type;
        switch (obj.type.category) {
            case TSNodeObjectTypeInt:
                result.value.int64=+obj.value.int64;
                break;
            case TSNodeObjectTypeUInt:
                result.value.uint64=+obj.value.uint64;
                break;
            case TSNodeObjectTypeDouble:
                result.value.double64=+obj.value.double64;
                break;
            default:
                TS_PRINTF_ERROR("Unknown type: %d\n", obj.type.category);
        }
    }
    else if (strcmp(op,"-")==0) {
        result.type = obj.type;
        switch (obj.type.category) {
            case TSNodeObjectTypeInt:
                result.value.int64=-obj.value.int64;
                break;
            case TSNodeObjectTypeUInt:
                result.value.uint64=-obj.value.uint64;
                break;
            case TSNodeObjectTypeDouble:
                result.value.double64=-obj.value.double64;
                break;
            default:
                TS_PRINTF_ERROR("Unknown type: %d\n", obj.type.category);
        }
    }
    else if (strcmp(op,"~")==0) {
        result.type = obj.type;
        switch (obj.type.category) {
            case TSNodeObjectTypeInt:
                result.value.int64=~obj.value.int64;
                break;
            case TSNodeObjectTypeUInt:
                result.value.uint64=~obj.value.uint64;
                break;
            default:
                TS_PRINTF_ERROR("Unknown type: %d\n", obj.type.category);
        }
    }
    else if (strcmp(op,"!")==0) {
        result.type = ts_interpreter_get_type_info("int", sizeof(int), TSNodeObjectTypeInt);
        switch (obj.type.category) {
            case TSNodeObjectTypeInt:
                result.value.int64=!obj.value.int64;
                break;
            case TSNodeObjectTypeUInt:
                result.value.int64=!obj.value.uint64;
                break;
            case TSNodeObjectTypeDouble:
                result.value.int64=!obj.value.double64;
                break;
            case TSNodeObjectTypePointer:
                result.value.int64=!obj.value.pointer;
                break;
            default:
                TS_PRINTF_ERROR("Unknown type: %d\n", obj.type.category);
        }
    }
    else if (strcmp(op,"++")==0) {
        result.type=obj.type;
        switch (obj.type.category) {
            case TSNodeObjectTypeInt:
                result.value.int64=++obj.value.int64;
                result.reference=obj.reference; // Keep the reference to update the original variable
                *(int64_t*)result.reference = result.value.int64; // Update the original variable
                break;
            case TSNodeObjectTypeUInt:
                result.value.uint64=++obj.value.uint64;
                result.reference=obj.reference; // Keep the reference to update the original variable
                *(uint64_t*)result.reference = result.value.uint64; // Update the original variable
                break;
            case TSNodeObjectTypePointer:
                result.value.pointer=(void*)((uint8_t*)obj.value.pointer+(obj.array_element_type.size));
                result.reference=obj.reference; // Keep the reference to update the original pointer
                *(void**)result.reference = result.value.pointer; // Update the original pointer value
                result.array_element_type = obj.array_element_type;
                break;
            default:
                TS_PRINTF_ERROR("Unknown type: %d\n", obj.type.category);
        }
    }
    else if (strcmp(op,"--")==0) {
        result.type=obj.type;
        switch (obj.type.category) {
            case TSNodeObjectTypeInt:
                result.value.int64=--obj.value.int64;
                result.reference=obj.reference; // Keep the reference to update the original variable
                *(int64_t*)result.reference = result.value.int64; // Update the original variable
                break;
            case TSNodeObjectTypeUInt:
                result.value.uint64=--obj.value.uint64;
                result.reference=obj.reference; // Keep the reference to update the original variable
                *(uint64_t*)result.reference = result.value.uint64; // Update the original variable
                break;
            case TSNodeObjectTypePointer:
                result.value.pointer=(void*)((uint8_t*)obj.value.pointer-(obj.array_element_type.size));
                result.reference=obj.reference; // Keep the reference to update the original pointer
                *(void**)result.reference = result.value.pointer; // Update the original pointer value
                result.array_element_type = obj.array_element_type;
                break;
            default:
                TS_PRINTF_ERROR("Unknown type: %d\n", obj.type.category);
        }
    }
    else if (strcmp(op,"p++")==0) {
        result.type=obj.type;
        switch (obj.type.category) {
                result.value.int64=++obj.value.int64;
                result.reference=obj.reference; // Keep the reference to update the original variable
                *(int64_t*)result.reference = result.value.int64; // Update the original variable
                break;
            case TSNodeObjectTypeUInt:
                result.value.uint64=++obj.value.uint64;
                result.reference=obj.reference; // Keep the reference to update the original variable
                *(uint64_t*)result.reference = result.value.uint64; // Update the original variable
                break;
            case TSNodeObjectTypePointer:
                result.value.pointer=(void*)((uint8_t*)obj.value.pointer+(obj.array_element_type.size));
                result.reference=obj.reference; // Keep the reference to update the original pointer
                *(void**)result.reference = result.value.pointer; // Update the original pointer value
                result.array_element_type = obj.array_element_type;
                break;
            default:
                TS_PRINTF_ERROR("Unknown type: %d\n", obj.type.category);
        }
    }
    else if (strcmp(op,"p--")==0) {
        result.type=obj.type;
        switch (obj.type.category) {
            case TSNodeObjectTypeInt:
                result.value.int64=--obj.value.int64;
                result.reference=obj.reference; // Keep the reference to update the original variable
                *(int64_t*)result.reference = result.value.int64; // Update the original variable
                break;
            case TSNodeObjectTypeUInt:
                result.value.uint64=--obj.value.uint64;
                result.reference=obj.reference; // Keep the reference to update the original variable
                *(uint64_t*)result.reference = result.value.uint64; // Update the original variable
                break;
            case TSNodeObjectTypePointer:
                result.value.pointer=(void*)((uint8_t*)obj.value.pointer-(obj.array_element_type.size));
                result.reference=obj.reference; // Keep the reference to update the original pointer
                *(void**)result.reference = result.value.pointer; // Update the original pointer value
                result.array_element_type = obj.array_element_type;
                break;
            default:
                TS_PRINTF_ERROR("Unknown type: %d\n", obj.type.category);
        }
    }
    else {
        TS_PRINTF_ERROR("Unknown unary operator: %s\n", op);
    }

    return result;
}
