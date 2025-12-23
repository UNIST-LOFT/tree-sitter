#include "tree_sitter/api.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

TSNodeObject ts_interpreter_function(TSNode node, uint64_t var_count, TSNodeObject* vars) {
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
        args[i] = ts_interpreter_simulate(arg_node, var_count, vars);
    }

    switch (found.type) {
        case TSNodeObjectTypeFunctionVoid:
            obj.type = TSNodeObjectTypeInt; // TODO: handle void return, now return 0
            obj.size = sizeof(int32_t);
            obj.value.int64 = 0;
            break;
        case TSNodeObjectTypeFunctionInt:
            obj.type = TSNodeObjectTypeInt;
            obj.size = sizeof(int32_t);
            break;
        case TSNodeObjectTypeFunctionUInt:
            obj.type = TSNodeObjectTypeUInt;
            obj.size = sizeof(uint32_t);
            break;
        case TSNodeObjectTypeFunctionPointer:
            obj.type = TSNodeObjectTypePointer;
            obj.size = sizeof(void*);
            break;
        default:
            TS_PRINTF_ERROR("Unknown function return type: %d\n", found.type);
    }

    // Call the function
    switch (found.type) {
        case TSNodeObjectTypeFunctionVoid: {
            switch (arg_count) {
                case 0:
                    found.value.void_func();
                    break;
                case 1:
                    switch (args[0].type) {
                        case TSNodeObjectTypeInt:
                        case TSNodeObjectTypeChar:
                            found.value.void_func(args[0].value.int64);
                            break;
                        case TSNodeObjectTypeUInt:
                            found.value.void_func(args[0].value.uint64);
                            break;
                        case TSNodeObjectTypeDouble:
                            found.value.void_func(args[0].value.double64);
                            break;
                        case TSNodeObjectTypePointer:
                            found.value.void_func(args[0].value.pointer);
                            break;
                        case TSNodeObjectTypeString:
                            found.value.void_func(args[0].name);
                            break;
                        default:
                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[0].name);
                    }
                    break;
                case 2:
                    switch (args[0].type) {
                        case TSNodeObjectTypeInt:
                        case TSNodeObjectTypeChar:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    found.value.void_func(args[0].value.int64, args[1].value.int64);
                                    break;
                                case TSNodeObjectTypeUInt:
                                    found.value.void_func(args[0].value.int64, args[1].value.uint64);
                                    break;
                                case TSNodeObjectTypeDouble:
                                    found.value.void_func(args[0].value.int64, args[1].value.double64);
                                    break;
                                case TSNodeObjectTypePointer:
                                    found.value.void_func(args[0].value.int64, args[1].value.pointer);
                                    break;
                                case TSNodeObjectTypeString:
                                    found.value.void_func(args[0].value.int64, args[1].name);
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeUInt:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    found.value.void_func(args[0].value.uint64, args[1].value.int64);
                                    break;
                                case TSNodeObjectTypeUInt:
                                    found.value.void_func(args[0].value.uint64, args[1].value.uint64);
                                    break;
                                case TSNodeObjectTypeDouble:
                                    found.value.void_func(args[0].value.uint64, args[1].value.double64);
                                    break;
                                case TSNodeObjectTypePointer:
                                    found.value.void_func(args[0].value.uint64, args[1].value.pointer);
                                    break;
                                case TSNodeObjectTypeString:
                                    found.value.void_func(args[0].value.uint64, args[1].name);
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeDouble:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    found.value.void_func(args[0].value.double64, args[1].value.int64);
                                    break;
                                case TSNodeObjectTypeUInt:
                                    found.value.void_func(args[0].value.double64, args[1].value.uint64);
                                    break;
                                case TSNodeObjectTypeDouble:
                                    found.value.void_func(args[0].value.double64, args[1].value.double64);
                                    break;
                                case TSNodeObjectTypePointer:
                                    found.value.void_func(args[0].value.double64, args[1].value.pointer);
                                    break;
                                case TSNodeObjectTypeString:
                                    found.value.void_func(args[0].value.double64, args[1].name);
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypePointer:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    found.value.void_func(args[0].value.pointer, args[1].value.int64);
                                    break;
                                case TSNodeObjectTypeUInt:
                                    found.value.void_func(args[0].value.pointer, args[1].value.uint64);
                                    break;
                                case TSNodeObjectTypeDouble:
                                    found.value.void_func(args[0].value.pointer, args[1].value.double64);
                                    break;
                                case TSNodeObjectTypePointer:
                                    found.value.void_func(args[0].value.pointer, args[1].value.pointer);
                                    break;
                                case TSNodeObjectTypeString:
                                    found.value.void_func(args[0].value.pointer, args[1].name);
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeString:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    found.value.void_func(args[0].name, args[1].value.int64);
                                    break;
                                case TSNodeObjectTypeUInt:
                                    found.value.void_func(args[0].name, args[1].value.uint64);
                                    break;
                                case TSNodeObjectTypeDouble:
                                    found.value.void_func(args[0].name, args[1].value.double64);
                                    break;
                                case TSNodeObjectTypePointer:
                                    found.value.void_func(args[0].name, args[1].value.pointer);
                                    break;
                                case TSNodeObjectTypeString:
                                    found.value.void_func(args[0].name, args[1].name);
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[1].name);
                            }
                            break;
                        default:
                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[0].name);
                    }
                    break;
                case 3:
                    switch (args[0].type) {
                        case TSNodeObjectTypeInt:
                        case TSNodeObjectTypeChar:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].value.int64, args[1].value.int64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].value.int64, args[1].value.int64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].value.int64, args[1].value.int64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].value.int64, args[1].value.int64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].value.int64, args[1].value.int64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeUInt:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].value.int64, args[1].value.uint64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].value.int64, args[1].value.uint64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].value.int64, args[1].value.uint64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].value.int64, args[1].value.uint64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].value.int64, args[1].value.uint64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeDouble:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].value.int64, args[1].value.double64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].value.int64, args[1].value.double64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].value.int64, args[1].value.double64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].value.int64, args[1].value.double64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].value.int64, args[1].value.double64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypePointer:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].value.int64, args[1].value.pointer, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].value.int64, args[1].value.pointer, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].value.int64, args[1].value.pointer, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].value.int64, args[1].value.pointer, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].value.int64, args[1].value.pointer, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeString:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].value.int64, args[1].name, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].value.int64, args[1].name, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].value.int64, args[1].name, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].value.int64, args[1].name, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].value.int64, args[1].name, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeUInt:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].value.uint64, args[1].value.int64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].value.uint64, args[1].value.int64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].value.uint64, args[1].value.int64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].value.uint64, args[1].value.int64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].value.uint64, args[1].value.int64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeUInt:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].value.uint64, args[1].value.uint64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].value.uint64, args[1].value.uint64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].value.uint64, args[1].value.uint64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].value.uint64, args[1].value.uint64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].value.uint64, args[1].value.uint64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeDouble:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].value.uint64, args[1].value.double64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].value.uint64, args[1].value.double64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].value.uint64, args[1].value.double64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].value.uint64, args[1].value.double64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].value.uint64, args[1].value.double64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypePointer:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].value.uint64, args[1].value.pointer, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].value.uint64, args[1].value.pointer, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].value.uint64, args[1].value.pointer, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].value.uint64, args[1].value.pointer, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].value.uint64, args[1].value.pointer, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeString:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].value.uint64, args[1].name, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].value.uint64, args[1].name, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].value.uint64, args[1].name, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].value.uint64, args[1].name, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].value.uint64, args[1].name, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeDouble:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].value.double64, args[1].value.int64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].value.double64, args[1].value.int64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].value.double64, args[1].value.int64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].value.double64, args[1].value.int64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].value.double64, args[1].value.int64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeUInt:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].value.double64, args[1].value.uint64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].value.double64, args[1].value.uint64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].value.double64, args[1].value.uint64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].value.double64, args[1].value.uint64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].value.double64, args[1].value.uint64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeDouble:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].value.double64, args[1].value.double64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].value.double64, args[1].value.double64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].value.double64, args[1].value.double64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].value.double64, args[1].value.double64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].value.double64, args[1].value.double64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypePointer:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].value.double64, args[1].value.pointer, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].value.double64, args[1].value.pointer, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].value.double64, args[1].value.pointer, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].value.double64, args[1].value.pointer, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].value.double64, args[1].value.pointer, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeString:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].value.double64, args[1].name, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].value.double64, args[1].name, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].value.double64, args[1].name, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].value.double64, args[1].name, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].value.double64, args[1].name, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypePointer:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].value.pointer, args[1].value.int64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].value.pointer, args[1].value.int64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].value.pointer, args[1].value.int64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].value.pointer, args[1].value.int64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].value.pointer, args[1].value.int64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeUInt:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].value.pointer, args[1].value.uint64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].value.pointer, args[1].value.uint64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].value.pointer, args[1].value.uint64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].value.pointer, args[1].value.uint64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].value.pointer, args[1].value.uint64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeDouble:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].value.pointer, args[1].value.double64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].value.pointer, args[1].value.double64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].value.pointer, args[1].value.double64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].value.pointer, args[1].value.double64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].value.pointer, args[1].value.double64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypePointer:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].value.pointer, args[1].value.pointer, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].value.pointer, args[1].value.pointer, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].value.pointer, args[1].value.pointer, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].value.pointer, args[1].value.pointer, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].value.pointer, args[1].value.pointer, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeString:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].value.pointer, args[1].name, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].value.pointer, args[1].name, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].value.pointer, args[1].name, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].value.pointer, args[1].name, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].value.pointer, args[1].name, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeString:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].name, args[1].value.int64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].name, args[1].value.int64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].name, args[1].value.int64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].name, args[1].value.int64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].name, args[1].value.int64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeUInt:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].name, args[1].value.uint64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].name, args[1].value.uint64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].name, args[1].value.uint64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].name, args[1].value.uint64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].name, args[1].value.uint64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeDouble:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].name, args[1].value.double64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].name, args[1].value.double64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].name, args[1].value.double64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].name, args[1].value.double64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].name, args[1].value.double64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypePointer:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].name, args[1].value.pointer, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].name, args[1].value.pointer, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].name, args[1].value.pointer, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].name, args[1].value.pointer, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].name, args[1].value.pointer, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeString:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            found.value.void_func(args[0].name, args[1].name, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            found.value.void_func(args[0].name, args[1].name, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            found.value.void_func(args[0].name, args[1].name, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            found.value.void_func(args[0].name, args[1].name, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            found.value.void_func(args[0].name, args[1].name, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[2].name);
                                    }
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[1].name);
                            }
                            break;
                        default:
                            TS_PRINTF_ERROR("Unsupported argument type for void function: %s\n", args[0].name);
                    }
                    break;
                default:
                    TS_PRINTF_ERROR("Unsupported number of arguments for void function: %" PRIu32 "\n", arg_count);
            }
            break;
        }
        case TSNodeObjectTypeFunctionInt: {
            int64_t return_value = 0;
            switch (arg_count) {
                case 0:
                    return_value = found.value.int_func();
                    break;
                case 1:
                    switch (args[0].type) {
                        case TSNodeObjectTypeInt:
                        case TSNodeObjectTypeChar:
                            return_value = found.value.int_func(args[0].value.int64);
                            break;
                        case TSNodeObjectTypeUInt:
                            return_value = found.value.int_func(args[0].value.uint64);
                            break;
                        case TSNodeObjectTypeDouble:
                            return_value = found.value.int_func(args[0].value.double64);
                            break;
                        case TSNodeObjectTypePointer:
                            return_value = found.value.int_func(args[0].value.pointer);
                            break;
                        case TSNodeObjectTypeString:
                            return_value = found.value.int_func(args[0].name);
                            break;
                        default:
                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[0].name);
                    }
                    break;
                case 2:
                    switch (args[0].type) {
                        case TSNodeObjectTypeInt:
                        case TSNodeObjectTypeChar:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    return_value = found.value.int_func(args[0].value.int64, args[1].value.int64);
                                    break;
                                case TSNodeObjectTypeUInt:
                                    return_value = found.value.int_func(args[0].value.int64, args[1].value.uint64);
                                    break;
                                case TSNodeObjectTypeDouble:
                                    return_value = found.value.int_func(args[0].value.int64, args[1].value.double64);
                                    break;
                                case TSNodeObjectTypePointer:
                                    return_value = found.value.int_func(args[0].value.int64, args[1].value.pointer);
                                    break;
                                case TSNodeObjectTypeString:
                                    return_value = found.value.int_func(args[0].value.int64, args[1].name);
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeUInt:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    return_value = found.value.int_func(args[0].value.uint64, args[1].value.int64);
                                    break;
                                case TSNodeObjectTypeUInt:
                                    return_value = found.value.int_func(args[0].value.uint64, args[1].value.uint64);
                                    break;
                                case TSNodeObjectTypeDouble:
                                    return_value = found.value.int_func(args[0].value.uint64, args[1].value.double64);
                                    break;
                                case TSNodeObjectTypePointer:
                                    return_value = found.value.int_func(args[0].value.uint64, args[1].value.pointer);
                                    break;
                                case TSNodeObjectTypeString:
                                    return_value = found.value.int_func(args[0].value.uint64, args[1].name);
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeDouble:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    return_value = found.value.int_func(args[0].value.double64, args[1].value.int64);
                                    break;
                                case TSNodeObjectTypeUInt:
                                    return_value = found.value.int_func(args[0].value.double64, args[1].value.uint64);
                                    break;
                                case TSNodeObjectTypeDouble:
                                    return_value = found.value.int_func(args[0].value.double64, args[1].value.double64);
                                    break;
                                case TSNodeObjectTypePointer:
                                    return_value = found.value.int_func(args[0].value.double64, args[1].value.pointer);
                                    break;
                                case TSNodeObjectTypeString:
                                    return_value = found.value.int_func(args[0].value.double64, args[1].name);
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypePointer:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    return_value = found.value.int_func(args[0].value.pointer, args[1].value.int64);
                                    break;
                                case TSNodeObjectTypeUInt:
                                    return_value = found.value.int_func(args[0].value.pointer, args[1].value.uint64);
                                    break;
                                case TSNodeObjectTypeDouble:
                                    return_value = found.value.int_func(args[0].value.pointer, args[1].value.double64);
                                    break;
                                case TSNodeObjectTypePointer:
                                    return_value = found.value.int_func(args[0].value.pointer, args[1].value.pointer);
                                    break;
                                case TSNodeObjectTypeString:
                                    return_value = found.value.int_func(args[0].value.pointer, args[1].name);
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeString:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    return_value = found.value.int_func(args[0].name, args[1].value.int64);
                                    break;
                                case TSNodeObjectTypeUInt:
                                    return_value = found.value.int_func(args[0].name, args[1].value.uint64);
                                    break;
                                case TSNodeObjectTypeDouble:
                                    return_value = found.value.int_func(args[0].name, args[1].value.double64);
                                    break;
                                case TSNodeObjectTypePointer:
                                    return_value = found.value.int_func(args[0].name, args[1].value.pointer);
                                    break;
                                case TSNodeObjectTypeString:
                                    return_value = found.value.int_func(args[0].name, args[1].name);
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[1].name);
                            }
                            break;
                        default:
                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[0].name);
                    }
                    break;
                case 3:
                    switch (args[0].type) {
                        case TSNodeObjectTypeInt:
                        case TSNodeObjectTypeChar:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].value.int64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].value.int64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].value.int64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].value.int64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].value.int64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeUInt:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].value.uint64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].value.uint64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].value.uint64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].value.uint64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].value.uint64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeDouble:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].value.double64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].value.double64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].value.double64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].value.double64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].value.double64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypePointer:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].value.pointer, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].value.pointer, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].value.pointer, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].value.pointer, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].value.pointer, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeString:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].name, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].name, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].name, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].name, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].value.int64, args[1].name, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeUInt:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].value.int64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].value.int64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].value.int64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].value.int64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].value.int64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeUInt:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].value.uint64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].value.uint64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].value.uint64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].value.uint64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].value.uint64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeDouble:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].value.double64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].value.double64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].value.double64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].value.double64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].value.double64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypePointer:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].value.pointer, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].value.pointer, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].value.pointer, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].value.pointer, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].value.pointer, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeString:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].name, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].name, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].name, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].name, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].value.uint64, args[1].name, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeDouble:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].value.int64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].value.int64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].value.int64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].value.int64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].value.int64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeUInt:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].value.uint64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].value.uint64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].value.uint64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].value.uint64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].value.uint64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeDouble:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].value.double64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].value.double64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].value.double64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].value.double64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].value.double64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypePointer:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].value.pointer, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].value.pointer, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].value.pointer, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].value.pointer, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].value.pointer, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeString:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].name, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].name, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].name, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].name, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].value.double64, args[1].name, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypePointer:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].value.int64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].value.int64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].value.int64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].value.int64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].value.int64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeUInt:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].value.uint64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].value.uint64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].value.uint64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].value.uint64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].value.uint64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeDouble:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].value.double64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].value.double64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].value.double64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].value.double64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].value.double64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypePointer:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].value.pointer, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].value.pointer, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].value.pointer, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].value.pointer, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].value.pointer, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeString:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].name, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].name, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].name, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].name, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].value.pointer, args[1].name, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeString:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].name, args[1].value.int64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].name, args[1].value.int64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].name, args[1].value.int64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].name, args[1].value.int64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].name, args[1].value.int64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeUInt:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].name, args[1].value.uint64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].name, args[1].value.uint64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].name, args[1].value.uint64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].name, args[1].value.uint64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].name, args[1].value.uint64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeDouble:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].name, args[1].value.double64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].name, args[1].value.double64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].name, args[1].value.double64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].name, args[1].value.double64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].name, args[1].value.double64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypePointer:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].name, args[1].value.pointer, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].name, args[1].value.pointer, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].name, args[1].value.pointer, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].name, args[1].value.pointer, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].name, args[1].value.pointer, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeString:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.int_func(args[0].name, args[1].name, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.int_func(args[0].name, args[1].name, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.int_func(args[0].name, args[1].name, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.int_func(args[0].name, args[1].name, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.int_func(args[0].name, args[1].name, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[2].name);
                                    }
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[1].name);
                            }
                            break;
                        default:
                            TS_PRINTF_ERROR("Unsupported argument type for int function: %s\n", args[0].name);
                    }
                    break;
                default:
                    TS_PRINTF_ERROR("Unsupported number of arguments for int function: %" PRIu32 "\n", arg_count);
            }
            obj.value.int64 = return_value;
            break;
        }
        case TSNodeObjectTypeFunctionUInt: {
            uint64_t return_value = 0;
            switch (arg_count) {
                case 0:
                    return_value = found.value.uint_func();
                    break;
                case 1:
                    switch (args[0].type) {
                        case TSNodeObjectTypeInt:
                        case TSNodeObjectTypeChar:
                            return_value = found.value.uint_func(args[0].value.int64);
                            break;
                        case TSNodeObjectTypeUInt:
                            return_value = found.value.uint_func(args[0].value.uint64);
                            break;
                        case TSNodeObjectTypeDouble:
                            return_value = found.value.uint_func(args[0].value.double64);
                            break;
                        case TSNodeObjectTypePointer:
                            return_value = found.value.uint_func(args[0].value.pointer);
                            break;
                        case TSNodeObjectTypeString:
                            return_value = found.value.uint_func(args[0].name);
                            break;
                        default:
                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[0].name);
                    }
                    break;
                case 2:
                    switch (args[0].type) {
                        case TSNodeObjectTypeInt:
                        case TSNodeObjectTypeChar:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    return_value = found.value.uint_func(args[0].value.int64, args[1].value.int64);
                                    break;
                                case TSNodeObjectTypeUInt:
                                    return_value = found.value.uint_func(args[0].value.int64, args[1].value.uint64);
                                    break;
                                case TSNodeObjectTypeDouble:
                                    return_value = found.value.uint_func(args[0].value.int64, args[1].value.double64);
                                    break;
                                case TSNodeObjectTypePointer:
                                    return_value = found.value.uint_func(args[0].value.int64, args[1].value.pointer);
                                    break;
                                case TSNodeObjectTypeString:
                                    return_value = found.value.uint_func(args[0].value.int64, args[1].name);
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeUInt:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    return_value = found.value.uint_func(args[0].value.uint64, args[1].value.int64);
                                    break;
                                case TSNodeObjectTypeUInt:
                                    return_value = found.value.uint_func(args[0].value.uint64, args[1].value.uint64);
                                    break;
                                case TSNodeObjectTypeDouble:
                                    return_value = found.value.uint_func(args[0].value.uint64, args[1].value.double64);
                                    break;
                                case TSNodeObjectTypePointer:
                                    return_value = found.value.uint_func(args[0].value.uint64, args[1].value.pointer);
                                    break;
                                case TSNodeObjectTypeString:
                                    return_value = found.value.uint_func(args[0].value.uint64, args[1].name);
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeDouble:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    return_value = found.value.uint_func(args[0].value.double64, args[1].value.int64);
                                    break;
                                case TSNodeObjectTypeUInt:
                                    return_value = found.value.uint_func(args[0].value.double64, args[1].value.uint64);
                                    break;
                                case TSNodeObjectTypeDouble:
                                    return_value = found.value.uint_func(args[0].value.double64, args[1].value.double64);
                                    break;
                                case TSNodeObjectTypePointer:
                                    return_value = found.value.uint_func(args[0].value.double64, args[1].value.pointer);
                                    break;
                                case TSNodeObjectTypeString:
                                    return_value = found.value.uint_func(args[0].value.double64, args[1].name);
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypePointer:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    return_value = found.value.uint_func(args[0].value.pointer, args[1].value.int64);
                                    break;
                                case TSNodeObjectTypeUInt:
                                    return_value = found.value.uint_func(args[0].value.pointer, args[1].value.uint64);
                                    break;
                                case TSNodeObjectTypeDouble:
                                    return_value = found.value.uint_func(args[0].value.pointer, args[1].value.double64);
                                    break;
                                case TSNodeObjectTypePointer:
                                    return_value = found.value.uint_func(args[0].value.pointer, args[1].value.pointer);
                                    break;
                                case TSNodeObjectTypeString:
                                    return_value = found.value.uint_func(args[0].value.pointer, args[1].name);
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeString:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    return_value = found.value.uint_func(args[0].name, args[1].value.int64);
                                    break;
                                case TSNodeObjectTypeUInt:
                                    return_value = found.value.uint_func(args[0].name, args[1].value.uint64);
                                    break;
                                case TSNodeObjectTypeDouble:
                                    return_value = found.value.uint_func(args[0].name, args[1].value.double64);
                                    break;
                                case TSNodeObjectTypePointer:
                                    return_value = found.value.uint_func(args[0].name, args[1].value.pointer);
                                    break;
                                case TSNodeObjectTypeString:
                                    return_value = found.value.uint_func(args[0].name, args[1].name);
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[1].name);
                            }
                            break;
                        default:
                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[0].name);
                    }
                    break;
                case 3:
                    switch (args[0].type) {
                        case TSNodeObjectTypeInt:
                        case TSNodeObjectTypeChar:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].value.int64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].value.int64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].value.int64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].value.int64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].value.int64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeUInt:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].value.uint64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].value.uint64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].value.uint64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].value.uint64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].value.uint64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeDouble:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].value.double64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].value.double64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].value.double64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].value.double64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].value.double64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypePointer:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].value.pointer, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].value.pointer, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].value.pointer, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].value.pointer, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].value.pointer, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeString:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].name, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].name, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].name, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].name, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].value.int64, args[1].name, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeUInt:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].value.uint64, args[1].value.int64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].value.uint64, args[1].value.int64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].value.uint64, args[1].value.int64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].value.uint64, args[1].value.int64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].value.uint64, args[1].value.int64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeUInt:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].value.uint64, args[1].value.uint64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].value.uint64, args[1].value.uint64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].value.uint64, args[1].value.uint64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].value.uint64, args[1].value.uint64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].value.uint64, args[1].value.uint64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeDouble:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].value.uint64, args[1].value.double64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].value.uint64, args[1].value.double64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].value.uint64, args[1].value.double64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].value.uint64, args[1].value.double64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].value.uint64, args[1].value.double64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypePointer:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].value.uint64, args[1].value.pointer, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].value.uint64, args[1].value.pointer, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].value.uint64, args[1].value.pointer, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].value.uint64, args[1].value.pointer, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].value.uint64, args[1].value.pointer, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeString:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.uint64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.uint64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.uint64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.pointer, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.uint64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeDouble:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].value.double64, args[1].value.int64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].value.double64, args[1].value.int64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].value.double64, args[1].value.int64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].value.double64, args[1].value.int64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].value.double64, args[1].value.int64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeUInt:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].value.double64, args[1].value.uint64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].value.double64, args[1].value.uint64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].value.double64, args[1].value.uint64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].value.double64, args[1].value.uint64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].value.double64, args[1].value.uint64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeDouble:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].value.double64, args[1].value.double64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].value.double64, args[1].value.double64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].value.double64, args[1].value.double64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].value.double64, args[1].value.double64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].value.double64, args[1].value.double64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypePointer:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].value.double64, args[1].value.pointer, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].value.double64, args[1].value.pointer, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].value.double64, args[1].value.pointer, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].value.double64, args[1].value.pointer, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].value.double64, args[1].value.pointer, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeString:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.double64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.double64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.double64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.double64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.double64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypePointer:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].value.pointer, args[1].value.int64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].value.pointer, args[1].value.int64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].value.pointer, args[1].value.int64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].value.pointer, args[1].value.int64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].value.pointer, args[1].value.int64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeUInt:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].value.pointer, args[1].value.uint64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].value.pointer, args[1].value.uint64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].value.pointer, args[1].value.uint64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].value.pointer, args[1].value.uint64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].value.pointer, args[1].value.uint64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeDouble:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].value.pointer, args[1].value.double64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].value.pointer, args[1].value.double64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].value.pointer, args[1].value.double64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].value.pointer, args[1].value.double64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].value.pointer, args[1].value.double64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypePointer:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].value.pointer, args[1].value.pointer, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].value.pointer, args[1].value.pointer, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].value.pointer, args[1].value.pointer, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].value.pointer, args[1].value.pointer, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].value.pointer, args[1].value.pointer, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeString:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.pointer, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.pointer, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.pointer, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.pointer, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.pointer, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeString:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.int64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.int64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.int64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.int64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.int64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeUInt:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.uint64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.uint64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.uint64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.uint64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.uint64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeDouble:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.double64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.double64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.double64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.double64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.double64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypePointer:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.pointer, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.pointer, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.pointer, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.pointer, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].name, args[1].value.pointer, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeString:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.uint_func(args[0].name, args[1].name, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.uint_func(args[0].name, args[1].name, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.uint_func(args[0].name, args[1].name, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.uint_func(args[0].name, args[1].name, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.uint_func(args[0].name, args[1].name, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[2].name);
                                    }
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[1].name);
                            }
                            break;
                        default:
                            TS_PRINTF_ERROR("Unsupported argument type for uint function: %s\n", args[0].name);
                    }
                    break;
                default:
                    TS_PRINTF_ERROR("Unsupported number of arguments for uint function: %" PRIu32 "\n", arg_count);
            }
            obj.value.uint64 = return_value;
            break;
        }
        case TSNodeObjectTypeFunctionPointer: {
            void* return_value = NULL;
            switch (arg_count) {
                case 0:
                    return_value = found.value.pointer_func();
                    break;
                case 1:
                    switch (args[0].type) {
                        case TSNodeObjectTypeInt:
                        case TSNodeObjectTypeChar:
                            return_value = found.value.pointer_func(args[0].value.int64);
                            break;
                        case TSNodeObjectTypeUInt:
                            return_value = found.value.pointer_func(args[0].value.uint64);
                            break;
                        case TSNodeObjectTypeDouble:
                            return_value = found.value.pointer_func(args[0].value.double64);
                            break;
                        case TSNodeObjectTypePointer:
                            return_value = found.value.pointer_func(args[0].value.pointer);
                            break;
                        case TSNodeObjectTypeString:
                            return_value = found.value.pointer_func(args[0].name);
                            break;
                        default:
                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[0].name);
                    }
                    break;
                case 2:
                    switch (args[0].type) {
                        case TSNodeObjectTypeInt:
                        case TSNodeObjectTypeChar:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    return_value = found.value.pointer_func(args[0].value.int64, args[1].value.int64);
                                    break;
                                case TSNodeObjectTypeUInt:
                                    return_value = found.value.pointer_func(args[0].value.int64, args[1].value.uint64);
                                    break;
                                case TSNodeObjectTypeDouble:
                                    return_value = found.value.pointer_func(args[0].value.int64, args[1].value.double64);
                                    break;
                                case TSNodeObjectTypePointer:
                                    return_value = found.value.pointer_func(args[0].value.int64, args[1].value.pointer);
                                    break;
                                case TSNodeObjectTypeString:
                                    return_value = found.value.pointer_func(args[0].value.int64, args[1].name);
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeUInt:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.int64);
                                    break;
                                case TSNodeObjectTypeUInt:
                                    return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.uint64);
                                    break;
                                case TSNodeObjectTypeDouble:
                                    return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.double64);
                                    break;
                                case TSNodeObjectTypePointer:
                                    return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.pointer);
                                    break;
                                case TSNodeObjectTypeString:
                                    return_value = found.value.pointer_func(args[0].value.uint64, args[1].name);
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeDouble:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    return_value = found.value.pointer_func(args[0].value.double64, args[1].value.int64);
                                    break;
                                case TSNodeObjectTypeUInt:
                                    return_value = found.value.pointer_func(args[0].value.double64, args[1].value.uint64);
                                    break;
                                case TSNodeObjectTypeDouble:
                                    return_value = found.value.pointer_func(args[0].value.double64, args[1].value.double64);
                                    break;
                                case TSNodeObjectTypePointer:
                                    return_value = found.value.pointer_func(args[0].value.double64, args[1].value.pointer);
                                    break;
                                case TSNodeObjectTypeString:
                                    return_value = found.value.pointer_func(args[0].value.double64, args[1].name);
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypePointer:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.int64);
                                    break;
                                case TSNodeObjectTypeUInt:
                                    return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.uint64);
                                    break;
                                case TSNodeObjectTypeDouble:
                                    return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.double64);
                                    break;
                                case TSNodeObjectTypePointer:
                                    return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.pointer);
                                    break;
                                case TSNodeObjectTypeString:
                                    return_value = found.value.pointer_func(args[0].value.pointer, args[1].name);
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeString:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    return_value = found.value.pointer_func(args[0].name, args[1].value.int64);
                                    break;
                                case TSNodeObjectTypeUInt:
                                    return_value = found.value.pointer_func(args[0].name, args[1].value.uint64);
                                    break;
                                case TSNodeObjectTypeDouble:
                                    return_value = found.value.pointer_func(args[0].name, args[1].value.double64);
                                    break;
                                case TSNodeObjectTypePointer:
                                    return_value = found.value.pointer_func(args[0].name, args[1].value.pointer);
                                    break;
                                case TSNodeObjectTypeString:
                                    return_value = found.value.pointer_func(args[0].name, args[1].name);
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[1].name);
                            }
                            break;
                        default:
                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[0].name);
                    }
                    break;
                case 3:
                    switch (args[0].type) {
                        case TSNodeObjectTypeInt:
                        case TSNodeObjectTypeChar:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].value.int64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].value.int64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].value.int64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].value.int64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].value.int64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeUInt:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].value.uint64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].value.uint64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].value.uint64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].value.uint64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].value.uint64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeDouble:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].value.double64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].value.double64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].value.double64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].value.double64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].value.double64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypePointer:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].value.pointer, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].value.pointer, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].value.pointer, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].value.pointer, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].value.pointer, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeString:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].name, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].name, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].name, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].name, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.pointer_func(args[0].value.int64, args[1].name, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[2].name);
                                    }
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeUInt:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.int64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.int64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.int64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.int64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.int64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeUInt:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.uint64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.uint64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.uint64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.uint64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.uint64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeDouble:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.double64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.double64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.double64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.double64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.double64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypePointer:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.pointer, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.pointer, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.pointer, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.pointer, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].value.pointer, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeString:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].name, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].name, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].name, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].name, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.pointer_func(args[0].value.uint64, args[1].name, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[2].name);
                                    }
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypeDouble:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].value.int64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].value.int64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].value.int64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].value.int64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].value.int64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeUInt:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].value.uint64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].value.uint64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].value.uint64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].value.uint64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].value.uint64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeDouble:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].value.double64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].value.double64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].value.double64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].value.double64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].value.double64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypePointer:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].value.pointer, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].value.pointer, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].value.pointer, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].value.pointer, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].value.pointer, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeString:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].name, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].name, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].name, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].name, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.pointer_func(args[0].value.double64, args[1].name, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[2].name);
                                    }
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[1].name);
                            }
                            break;
                        case TSNodeObjectTypePointer:
                            switch (args[1].type) {
                                case TSNodeObjectTypeInt:
                                case TSNodeObjectTypeChar:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.int64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.int64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.int64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.int64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.int64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeUInt:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.uint64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.uint64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.uint64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.uint64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.uint64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeDouble:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.double64, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.double64, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.double64, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.double64, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.double64, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypePointer:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.pointer, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.pointer, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.pointer, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.pointer, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].value.pointer, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[2].name);
                                    }
                                    break;
                                case TSNodeObjectTypeString:
                                    switch (args[2].type) {
                                        case TSNodeObjectTypeInt:
                                        case TSNodeObjectTypeChar:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].name, args[2].value.int64);
                                            break;
                                        case TSNodeObjectTypeUInt:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].name, args[2].value.uint64);
                                            break;
                                        case TSNodeObjectTypeDouble:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].name, args[2].value.double64);
                                            break;
                                        case TSNodeObjectTypePointer:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].name, args[2].value.pointer);
                                            break;
                                        case TSNodeObjectTypeString:
                                            return_value = found.value.pointer_func(args[0].value.pointer, args[1].name, args[2].name);
                                            break;
                                        default:
                                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[2].name);
                                    }
                                    break;
                                default:
                                    TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[1].name);
                            }
                            break;
                        default:
                            TS_PRINTF_ERROR("Unsupported argument type for pointer function: %s\n", args[0].name);
                    }
                    break;
                default:
                    TS_PRINTF_ERROR("Unknown function type during call: %" PRIu32 "\n", found.type);
            }
            obj.value.pointer = return_value;
            break;
        }
        default:
            TS_PRINTF_ERROR("Unknown function return type during call: %" PRIu32 "\n", found.type);
    }

    return obj;
}
