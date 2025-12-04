#include "tree_sitter/api.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

TSNodeObject ts_interpreter_variable(TSNode node, uint64_t var_count, TSNodeObject* vars) {
    char* node_name=ts_node_find_value(node);
    for (size_t i=0;i<var_count;i++) {
        if (strcmp(node_name, vars[i].name)==0) {
            return vars[i];
        }
    }

    fprintf(stderr, "Variable not found: %s\n", node_name);
    assert(0);
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
                        result.value.int64=obj1.value.int64==(int64_t)obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.int64==obj2.value.double64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.int64 == (int64_t)obj2.value.pointer;
                        break;
                    default:
                        assert(0 && "Unknown type");
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
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.pointer==NULL;
                        break;
                    default:
                        assert(0 && "Unknown type");
                }
                break;

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
                        result.value.int64=obj1.value.int64!=(int64_t)obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.int64!=obj2.value.double64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.int64 != (int64_t)obj2.value.pointer;
                        break;
                    default:
                        assert(0 && "Unknown type");
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
                    case TSNodeObjectTypeUInt:
                        result.value.int64=obj1.value.pointer!=NULL;
                        break;
                    default:
                        assert(0 && "Unknown type");
                }
                break;

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
                        result.value.int64=obj1.value.int64<(int64_t)obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.int64<obj2.value.double64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.int64 < (int64_t)obj2.value.pointer;
                        break;
                    default:
                        assert(0 && "Unknown type");
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
                        result.value.int64=obj1.value.int64>(int64_t)obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.int64>obj2.value.double64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.int64 > (int64_t)obj2.value.pointer;
                        break;
                    default:
                        assert(0 && "Unknown type");
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
                        result.value.int64=obj1.value.int64<=(int64_t)obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.int64<=obj2.value.double64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.int64 <= (int64_t)obj2.value.pointer;
                        break;
                    default:
                        assert(0 && "Unknown type");
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
                        result.value.int64=obj1.value.int64>=(int64_t)obj2.value.uint64;
                        break;
                    case TSNodeObjectTypeDouble:
                        result.value.int64=obj1.value.int64>=obj2.value.double64;
                        break;
                    case TSNodeObjectTypePointer:
                        result.value.int64=obj1.value.int64 >= (int64_t)obj2.value.pointer;
                        break;
                    default:
                        assert(0 && "Unknown type");
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

TSNodeObject ts_interpreter_function(TSNode node, uint64_t var_count, TSNodeObject* vars) {
    assert(strcmp(ts_node_type(node),"call_expression")==0 && "Node must be a call_expression");
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
    assert(exists && "Function not found in variables");

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
            fprintf(stderr, "Unknown function type\n");
            abort();
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
                            fprintf(stderr, "Unsupported argument type for void function");
                            abort();
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
                                    fprintf(stderr, "Unsupported argument type for void function");
                                    abort();
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
                                    fprintf(stderr, "Unsupported argument type for void function");
                                    abort();
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
                                    fprintf(stderr, "Unsupported argument type for void function");
                                    abort();
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
                                    fprintf(stderr, "Unsupported argument type for void function");
                                    abort();
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
                                    fprintf(stderr, "Unsupported argument type for void function");
                                    abort();
                            }
                            break;
                        default:
                            fprintf(stderr, "Unsupported argument type for void function");
                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
                                    }
                                    break;
                                default:
                                    fprintf(stderr, "Unsupported argument type for void function");
                                    abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
                                    }
                                    break;
                                default:
                                    fprintf(stderr, "Unsupported argument type for void function");
                                    abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
                                    }
                                    break;
                                default:
                                    fprintf(stderr, "Unsupported argument type for void function");
                                    abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
                                    }
                                    break;
                                default:
                                    fprintf(stderr, "Unsupported argument type for void function");
                                    abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for void function");
                                            abort();
                                    }
                                    break;
                                default:
                                    fprintf(stderr, "Unsupported argument type for void function");
                                    abort();
                            }
                            break;
                        default:
                            fprintf(stderr, "Unsupported argument type for void function");
                            abort();
                    }
                    break;
                default:
                    assert(0 && "Unsupported number of arguments for void function");
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
                            fprintf(stderr, "Unsupported argument type for int function");
                            abort();
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
                                    fprintf(stderr, "Unsupported argument type for int function");
                                    abort();
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
                                    fprintf(stderr, "Unsupported argument type for int function");
                                    abort();
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
                                    fprintf(stderr, "Unsupported argument type for int function");
                                    abort();
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
                                    fprintf(stderr, "Unsupported argument type for int function");
                                    abort();
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
                                    fprintf(stderr, "Unsupported argument type for int function");
                                    abort();
                            }
                            break;
                        default:
                            fprintf(stderr, "Unsupported argument type for int function");
                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
                                    }
                                    break;
                                default:
                                    fprintf(stderr, "Unsupported argument type for int function");
                                    abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
                                    }
                                    break;
                                default:
                                    fprintf(stderr, "Unsupported argument type for int function");
                                    abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
                                    }
                                    break;
                                default:
                                    fprintf(stderr, "Unsupported argument type for int function");
                                    abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
                                    }
                                    break;
                                default:
                                    fprintf(stderr, "Unsupported argument type for int function");
                                    abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for int function");
                                            abort();
                                    }
                                    break;
                                default:
                                    fprintf(stderr, "Unsupported argument type for int function");
                                    abort();
                            }
                            break;
                        default:
                            fprintf(stderr, "Unsupported argument type for int function");
                            abort();
                    }
                    break;
                default:
                    assert(0 && "Unsupported number of arguments for int function");
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
                            fprintf(stderr, "Unsupported argument type for uint function");
                            abort();
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
                                    fprintf(stderr, "Unsupported argument type for uint function");
                                    abort();
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
                                    fprintf(stderr, "Unsupported argument type for uint function");
                                    abort();
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
                                    fprintf(stderr, "Unsupported argument type for uint function");
                                    abort();
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
                                    fprintf(stderr, "Unsupported argument type for uint function");
                                    abort();
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
                                    fprintf(stderr, "Unsupported argument type for uint function");
                                    abort();
                            }
                            break;
                        default:
                            fprintf(stderr, "Unsupported argument type for uint function");
                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
                                    }
                                    break;
                                default:
                                    fprintf(stderr, "Unsupported argument type for uint function");
                                    abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
                                    }
                                    break;
                                default:
                                    fprintf(stderr, "Unsupported argument type for uint function");
                                    abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
                                    }
                                    break;
                                default:
                                    fprintf(stderr, "Unsupported argument type for uint function");
                                    abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
                                    }
                                    break;
                                default:
                                    fprintf(stderr, "Unsupported argument type for uint function");
                                    abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for uint function");
                                            abort();
                                    }
                                    break;
                                default:
                                    fprintf(stderr, "Unsupported argument type for uint function");
                                    abort();
                            }
                            break;
                        default:
                            fprintf(stderr, "Unsupported argument type for uint function");
                            abort();
                    }
                    break;
                default:
                    assert(0 && "Unsupported number of arguments for uint function");
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
                            fprintf(stderr, "Unsupported argument type for pointer function");
                            abort();
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
                                    fprintf(stderr, "Unsupported argument type for pointer function");
                                    abort();
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
                                    fprintf(stderr, "Unsupported argument type for pointer function");
                                    abort();
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
                                    fprintf(stderr, "Unsupported argument type for pointer function");
                                    abort();
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
                                    fprintf(stderr, "Unsupported argument type for pointer function");
                                    abort();
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
                                    fprintf(stderr, "Unsupported argument type for pointer function");
                                    abort();
                            }
                            break;
                        default:
                            fprintf(stderr, "Unsupported argument type for pointer function");
                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for pointer function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for pointer function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for pointer function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for pointer function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for pointer function");
                                            abort();
                                    }
                                    break;
                                default:
                                    fprintf(stderr, "Unsupported argument type for pointer function");
                                    abort();
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
                                            fprintf(stderr, "Unsupported argument type for pointer function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for pointer function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for pointer function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for pointer function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for pointer function");
                                            abort();
                                    }
                                    break;
                                default:
                                    fprintf(stderr, "Unsupported argument type for pointer function");
                                    abort();
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
                                            fprintf(stderr, "Unsupported argument type for pointer function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for pointer function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for pointer function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for pointer function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for pointer function");
                                            abort();
                                    }
                                    break;
                                default:
                                    fprintf(stderr, "Unsupported argument type for pointer function");
                                    abort();
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
                                            fprintf(stderr, "Unsupported argument type for pointer function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for pointer function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for pointer function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for pointer function");
                                            abort();
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
                                            fprintf(stderr, "Unsupported argument type for pointer function");
                                            abort();
                                    }
                                    break;
                                default:
                                    fprintf(stderr, "Unsupported argument type for pointer function");
                                    abort();
                            }
                            break;
                        default:
                            fprintf(stderr, "Unsupported argument type for pointer function");
                            abort();
                    }
                    break;
                default:
                    assert(0 && "Unsupported number of arguments for pointer function");
            }
            obj.value.pointer = return_value;
            break;
        }
        default:
            assert(0 && "Unknown function type during call");
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
        assert(0 && "Unknown node type");
    }
}