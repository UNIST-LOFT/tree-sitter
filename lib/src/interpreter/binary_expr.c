#include "tree_sitter/api.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

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
