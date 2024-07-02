#include "tree_sitter/api.h"
#include <assert.h>
#include <string.h>

TSNodeObject ts_interpreter_variable(TSNode node, uint64_t var_count, TSNodeObject* vars) {
    char* node_name=ts_node_find_value(node);
    for (size_t i=0;i<var_count;i++) {
        if (strcmp(node_name, vars[i].name)==0) {
            return vars[i];
        }
    }

    assert(0 && "Variable not found");
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

TSNodeObject ts_interpreter_literal(TSNode node, uint64_t var_count, TSNodeObject* vars) {
    TSNodeObject obj;
    obj.name=ts_node_find_value(node);
    obj.node=node;

    if (in_str(obj.name,'.')) {
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
            assert(0 && "Unknown unsigned type");
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
        assert(0 && "Dereference operation not supported");
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
                assert(0 && "Unknown type");
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
                assert(0 && "Unknown type");
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
                assert(0 && "Unknown type");
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
                assert(0 && "Unknown type");
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
                assert(0 && "Unknown type");
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
                assert(0 && "Unknown type");
        }
    }
    else {
        assert(0 && "Unknown unary operator");
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
                        assert(0 && "Unknown type");
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
                        assert(0 && "Unknown type");
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
                        assert(0 && "Unknown type");
                }
                break;
            
            default:
                assert(0 && "Unknown type in addition");
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
                        assert(0 && "Unknown type");
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
                        assert(0 && "Unknown type");
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
                        assert(0 && "Unknown type");
                }
                break;
            
            default:
                assert(0 && "Unknown type in subtraction");
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
                        assert(0 && "Unknown type");
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
                        assert(0 && "Unknown type");
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
                        assert(0 && "Unknown type");
                }
                break;
            
            default:
                assert(0 && "Unknown type in multiplication");
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
                        assert(0 && "Unknown type");
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
                        assert(0 && "Unknown type");
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
                        assert(0 && "Unknown type");
                }
                break;

            default:
                assert(0 && "Unknown type in division");
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
                        assert(0 && "Unknown type");
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
                        assert(0 && "Unknown type");
                }
                break;

            default:
                assert(0 && "Unknown type in modulo");
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
                        result.value.int64=obj1.value.int64==obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.int64==obj2.value.double64;
                        break;
                    default:
                        assert(0 && "Unknown type");
                }
                break;

            case TSNodeObjectTypeUInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.uint64==obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.uint64==obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.uint64==obj2.value.double64;
                        break;
                    default:
                        assert(0 && "Unknown type");
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
                        assert(0 && "Unknown type");
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
                    default:
                        assert(0 && "Unknown type");
                }

            default:
                assert(0 && "Unknown type in equality");
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
                        result.value.int64=obj1.value.int64!=obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.int64!=obj2.value.double64;
                        break;
                    default:
                        assert(0 && "Unknown type");
                }
                break;

            case TSNodeObjectTypeUInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.uint64!=obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.uint64!=obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.uint64!=obj2.value.double64;
                        break;
                    default:
                        assert(0 && "Unknown type");
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
                        assert(0 && "Unknown type");
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
                    default:
                        assert(0 && "Unknown type");
                }

            default:
                assert(0 && "Unknown type in inequality");
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
                        result.value.int64=obj1.value.int64<obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.int64<obj2.value.double64;
                        break;
                    default:
                        assert(0 && "Unknown type");
                }
                break;

            case TSNodeObjectTypeUInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.uint64<obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.uint64<obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.uint64<obj2.value.double64;
                        break;
                    default:
                        assert(0 && "Unknown type");
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
                        assert(0 && "Unknown type");
                }
                break;

            default:
                assert(0 && "Unknown type in less than");
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
                        result.value.int64=obj1.value.int64>obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.int64>obj2.value.double64;
                        break;
                    default:
                        assert(0 && "Unknown type");
                }
                break;
    
                case TSNodeObjectTypeUInt:
                    switch (obj2.type) {
                        case TSNodeObjectTypeInt:
                            result.value.int64=obj1.value.uint64>obj2.value.int64;
                            break;
                        case TSNodeObjectTypeUInt:
                            result.value.int64=obj1.value.uint64>obj2.value.uint64;
                            break;
                        case TSNodeObjectTypeDouble:
                            result.value.int64=obj1.value.uint64>obj2.value.double64;
                            break;
                        default:
                            assert(0 && "Unknown type");
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
                            assert(0 && "Unknown type");
                    }
                    break;

                default:
                    assert(0 && "Unknown type in greater than");
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
                        result.value.int64=obj1.value.int64<=obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.int64<=obj2.value.double64;
                        break;
                    default:
                        assert(0 && "Unknown type");
                }
                break;

            case TSNodeObjectTypeUInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.uint64<=obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.uint64<=obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.uint64<=obj2.value.double64;
                        break;
                    default:
                        assert(0 && "Unknown type");
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
                        assert(0 && "Unknown type");
                }
                break;

            default:
                assert(0 && "Unknown type in less than or equal");
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
                        result.value.int64=obj1.value.int64>=obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.int64>=obj2.value.double64;
                        break;
                    default:
                        assert(0 && "Unknown type");
                }
                break;

            case TSNodeObjectTypeUInt:
                switch (obj2.type) {
                    case TSNodeObjectTypeInt:
                        result.value.int64=obj1.value.uint64>=obj2.value.int64;
                        break;
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.uint64>=obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.uint64>=obj2.value.double64;
                        break;
                    default:
                        assert(0 && "Unknown type");
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
                        assert(0 && "Unknown type");
                }
                break;

            default:
                assert(0 && "Unknown type in greater than or equal");
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
                        assert(0 && "Unknown type in logical and");
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
                        assert(0 && "Unknown type in logical and");
                }
                break;
            default:
                assert(0 && "Unknown type in logical and");
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
                        assert(0 && "Unknown type in logical or");
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
                        assert(0 && "Unknown type in logical or");
                }
                break;
            default:
                assert(0 && "Unknown type in logical or");
        }
    }

    else {
        assert(0 && "Unknown binary operator");
    }

    return result;
}

TSNodeObject ts_interpreter_simulate(TSNode node, uint64_t var_count, TSNodeObject* vars) {
    if (strcmp(ts_node_type(node),"identifier")==0 || strcmp(ts_node_type(node),"field_expression")==0) {
        return ts_interpreter_variable(node,var_count,vars);
    }
    else if (strcmp(ts_node_type(node),"number_literal")==0) {
        return ts_interpreter_literal(node,var_count,vars);
    }
    else if (strcmp(ts_node_type(node),"unary_expression")==0) {
        return ts_interpreter_unary(node,var_count,vars);
    }
    else if (strcmp(ts_node_type(node),"binary_expression")==0) {
        return ts_interpreter_binary(node,var_count,vars);
    }
    else if (strcmp(ts_node_type(node),"string_literal")==0) {
        TSNodeObject obj;
        obj.name=ts_node_find_value(node);
        obj.node=node;
        obj.size=sizeof(char)*(strlen(ts_node_find_value(node))-2); // Remove quotes
        obj.type=TSNodeObjectTypeString;
        return obj;
    }
    else if (strcmp(ts_node_type(node),"parenthesized_expression")==0 ||
            strcmp(ts_node_type(node),"expression_statement")==0 ||
            strcmp(ts_node_type(node),"ERROR")==0) {
        return ts_interpreter_simulate(ts_node_named_child(node,0),var_count,vars);
    }
    else {
        assert(0 && "Unknown node type");
    }
}