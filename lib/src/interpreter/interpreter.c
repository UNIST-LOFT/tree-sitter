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
        obj.type=TSNodeObjectTypeChar;
        obj.value.int64=ts_node_find_value(node)[1];
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

TSNodeObject ts_interpreter_simulate(TSNode node, uint64_t var_count, TSNodeObject* vars) {
    if (strcmp(ts_node_type(node),"identifier")==0 || strcmp(ts_node_type(node),"field_expression")==0) {
        return ts_interpreter_variable(node,var_count,vars);
    }
    else if (strcmp(ts_node_type(node),"number_literal")==0 || strcmp(ts_node_type(node),"char_literal")==0) {
        return ts_interpreter_literal(node);
    }
    else if (strcmp(ts_node_type(node),"unary_expression")==0) {
        return ts_interpreter_unary(node,var_count,vars);
    }
    else if (strcmp(ts_node_type(node),"binary_expression")==0) {
        return ts_interpreter_binary(node,var_count,vars);
    }
    else if (strcmp(ts_node_type(node), "assignment_expression") == 0) {
        return ts_interpreter_assign(node,var_count,vars);
    }
    else if (strcmp(ts_node_type(node),"string_literal")==0) {
        TSNodeObject obj;
        obj.name=ts_node_find_value(node);
        obj.node=node;
        obj.size=sizeof(char)*(strlen(ts_node_find_value(node))-2); // Remove quotes
        obj.type=TSNodeObjectTypeString;
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
    else if (strcmp(ts_node_type(node),"call_expression")==0) {
        return ts_interpreter_function(node,var_count,vars);
    }
    else if (strcmp(ts_node_type(node),"parenthesized_expression")==0 ||
            strcmp(ts_node_type(node),"expression_statement")==0 ||
            strcmp(ts_node_type(node),"ERROR")==0) {
        return ts_interpreter_simulate(ts_node_named_child(node,0),var_count,vars);
    }
    else {
        TS_PRINTF_ERROR("Unsupported node type in interpreter: %s\n", ts_node_type(node));
    }
}