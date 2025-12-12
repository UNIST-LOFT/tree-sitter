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
        result.value.int64=!obj.value.int64;
    }
    else if (strcmp(op,"++")==0) {
        result.size=obj.size;
        switch (obj.type) {
            case TSNodeObjectTypeInt:
                result.type=TSNodeObjectTypeInt;
                result.value.int64=++obj.value.int64;
                break;
            case TSNodeObjectTypeUInt:
                result.type=TSNodeObjectTypeUInt;
                result.value.uint64=++obj.value.uint64;
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
                break;
            case TSNodeObjectTypeUInt:
                result.type=TSNodeObjectTypeUInt;
                result.value.uint64=--obj.value.uint64;
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
                result.value.int64=obj.value.int64++;
                break;
            case TSNodeObjectTypeUInt:
                result.type=TSNodeObjectTypeUInt;
                result.value.uint64=obj.value.uint64++;
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
                result.value.int64=obj.value.int64--;
                break;
            case TSNodeObjectTypeUInt:
                result.type=TSNodeObjectTypeUInt;
                result.value.uint64=obj.value.uint64--;
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
        switch (obj1.type) {
            case TSNodeObjectTypeInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.type=TSNodeObjectTypeInt;
                        result.value.int64=obj1.value.int64+obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.type=TSNodeObjectTypeUInt;
                        result.value.uint64=obj1.value.int64+obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.type=TSNodeObjectTypeDouble;
                        result.value.double64=obj1.value.int64+obj2.value.double64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;
            
            case TSNodeObjectTypeUInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.type=TSNodeObjectTypeUInt;
                        result.value.uint64=obj1.value.uint64+obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.type=TSNodeObjectTypeUInt;
                        result.value.uint64=obj1.value.uint64+obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.type=TSNodeObjectTypeDouble;
                        result.value.double64=obj1.value.uint64+obj2.value.double64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypeDouble:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.type=TSNodeObjectTypeDouble;
                        result.value.double64=obj1.value.double64+obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.type=TSNodeObjectTypeDouble;
                        result.value.double64=obj1.value.double64+obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.type=TSNodeObjectTypeDouble;
                        result.value.double64=obj1.value.double64+obj2.value.double64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;
            
            default:
                TS_PRINTF_ERROR("Unknown type in addition: %d\n", obj1.type);
        }
    }
    else if (strcmp(op,"-")==0) {
        switch (obj1.type) {
            case TSNodeObjectTypeInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.type=TSNodeObjectTypeInt;
                        result.value.int64=obj1.value.int64-obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.type=TSNodeObjectTypeUInt;
                        result.value.uint64=obj1.value.int64-obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.type=TSNodeObjectTypeDouble;
                        result.value.double64=obj1.value.int64-obj2.value.double64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;
            
            case TSNodeObjectTypeUInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.type=TSNodeObjectTypeUInt;
                        result.value.uint64=obj1.value.uint64-obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.type=TSNodeObjectTypeUInt;
                        result.value.uint64=obj1.value.uint64-obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.type=TSNodeObjectTypeDouble;
                        result.value.double64=obj1.value.uint64-obj2.value.double64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypeDouble:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.type=TSNodeObjectTypeDouble;
                        result.value.double64=obj1.value.double64-obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.type=TSNodeObjectTypeDouble;
                        result.value.double64=obj1.value.double64-obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.type=TSNodeObjectTypeDouble;
                        result.value.double64=obj1.value.double64-obj2.value.double64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;
            
            default:
                TS_PRINTF_ERROR("Unknown type in subtraction: %d\n", obj1.type);
        }
    }
    else if (strcmp(op,"*")==0) {
        switch (obj1.type) {
            case TSNodeObjectTypeInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.type=TSNodeObjectTypeInt;
                        result.value.int64=obj1.value.int64*obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.type=TSNodeObjectTypeUInt;
                        result.value.uint64=obj1.value.int64*obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.type=TSNodeObjectTypeDouble;
                        result.value.double64=obj1.value.int64*obj2.value.double64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;
            
            case TSNodeObjectTypeUInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.type=TSNodeObjectTypeUInt;
                        result.value.uint64=obj1.value.uint64*obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.type=TSNodeObjectTypeUInt;
                        result.value.uint64=obj1.value.uint64*obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.type=TSNodeObjectTypeDouble;
                        result.value.double64=obj1.value.uint64*obj2.value.double64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypeDouble:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.type=TSNodeObjectTypeDouble;
                        result.value.double64=obj1.value.double64*obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.type=TSNodeObjectTypeDouble;
                        result.value.double64=obj1.value.double64*obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.type=TSNodeObjectTypeDouble;
                        result.value.double64=obj1.value.double64*obj2.value.double64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;
            
            default:
                TS_PRINTF_ERROR("Unknown type in multiplication: %d\n", obj1.type);
        }
    }
    else if (strcmp(op,"/")==0) {
        switch (obj1.type) {
            case TSNodeObjectTypeInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.type=TSNodeObjectTypeInt;
                        result.value.int64=obj1.value.int64/obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.type=TSNodeObjectTypeUInt;
                        result.value.uint64=obj1.value.int64/obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.type=TSNodeObjectTypeDouble;
                        result.value.double64=obj1.value.int64/obj2.value.double64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypeUInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.type=TSNodeObjectTypeUInt;
                        result.value.uint64=obj1.value.uint64/obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.type=TSNodeObjectTypeUInt;
                        result.value.uint64=obj1.value.uint64/obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.type=TSNodeObjectTypeDouble;
                        result.value.double64=obj1.value.uint64/obj2.value.double64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypeDouble:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.type=TSNodeObjectTypeDouble;
                        result.value.double64=obj1.value.double64/obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.type=TSNodeObjectTypeDouble;
                        result.value.double64=obj1.value.double64/obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.type=TSNodeObjectTypeDouble;
                        result.value.double64=obj1.value.double64/obj2.value.double64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            default:
                TS_PRINTF_ERROR("Unknown type in division: %d\n", obj1.type);
        }
    }
    else if (strcmp(op,"%%")==0) {
        switch (obj1.type) {
            case TSNodeObjectTypeInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.type=TSNodeObjectTypeInt;
                        result.value.int64=obj1.value.int64%obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.type=TSNodeObjectTypeUInt;
                        result.value.uint64=obj1.value.int64%obj2.value.uint64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypeUInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.type=TSNodeObjectTypeUInt;
                        result.value.uint64=obj1.value.uint64%obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.type=TSNodeObjectTypeUInt;
                        result.value.uint64=obj1.value.uint64%obj2.value.uint64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            default:
                TS_PRINTF_ERROR("Unknown type in modulus: %d\n", obj1.type);
        }
    }

    /* Comparison */
    else if (strcmp(op,"==")==0) {
        result.size=sizeof(int);
        result.type=TSNodeObjectTypeInt;
        switch (obj1.type) {
            case TSNodeObjectTypeInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.int64==obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.int64==(int64_t)obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.int64==obj2.value.double64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.int64 == (int64_t)obj2.value.pointer;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypeUInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=(int64_t)obj1.value.uint64==obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.uint64==obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.uint64==obj2.value.double64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.uint64 == (uint64_t)obj2.value.pointer;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypeDouble:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.double64==obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.double64==obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.double64==obj2.value.double64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypePointer:
                switch (obj2.type) {
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.pointer==obj2.value.pointer;
                        break;
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.pointer==NULL;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.pointer==NULL;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            default:
                TS_PRINTF_ERROR("Unknown type in equality: %d\n", obj1.type);
        }
    }
    else if (strcmp(op,"!=")==0) {
        result.size=sizeof(int);
        result.type=TSNodeObjectTypeInt;
        switch (obj1.type) {
            case TSNodeObjectTypeInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.int64!=obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.int64!=(int64_t)obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.int64!=obj2.value.double64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.int64 != (int64_t)obj2.value.pointer;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypeUInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=(int64_t)obj1.value.uint64!=obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.uint64!=obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.uint64!=obj2.value.double64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.uint64 != (uint64_t)obj2.value.pointer;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypeDouble:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.double64!=obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.double64!=obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.double64!=obj2.value.double64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypePointer:
                switch (obj2.type) {
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.pointer!=obj2.value.pointer;
                        break;
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.pointer!=NULL;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.pointer!=NULL;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            default:
                TS_PRINTF_ERROR("Unknown type in inequality: %d\n", obj1.type);
        }
    }
    else if (strcmp(op,"<")==0) {
        result.size=sizeof(int);
        result.type=TSNodeObjectTypeInt;
        switch (obj1.type) {
            case TSNodeObjectTypeInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.int64<obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.int64<(int64_t)obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.int64<obj2.value.double64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.int64 < (int64_t)obj2.value.pointer;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypeUInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=(int64_t)obj1.value.uint64<obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.uint64<obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.uint64<obj2.value.double64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.uint64 < (uint64_t)obj2.value.pointer;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypeDouble:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.double64<obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.double64<obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.double64<obj2.value.double64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypePointer:
                switch (obj2.type) {
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.pointer<obj2.value.pointer;
                        break;
                    case TSNodeObjectTypeInt:
                        result.value.int64=(int64_t)obj1.value.pointer<(int64_t)NULL;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=(int64_t)obj1.value.pointer<(int64_t)NULL;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            default:
                TS_PRINTF_ERROR("Unknown type in less than: %d\n", obj1.type);
        }
    }
    else if (strcmp(op,">")==0) {
        result.size=sizeof(int);
        result.type=TSNodeObjectTypeInt;
        switch (obj1.type) {
            case TSNodeObjectTypeInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.int64>obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.int64>(int64_t)obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.int64>obj2.value.double64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.int64 > (int64_t)obj2.value.pointer;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;
    
            case TSNodeObjectTypeUInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=(int64_t)obj1.value.uint64>obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.uint64>obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.uint64>obj2.value.double64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.uint64 > (uint64_t)obj2.value.pointer;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypeDouble:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.double64>obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.double64>obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.double64>obj2.value.double64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypePointer:
                switch (obj2.type) {
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.pointer>obj2.value.pointer;
                        break;
                    case TSNodeObjectTypeInt:
                        result.value.int64=(int64_t)obj1.value.pointer>(int64_t)NULL;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=(int64_t)obj1.value.pointer>(int64_t)NULL;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            default:
                TS_PRINTF_ERROR("Unknown type in greater than: %d\n", obj1.type);
        }
    }
    else if (strcmp(op,"<=")==0) {
        result.size=sizeof(int);
        result.type=TSNodeObjectTypeInt;
        switch (obj1.type) {
            case TSNodeObjectTypeInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.int64<=obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.int64<=(int64_t)obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.int64<=obj2.value.double64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.int64 <= (int64_t)obj2.value.pointer;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypeUInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=(int64_t)obj1.value.uint64<=obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.uint64<=obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.uint64<=obj2.value.double64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.uint64 <= (uint64_t)obj2.value.pointer;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypeDouble:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.double64<=obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.double64<=obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.double64<=obj2.value.double64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypePointer:
                switch (obj2.type) {
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.pointer<=obj2.value.pointer;
                        break;
                    case TSNodeObjectTypeInt:
                        result.value.int64=(int64_t)obj1.value.pointer<=(int64_t)NULL;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=(int64_t)obj1.value.pointer<=(int64_t)NULL;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            default:
                TS_PRINTF_ERROR("Unknown type in less than or equal: %d\n", obj1.type);
        }
    }
    else if (strcmp(op,">=")==0) {
        result.size=sizeof(int);
        result.type=TSNodeObjectTypeInt;
        switch (obj1.type) {
            case TSNodeObjectTypeInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.int64>=obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.int64>=(int64_t)obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.int64>=obj2.value.double64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.int64 >= (int64_t)obj2.value.pointer;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypeUInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=(int64_t)obj1.value.uint64>=obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.uint64>=obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.uint64>=obj2.value.double64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.uint64 >= (uint64_t)obj2.value.pointer;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypeDouble:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.double64>=obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.double64>=obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.double64>=obj2.value.double64;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            case TSNodeObjectTypePointer:
                switch (obj2.type) {
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.pointer>=obj2.value.pointer;
                        break;
                    case TSNodeObjectTypeInt:
                        result.value.int64=(int64_t)obj1.value.pointer>=(int64_t)NULL;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=(int64_t)obj1.value.pointer>=(int64_t)NULL;
                        break;
                    default:
                        TS_PRINTF_ERROR("Unknown type: %d\n", obj2.type);
                }
                break;

            default:
                TS_PRINTF_ERROR("Unknown type in greater than or equal: %d\n", obj1.type);
        }
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

    else {
        TS_PRINTF_ERROR("Unknown operator: %s\n", op);
    }

    return result;
}

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
    TSNodeObject right_value = ts_interpreter_simulate(right, var_count, vars);
    switch (left_obj.type) {
        case TSNodeObjectTypeInt:
            switch (left_obj.size) {
                case 1:
                    *(int8_t*)left_obj.reference = (int8_t)right_value.value.int64;
                    break;
                case 2:
                    *(int16_t*)left_obj.reference = (int16_t)right_value.value.int64;
                    break;
                case 4:
                    *(int32_t*)left_obj.reference = (int32_t)right_value.value.int64;
                    break;
                case 8:
                    *(int64_t*)left_obj.reference = (int64_t)right_value.value.int64;
                    break;
                default:
                    TS_PRINTF_ERROR("size of LHS of assignment %" PRIu64, left_obj.size);
            }
            break;
        case TSNodeObjectTypeUInt:
            switch (left_obj.size) {
                case 1:
                    *(uint8_t*)left_obj.reference = (uint8_t)right_value.value.uint64;
                    break;
                case 2:
                    *(uint16_t*)left_obj.reference = (uint16_t)right_value.value.uint64;
                    break;
                case 4:
                    *(uint32_t*)left_obj.reference = (uint32_t)right_value.value.uint64;
                    break;
                case 8:
                    *(uint64_t*)left_obj.reference = (uint64_t)right_value.value.uint64;
                    break;
                default:
                    TS_PRINTF_ERROR("size of LHS of assignment %" PRIu64, left_obj.size);
            }
            break;
        case TSNodeObjectTypePointer:
            *(void**)left_obj.reference = right_value.value.pointer;
            break;
        default:
            TS_PRINTF_ERROR("Unsupported LHS type in assignment: %" PRIu32 "\n", left_obj.type);
    }
    return left_obj;
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