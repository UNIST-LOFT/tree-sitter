#include "tree_sitter/api.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

TSNodeObject ts_interpreter_variable(TSNode node, uint64_t var_count, TSNodeObject* vars) {
    char* node_name=ts_node_find_value(node);
    for (size_t i=0;i<var_count;i++) {
        if (strcmp(node_name, vars[i].name)==0) {
            return vars[i];
        }
    }

    TS_PRINTF_ERROR("Variable not found: %s\n", node_name);
}

int in_str(char* str, char c) {
    for (size_t i=0;i<strlen(str);i++) {
        if (str[i]==c) {
            return 1;
        }
    }
    return 0;
}

int is_postfix(char* str,char* postfix) {
    if (strlen(str)<strlen(postfix)) {
        return 0;
    }
    for (size_t i=0;i<strlen(postfix);i++) {
        if (str[strlen(str)-strlen(postfix)+i]!=postfix[i]) {
            return 0;
        }
    }
    return 1;
}

TSNodeObject ts_interpreter_literal(TSNode node) {
    TSNodeObject obj;
    obj.name=ts_node_find_value(node);
    obj.node=node;

    if (strcmp(ts_node_type(node),"char_literal")==0) {
        obj.size=sizeof(char);
        obj.type=TSNodeObjectTypeInt;
        obj.value.int64=ts_node_find_value(node)[0];
    }
    else if (in_str(obj.name,'.')) {
        // Float/double
        if (in_str(obj.name,'f') || in_str(obj.name,'F')) {
            obj.size=sizeof(float);
        }
        else {
            obj.size=sizeof(double);
        }

        obj.type=TSNodeObjectTypeDouble;
        obj.value.double64=atof(ts_node_find_value(node));
        return obj;
    }
    else if (in_str(obj.name,'u') || in_str(obj.name,'U')) {
        // unsigned
        int long_size=sizeof(unsigned long);

        if (is_postfix(obj.name,"ULL") || is_postfix(obj.name,"ull") ||
                    is_postfix(obj.name,"LLU") || is_postfix(obj.name,"llu") ||
                    (long_size==8 && (is_postfix(obj.name,"UL") || is_postfix(obj.name,"ul") ||
                    is_postfix(obj.name,"LU") || is_postfix(obj.name,"lu")))) {
            obj.size=sizeof(unsigned long long);
        }
        else if (is_postfix(obj.name,"U") || is_postfix(obj.name,"u") ||
                    (long_size==4 && (is_postfix(obj.name,"UL") || is_postfix(obj.name,"ul") ||
                    is_postfix(obj.name,"LU") || is_postfix(obj.name,"lu")))) {
            obj.size=sizeof(unsigned long);
        }
        else {
            TS_PRINTF_ERROR("Unknown unsigned type: %s\n", obj.name);
        }

        obj.type=TSNodeObjectTypeUInt;
        obj.value.uint64=(uint64_t)atoll(ts_node_find_value(node));
    }
    else {
        // signed
        int long_size=sizeof(long);

        if (is_postfix(obj.name,"LL") || is_postfix(obj.name,"ll") ||
                    (long_size==8 && (is_postfix(obj.name,"L") || is_postfix(obj.name,"l")))) {
            obj.size=sizeof(long long);
        }
        else if (in_str(obj.name,'\'')) {
            // Char literal
            obj.size=sizeof(char);
        }
        else if (long_size==4 && (is_postfix(obj.name,"L") || is_postfix(obj.name,"l"))) {
            obj.size=sizeof(long);
        }
        else {
            obj.size=sizeof(int);
        }

        obj.type=TSNodeObjectTypeInt;
        obj.value.int64=atoll(ts_node_find_value(node));
    }

    return obj;
}

TSNodeObject ts_interpreter_casting(TSNode node, uint64_t var_count, TSNodeObject* vars) {
    TSNodeObject obj = ts_interpreter_simulate(ts_node_named_child(node, 1), var_count, vars); // Casted value
    char* cast_type = ts_node_find_value(node); // Casted type
    if (strcmp(cast_type, "int8_t") == 0)
        obj.size = 1;
    else if (strcmp(cast_type, "int16_t") == 0)
        obj.size = 2;
    else if (strcmp(cast_type, "int32_t") == 0)
        obj.size = 4;
    else if (strcmp(cast_type, "int64_t") == 0)
        obj.size = 8;
    else if (strcmp(cast_type, "uint8_t") == 0)
        obj.size = 1;
    else if (strcmp(cast_type, "uint16_t") == 0)
        obj.size = 2;
    else if (strcmp(cast_type, "uint32_t") == 0)
        obj.size = 4;
    else if (strcmp(cast_type, "uint64_t") == 0)
        obj.size = 8;
    else if (strcmp(cast_type, "float") == 0)
        obj.size = 4;
    else if (strcmp(cast_type, "double") == 0)
        obj.size = 8;
    // Pointer casts
    else if (strcmp(cast_type, "int8_t*") == 0 || strcmp(cast_type, "uint8_t*") == 0) {
        if (obj.type == TSNodeObjectTypePointer) {
            // ptr to ptr
            obj.array_element_size = 1;
            obj.size = sizeof(void*);
        }
        else {
            // other to ptr
            obj.type = TSNodeObjectTypePointer;
            obj.array_element_size = 1;
            obj.size = sizeof(void*);
            if (strcmp(cast_type, "int8_t*") == 0) {
                obj.value.pointer = *((int8_t**)obj.reference);
                obj.array_element_type = TSNodeObjectTypeInt;
            }
            else {
                obj.value.pointer = *((uint8_t**)obj.reference);
                obj.array_element_type = TSNodeObjectTypeUInt;
            }
        }
    }
    else if (strcmp(cast_type, "int16_t*") == 0 || strcmp(cast_type, "uint16_t*") == 0) {
        if (obj.type == TSNodeObjectTypePointer) {
            obj.array_element_size = 2;
            obj.size = sizeof(void*);
        }
        else {
            obj.type = TSNodeObjectTypePointer;
            obj.array_element_size = 2;
            obj.size = sizeof(void*);
            if (strcmp(cast_type, "int16_t*") == 0) {
                obj.value.pointer = *((int16_t**)obj.reference);
                obj.array_element_type = TSNodeObjectTypeInt;
            }
            else {
                obj.value.pointer = *((uint16_t**)obj.reference);
                obj.array_element_type = TSNodeObjectTypeUInt;
            }
        }
    }
    else if (strcmp(cast_type, "int32_t*") == 0 || strcmp(cast_type, "uint32_t*") == 0 ||
             strcmp(cast_type, "float*") == 0) {
        if (obj.type == TSNodeObjectTypePointer) {
            obj.array_element_size = 4;
            obj.size = sizeof(void*);
        }
        else {
            obj.type = TSNodeObjectTypePointer;
            obj.array_element_size = 4;
            obj.size = sizeof(void*);
            if (strcmp(cast_type, "int32_t*") == 0) {
                obj.value.pointer = *((int32_t**)obj.reference);
                obj.array_element_type = TSNodeObjectTypeInt;
            }
            else if (strcmp(cast_type, "uint32_t*") == 0) {
                obj.value.pointer = *((uint32_t**)obj.reference);
                obj.array_element_type = TSNodeObjectTypeUInt;
            }
            else {
                obj.value.pointer = *((float**)obj.reference);
                obj.array_element_type = TSNodeObjectTypeDouble;
            }
        }
    }
    else if (strcmp(cast_type, "int64_t*") == 0 || strcmp(cast_type, "uint64_t*") == 0 ||
             strcmp(cast_type, "double*") == 0) {
        if (obj.type == TSNodeObjectTypePointer) {
            obj.array_element_size = 8;
            obj.size = sizeof(void*);
        }
        else {
            obj.type = TSNodeObjectTypePointer;
            obj.array_element_size = 8;
            obj.size = sizeof(void*);
            if (strcmp(cast_type, "int64_t*") == 0) {
                obj.value.pointer = *((int64_t**)obj.reference);
                obj.array_element_type = TSNodeObjectTypeInt;
            }
            else if (strcmp(cast_type, "uint64_t*") == 0) {
                obj.value.pointer = *((uint64_t**)obj.reference);
                obj.array_element_type = TSNodeObjectTypeUInt;
            }
            else {
                obj.value.pointer = *((double**)obj.reference);
                obj.array_element_type = TSNodeObjectTypeDouble;
            }
        }
    }
    else {
        TS_PRINTF_ERROR("Unsupported cast type: %s\n", cast_type);
    }
    return obj;
}

TSNodeObject ts_interpreter_subscript(TSNode node, uint64_t var_count, TSNodeObject* vars) {
    // Base is pointer variable
    TSNodeObject base_obj = ts_interpreter_variable(ts_node_named_child(node, 0), var_count, vars);
    TSNodeObject index_obj = ts_interpreter_simulate(ts_node_named_child(node, 1), var_count, vars);

    TSNodeObject obj;
    obj.name = NULL; // No name for subscript result
    obj.node = node;
    obj.size = base_obj.array_element_size;
    // TODO: Handle other types: add pointee type information in TSNodeObject
    obj.type = TSNodeObjectTypePointer;
    int32_t index;
    if (index_obj.type == TSNodeObjectTypeInt) {
        index = (int32_t)(index_obj.value.int64);
    }
    else if (index_obj.type == TSNodeObjectTypeUInt) {
        index = (int32_t)(index_obj.value.uint64);
    }
    else {
        TS_PRINTF_ERROR("Array index must be int or uint type\n");
    }
    obj.reference = (void*)((unsigned char*)base_obj.value.pointer + (index * base_obj.array_element_size)); // Compute offset
    obj.value.int64 = *((int32_t*)obj.reference); // Store pointer value as int64
    obj.array_element_size = 0; // Not an array
    return obj;
}

TSNodeObject ts_interpreter_simulate(TSNode node, uint64_t var_count, TSNodeObject* vars) {
    if (strcmp(ts_node_type(node),"identifier")==0 || strcmp(ts_node_type(node),"field_expression")==0 ||
        strcmp(ts_node_type(node),"statement_identifier")==0) {
        return ts_interpreter_variable(node,var_count,vars);
    }
    else if (strcmp(ts_node_type(node),"number_literal")==0 || strcmp(ts_node_type(node),"char_literal")==0) {
        return ts_interpreter_literal(node);
    }
    else if (strcmp(ts_node_type(node),"unary_expression")==0 ||
             strcmp(ts_node_type(node),"pointer_expression")==0 ||
             strcmp(ts_node_type(node),"update_expression")==0) {
        return ts_interpreter_unary(node,var_count,vars);
    }
    else if (strcmp(ts_node_type(node),"binary_expression")==0) {
        return ts_interpreter_binary(node,var_count,vars);
    }
    else if (strcmp(ts_node_type(node), "assignment_expression") == 0) {
        return ts_interpreter_assign(node,var_count,vars);
    }
    else if (strcmp(ts_node_type(node),"string_literal")==0) {
        // For string literal, we convert it to pointer of char(s)
        TSNodeObject obj;
        char* value = malloc(sizeof(char)*(strlen(ts_node_find_value(node))+1));
        strcpy(value, ts_node_find_value(node));
        value[strlen(value)] = '\0';
        obj.name=value;
        obj.node=node;
        obj.size=sizeof(char)*(strlen(ts_node_find_value(node))-2); // Remove quotes
        obj.type=TSNodeObjectTypeString;
        obj.reference=&value;
        obj.value.pointer=value;
        return obj;
    }
    else if (strcmp(ts_node_type(node),"true")==0) {
        TSNodeObject obj;
        obj.name="true";
        obj.node=node;
        obj.size=sizeof(unsigned int); // Remove quotes
        obj.type=TSNodeObjectTypeUInt;
        obj.value.uint64=1;
        return obj;
    }
    else if (strcmp(ts_node_type(node),"false")==0) {
        TSNodeObject obj;
        obj.name="false";
        obj.node=node;
        obj.size=sizeof(unsigned int); // Remove quotes
        obj.type=TSNodeObjectTypeUInt;
        obj.value.uint64=0;
        return obj;
    }
    else if (strcmp(ts_node_type(node), "null")==0) {
        TSNodeObject obj;
        obj.name="null";
        obj.node=node;
        obj.size=sizeof(void*);
        obj.type=TSNodeObjectTypePointer;
        obj.value.pointer=NULL;
        return obj;
    }
    else if (strcmp(ts_node_type(node), "cast_expression") == 0) {
        return ts_interpreter_casting(node, var_count, vars);
    }
    else if (strcmp(ts_node_type(node), "conditional_expression") == 0) {
        TSNodeObject obj;
        TSNodeObject cond_result = ts_interpreter_simulate(ts_node_named_child(node, 0),var_count, vars);
        if (cond_result.value.int64) {
            obj = ts_interpreter_simulate(ts_node_named_child(node, 1), var_count, vars);
        }
        else {
            obj = ts_interpreter_simulate(ts_node_named_child(node, 2), var_count, vars);
        }
        return obj;
    }
    else if (strcmp(ts_node_type(node),"call_expression")==0) {
        return ts_interpreter_function(node,var_count,vars);
    }
    else if (strcmp(ts_node_type(node), "compound_statement") == 0) {
        TSNodeObject obj;
        for (size_t i = 0; i < ts_node_named_child_count(node); i++) {
            obj = ts_interpreter_simulate(ts_node_named_child(node, i), var_count, vars);
        }
        return obj;
    }
    else if (strcmp(ts_node_type(node), "subscript_expression") == 0) {
        return ts_interpreter_subscript(node, var_count, vars);
    }
    else if (strcmp(ts_node_type(node),"parenthesized_expression")==0 ||
            strcmp(ts_node_type(node),"expression_statement")==0 ||
            strcmp(ts_node_type(node),"subscript_argument_list")==0 ||
            strcmp(ts_node_type(node),"ERROR")==0) {
        return ts_interpreter_simulate(ts_node_named_child(node,0),var_count,vars);
    }
    /* control flow statements do not return interpreter; just jump */
    else if (strcmp(ts_node_type(node), "continue_statement")==0) {
        TSNodeObject obj;
        int8_t found = 0;
        for (size_t i=0;i<var_count;i++) {
            if (strcmp(vars[i].name, "continue")==0) {
                obj = vars[i];
                found = 1;
                break;
            }
        }
        if (!found || obj.type != TSNodeObjectTypeJmpBuf) {
            TS_PRINTF_ERROR("Continue statement found but no corresponding jmp_buf in vars\n");
        }
        longjmp(*(obj.value.jmpbuf), 1);
    }
    else if (strcmp(ts_node_type(node), "break_statement")==0) {
        TSNodeObject obj;
        int8_t found = 0;
        for (size_t i=0;i<var_count;i++) {
            if (strcmp(vars[i].name, "break")==0) {
                obj = vars[i];
                found = 1;
                break;
            }
        }
        if (!found || obj.type != TSNodeObjectTypeJmpBuf) {
            TS_PRINTF_ERROR("Break statement found but no corresponding jmp_buf in vars\n");
        }
        longjmp(*(obj.value.jmpbuf), 1);
    }
    else if (strcmp(ts_node_type(node), "goto_statement")==0) {
        TSNodeObject obj;
        char* label_name = malloc(7 + strlen(ts_node_find_value(ts_node_named_child(node, 0)))); // "goto " + label name
        sprintf(label_name, "goto %s", ts_node_find_value(ts_node_named_child(node, 0)));
        int8_t found = 0;
        for (size_t i=0;i<var_count;i++) {
            if (strcmp(vars[i].name, label_name)==0) {
                obj = vars[i];
                found = 1;
                break;
            }
        }
        free(label_name);
        if (!found || obj.type != TSNodeObjectTypeJmpBuf) {
            TS_PRINTF_ERROR("Goto statement found but no corresponding jmp_buf in vars\n");
        }
        longjmp(*(obj.value.jmpbuf), 1);
    }
    else if (strcmp(ts_node_type(node), "return_statement")==0) {
        TSNodeObject obj;
        int8_t found = 0;
        for (size_t i=0;i<var_count;i++) {
            if (strcmp(vars[i].name, "return")==0) {
                obj = vars[i];
                found = 1;
                break;
            }
        }
        if (!found || obj.type != TSNodeObjectTypeJmpBuf) {
            TS_PRINTF_ERROR("Return statement found but no corresponding jmp_buf in vars\n");
        }
        longjmp(*(obj.value.jmpbuf), obj.array_element_size); // array_element_size is patch ID
    }
    else {
        TS_PRINTF_ERROR("Unsupported node type in interpreter: %s\n", ts_node_type(node));
    }
}