#include "tree_sitter/api.h"
#include <string.h>
#include <inttypes.h>

/* Single definition of the globals declared `extern` in api.h. These are
 * shared by every translation unit that includes the header. */
TSNodeObject new_variables[MAX_VARS];
uint32_t new_var_count = 0;




TSNodeObject ts_interpreter_var_decl(TSNode node, uint64_t var_count, TSNodeObject* vars, TSTypeInfo* type_info_table) {
    /*
        The (category, width) of the declared type, and of what it points at when it is a pointer. A
        type of the program resolves through the type info, so a declaration of one of its typedefs or
        records works the same as one of a built-in type. The value of a declaration node already
        carries the "*" of a pointer declarator, see ts_add_value().
    */
    const char* type_text = ts_node_find_value(node);
    TSTypeInfo decl_type_info;
    TSTypeInfo element_type_info;
    if (!ts_interpreter_resolve_type(type_text, type_info_table, &decl_type_info, &element_type_info)) {
        TS_PRINTF_ERROR("Unsupported variable declaration type: %s\n",
                        type_text ? type_text : "(null)");
    }
    TSNodeObjectType decl_type = decl_type_info.category;
    uint64_t decl_size = decl_type_info.size;

    TSNode rhs = ts_node_child_by_field_name(node, "declarator", strlen("declarator"));
    if (ts_node_is_null(rhs)) {
        // ts_node_type() below would read the type of a node that does not exist
        TS_PRINTF_ERROR("Variable declaration without a declarator: %s\n",
                        type_text ? type_text : "(null)");
    }
    TSNodeObject new_var = {0};
    if (strcmp(ts_node_type(rhs), "identifier") == 0) {
        // primitive var without init
        char* ident_name = ts_node_find_value(rhs);
        if (ident_name == NULL) {
            TS_PRINTF_ERROR("Failed to retrieve identifier name for variable declaration.\n");
        }

        new_var.name = malloc(strlen(ident_name) + 1);
        strcpy(new_var.name, ident_name);
        new_var.type = decl_type_info;
        new_var.node = rhs;
        void* ref = malloc(decl_size);
        memset(ref, 0, decl_size); // No init: initialize to zero
        new_var.reference = ref;

        /* A typedef of a pointer is declared through a plain identifier, e.g. `int_ptr p;`, so the
           category decides what the value is, not the shape of the declarator. A record is at its own
           address, the way ts_interpreter_field expects to find one. */
        new_var.array_element_type = element_type_info;
        if (decl_type == TSNodeObjectTypeInt) {
            new_var.value.int64 = 0;
        } else if (decl_type == TSNodeObjectTypeUInt) {
            new_var.value.uint64 = 0;
        } else if (decl_type == TSNodeObjectTypeDouble) {
            new_var.value.double64 = 0.0;
        } else if (decl_type == TSNodeObjectTypePointer) {
            new_var.value.pointer = NULL;
        } else if (decl_type == TSNodeObjectTypeStruct) {
            new_var.value.pointer = ref;
        } else {
            TS_PRINTF_ERROR("Unsupported variable type for declaration: %d\n", decl_type);
        }
    }
    else if (strcmp(ts_node_type(rhs), "pointer_declarator") == 0) {
        // pointer var without init
        TSNode inner_ident = ts_node_named_child(rhs, 0);
        char* ident_name = NULL;
        if (strcmp(ts_node_type(inner_ident), "identifier") == 0) {
            ident_name = ts_node_find_value(inner_ident);
        }
        if (ident_name == NULL) {
            TS_PRINTF_ERROR("Failed to retrieve identifier name for variable declaration.\n");
        }

        new_var.name = malloc(strlen(ident_name) + 1);
        strcpy(new_var.name, ident_name);
        new_var.type = decl_type_info;
        // Element type of the pointer is what it points at, so that it can be subscripted and stepped
        new_var.array_element_type = element_type_info;
        new_var.node = rhs;
        void* ref = malloc(decl_size);
        memset(ref, 0, decl_size); // No init: initialize to zero
        new_var.reference = ref;

        if (decl_type == TSNodeObjectTypePointer) {
            new_var.value.pointer = NULL;
        } else {
            TS_PRINTF_ERROR("Unsupported variable type for pointer declaration: %d\n", decl_type);
        }
    }
    else if (strcmp(ts_node_type(rhs), "init_declarator") == 0) {
        // primitive var with init
        TSNode name_node = ts_node_named_child(rhs, 0);
        TSNode value_node = ts_node_named_child(rhs, 1);
        char* ident_name = NULL;
        if (strcmp(ts_node_type(name_node), "identifier") == 0) {
            ident_name = ts_node_find_value(name_node);
        } else if (strcmp(ts_node_type(name_node), "pointer_declarator") == 0) {
            TSNode inner_ident = ts_node_named_child(name_node, 0);
            if (strcmp(ts_node_type(inner_ident), "identifier") == 0) {
                ident_name = ts_node_find_value(inner_ident);
            }
        }
        if (ident_name == NULL) {
            TS_PRINTF_ERROR("Failed to retrieve identifier name for variable declaration.\n");
        }

        new_var.name = malloc(strlen(ident_name) + 1);
        strcpy(new_var.name, ident_name);
        new_var.type = decl_type_info;
        // element_type_info is what the pointer points at, and unknown when this is not a pointer
        new_var.array_element_type = element_type_info;
        new_var.node = rhs;
        void* ref = malloc(decl_size);
        memset(ref, 0, decl_size);
        TSNodeObject init = ts_interpreter_simulate(value_node, var_count, vars, type_info_table); // Initializer
        /* The declared width, not eight bytes: the storage is malloc(decl_size), so a wider store
         * would run past a `char` or `int` variable's own allocation. Reads take the same width
         * (see ts_interpreter_load_variable). */
        if (init.type.category == TSNodeObjectTypeInt || init.type.category == TSNodeObjectTypeUInt ||
                init.type.category == TSNodeObjectTypeDouble) {
            long double number = (init.type.category == TSNodeObjectTypeInt)
                    ? (long double)init.value.int64
                    : (init.type.category == TSNodeObjectTypeUInt) ? (long double)init.value.uint64
                                                                   : init.value.double64;
            switch (decl_type) {
                case TSNodeObjectTypeInt:
                    switch (decl_size) {
                        case 1: *(int8_t*)ref = (int8_t)number; break;
                        case 2: *(int16_t*)ref = (int16_t)number; break;
                        case 4: *(int32_t*)ref = (int32_t)number; break;
                        case 8: *(int64_t*)ref = (int64_t)number; break;
                        default: TS_PRINTF_ERROR("Unsupported int size in declaration: %" PRIu64 "\n", decl_size);
                    }
                    break;
                case TSNodeObjectTypeUInt:
                    switch (decl_size) {
                        case 1: *(uint8_t*)ref = (uint8_t)number; break;
                        case 2: *(uint16_t*)ref = (uint16_t)number; break;
                        case 4: *(uint32_t*)ref = (uint32_t)number; break;
                        case 8: *(uint64_t*)ref = (uint64_t)number; break;
                        default: TS_PRINTF_ERROR("Unsupported uint size in declaration: %" PRIu64 "\n", decl_size);
                    }
                    break;
                case TSNodeObjectTypeDouble:
                    switch (decl_size) {
                        case 4: *(float*)ref = (float)number; break;
                        case 8: *(double*)ref = (double)number; break;
                        case 16: *(long double*)ref = number; break;
                        default: TS_PRINTF_ERROR("Unsupported double size in declaration: %" PRIu64 "\n", decl_size);
                    }
                    break;
                default:
                    TS_PRINTF_ERROR("Unsupported declared type for a number initializer.\n");
            }
        } else if (init.type.category == TSNodeObjectTypePointer) {
            *(void**)ref = init.value.pointer;
        } else if (init.type.category == TSNodeObjectTypeStruct) {
            /* A record is copied whole, as far as both sides have room for: its value is its address,
               so there is no number to convert */
            uint64_t copied = (decl_size < init.type.size) ? decl_size : init.type.size;
            if (init.reference != NULL && copied > 0) {
                memcpy(ref, init.reference, copied);
            }
        } else if (init.type.category == TSNodeObjectTypeString) {
            // String literal initializer
            *(char**)ref = init.value.pointer;
        } else {
            TS_PRINTF_ERROR("Unsupported initializer type for variable declaration: %d\n", init.type.category);
        }
        new_var.reference = ref;

        if (decl_type == TSNodeObjectTypeInt) {
            new_var.value.int64 = init.value.int64;
        } else if (decl_type == TSNodeObjectTypeUInt) {
            new_var.value.uint64 = init.value.uint64;
        } else if (decl_type == TSNodeObjectTypeDouble) {
            new_var.value.double64 = init.value.double64;
        } else if (decl_type == TSNodeObjectTypePointer) {
            new_var.value.pointer = init.value.pointer;
        } else if (decl_type == TSNodeObjectTypeStruct) {
            new_var.value.pointer = ref; // A record is at its own address
        }
    }

    for (size_t i = 0; i < new_var_count; i++) {
        if (strcmp(new_var.name, new_variables[i].name) == 0) {
            // Overwrite
            free((void*)new_variables[i].reference);
            new_variables[i] = new_var;
            return new_var;
        }
    }

    if (new_var_count >= MAX_VARS) {
        TS_PRINTF_ERROR("Exceeded maximum number of new variables: %d\n", MAX_VARS);
    }
    new_variables[new_var_count++] = new_var;
    return new_var;
}