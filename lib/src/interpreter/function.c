#include "tree_sitter/api.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include <ffi.h>

TSNodeObject ts_interpreter_function(TSNode node, uint64_t var_count, TSNodeObject* vars, TSTypeInfo* type_info_table) {
    if (strcmp(ts_node_type(node), "call_expression") != 0) {
        TS_PRINTF_ERROR("Node is not a call_expression: %s\n", ts_node_type(node));
    }
    TSNodeObject obj;
    const char* func_name = ts_node_find_value(ts_node_named_child(node,0));
    obj.name = malloc(strlen(func_name)+1);
    strcpy(obj.name, func_name); // Copy function name
    
    TSNodeObject found;
    int exists = 0;
    for (size_t i = 0; i < var_count; i++) {
        if (strcmp(vars[i].name, obj.name) == 0) {
            found = vars[i];
            exists = 1;
            break;
        }
    }
    if (!exists)
        TS_PRINTF_ERROR("Function %s not found in variables\n", obj.name);

    TSNodeObject args[10]; // Max 10 arguments
    TSNode arg_list = ts_node_named_child(node,1);
    uint32_t arg_count = ts_node_named_child_count(arg_list);
    for (size_t i = 0; i < arg_count; i++) {
        TSNode arg_node = ts_node_named_child(arg_list, i);
        args[i] = ts_interpreter_simulate(arg_node, var_count, vars, type_info_table);
    }

    switch (found.type.category) {
        case TSNodeObjectTypeFunctionVoid:
            // Void type will not be used
            obj.type = ts_interpreter_get_type_info("void", 0, TSNodeObjectTypeInt);
            obj.value.int64 = 0;
            break;
        case TSNodeObjectTypeFunctionInt:
            obj.type = ts_interpreter_get_type_info("int", sizeof(int32_t), TSNodeObjectTypeInt);
            break;
        case TSNodeObjectTypeFunctionUInt:
            obj.type = ts_interpreter_get_type_info("unsigned int", sizeof(uint32_t), TSNodeObjectTypeUInt);
            break;
        case TSNodeObjectTypeFunctionPointer:
            obj.type = ts_interpreter_get_type_info("void*", sizeof(void*), TSNodeObjectTypePointer);
            break;
        default:
            TS_PRINTF_ERROR("Unknown function return type: %d\n", found.type.category);
    }

    // Prepare ffi
    ffi_type* arg_types[10];
    void* arg_values[10];
    for (size_t i = 0; i < arg_count; i++) {
        switch (args[i].type.category) {
            case TSNodeObjectTypeInt:
                if (args[i].array_element_type.size == 1 || args[i].array_element_type.size == 2 ||
                        args[i].array_element_type.size == 4 || args[i].array_element_type.size == 8) {
                    switch (args[i].array_element_type.size) {
                        case 1:
                            arg_types[i] = &ffi_type_sint8;
                            break;
                        case 2:
                            arg_types[i] = &ffi_type_sint16;
                            break;
                        case 4:
                            arg_types[i] = &ffi_type_sint32;
                            break;
                        case 8:
                            arg_types[i] = &ffi_type_sint64;
                            break;
                        default:
                            TS_PRINTF_ERROR("Unsupported int array element size: %" PRIu32 "\n", args[i].array_element_type.size);
                    }
                }
                else {
                    if (args[i].type.size == 1)
                        arg_types[i] = &ffi_type_sint8;
                    else if (args[i].type.size == 2)
                        arg_types[i] = &ffi_type_sint16;
                    else if (args[i].type.size == 4)
                        arg_types[i] = &ffi_type_sint32;
                    else if (args[i].type.size == 8)
                        arg_types[i] = &ffi_type_sint64;
                    else
                        TS_PRINTF_ERROR("Unsupported int size: %" PRIu32 "\n", args[i].type.size);
                }
                arg_values[i] = &args[i].value.int64;
                break;
            case TSNodeObjectTypeUInt:
                if (args[i].array_element_type.size == 1 || args[i].array_element_type.size == 2 ||
                        args[i].array_element_type.size == 4 || args[i].array_element_type.size == 8) {
                    switch (args[i].array_element_type.size) {
                        case 1:
                            arg_types[i] = &ffi_type_uint8;
                            break;
                        case 2:
                            arg_types[i] = &ffi_type_uint16;
                            break;
                        case 4:
                            arg_types[i] = &ffi_type_uint32;
                            break;
                        case 8:
                            arg_types[i] = &ffi_type_uint64;
                            break;
                        default:
                            TS_PRINTF_ERROR("Unsupported uint array element size: %" PRIu32 "\n", args[i].array_element_type.size);
                    }
                }
                else {
                    if (args[i].type.size == 1)
                        arg_types[i] = &ffi_type_uint8;
                    else if (args[i].type.size == 2)
                        arg_types[i] = &ffi_type_uint16;
                    else if (args[i].type.size == 4)
                        arg_types[i] = &ffi_type_uint32;
                    else if (args[i].type.size == 8)
                        arg_types[i] = &ffi_type_uint64;
                    else
                        TS_PRINTF_ERROR("Unsupported uint size: %" PRIu32 "\n", args[i].type.size);
                }
                arg_values[i] = &args[i].value.uint64;
                break;
            case TSNodeObjectTypeDouble:
                if (args[i].type.size == 4)
                    arg_types[i] = &ffi_type_float;
                else if (args[i].type.size == 8)
                    arg_types[i] = &ffi_type_double;
                else
                    TS_PRINTF_ERROR("Unsupported double size: %" PRIu32 "\n", args[i].type.size);
                arg_values[i] = &args[i].value.double64;
                break;
            case TSNodeObjectTypePointer:
                arg_types[i] = &ffi_type_pointer;
                arg_values[i] = &args[i].value.pointer;
                break;
            case TSNodeObjectTypeString:
                arg_types[i] = &ffi_type_pointer;
                arg_values[i] = &args[i].name;
                break;
            default:
                TS_PRINTF_ERROR("Unsupported argument type: %s\n", args[i].name);
        }
    }

    ffi_cif cif;
    ffi_type* ret_type;
    switch (obj.type.category) {
        case TSNodeObjectTypeInt:
            ret_type = &ffi_type_sint64;
            break;
        case TSNodeObjectTypeUInt:
            ret_type = &ffi_type_uint64;
            break;
        case TSNodeObjectTypePointer:
            ret_type = &ffi_type_pointer;
            break;
        default:
            ret_type = &ffi_type_void;
            break;
    }
    ffi_status status = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, arg_count, ret_type, arg_types);
    if (status != FFI_OK) {
        TS_PRINTF_ERROR("ffi_prep_cif failed: %d\n", status);
    }

    // Call the function
    switch (obj.type.category) {
        case TSNodeObjectTypeInt: {
            int64_t ret;
            ffi_call(&cif, FFI_FN(found.value.int_func), &ret, arg_values);
            obj.value.int64 = ret;
            obj.reference = malloc(sizeof(int64_t));
            *((int64_t*)obj.reference) = ret;
            break;
        }
        case TSNodeObjectTypeUInt: {
            uint64_t ret;
            ffi_call(&cif, FFI_FN(found.value.uint_func), &ret, arg_values);
            obj.value.uint64 = ret;
            obj.reference = malloc(sizeof(uint64_t));
            *((uint64_t*)obj.reference) = ret;
            break;
        }
        case TSNodeObjectTypePointer: {
            void* ret;
            ffi_call(&cif, FFI_FN(found.value.pointer_func), &ret, arg_values);
            obj.value.pointer = ret;
            obj.array_element_type = found.array_element_type;
            obj.reference = malloc(sizeof(void*));
            *((void**)obj.reference) = ret;
            break;
        }
        case TSNodeObjectTypeFunctionVoid: {
            ffi_call(&cif, FFI_FN(found.value.void_func), NULL, arg_values);
            obj.value.int64 = 0; // Void return
            break;
        }
        default:
            TS_PRINTF_ERROR("Unsupported function return type in call: %d\n", obj.type.category);
    }

    return obj;
}
