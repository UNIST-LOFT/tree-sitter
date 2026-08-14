#include "tree_sitter/api.h"
#include <string.h>
#include <inttypes.h>

/* Single definition of the globals declared `extern` in api.h. These are
 * shared by every translation unit that includes the header. */
TSNodeObject new_variables[MAX_VARS];
uint32_t new_var_count = 0;

/* Maps concrete C type spellings to the interpreter's (type, byte-width)
 * representation. This mirrors the _CAST_TYPE_WIDTH table in the Python front
 * end. Each entry lists every spelling that resolves to the same width; the
 * `names` array is NULL-terminated. */
typedef struct {
    TSNodeObjectType type;
    uint64_t size;
    const char* names[20];
} TSCastTypeWidth;

static const TSCastTypeWidth CAST_TYPE_WIDTH[] = {
    // signed integers
    {TSNodeObjectTypeInt, 1, {"int8_t", "__int8", "char", "signed char", "i8", "s8", NULL}},
    {TSNodeObjectTypeInt, 2, {"int16_t", "__int16", "short", "short int", "i16", "s16",
                              "signed short", "signed short int", NULL}},
    {TSNodeObjectTypeInt, 4, {"int32_t", "__int32", "int", "signed", "signed int",
                              "i32", "s32", "wchar_t", NULL}},
    {TSNodeObjectTypeInt, 8, {"int64_t", "__int64", "long", "long int",
                              "signed long", "signed long int", "long long",
                              "long long int", "signed long long",
                              "i64", "s64",
                              "signed long long int", "ssize_t", "ptrdiff_t",
                              "intptr_t", "intmax_t", "off_t", "off64_t", NULL}},
    // unsigned integers
    {TSNodeObjectTypeUInt, 1, {"uint8_t", "u_int8_t", "unsigned char", "u8", NULL}},
    {TSNodeObjectTypeUInt, 2, {"uint16_t", "u_int16_t", "unsigned short", "u16",
                               "unsigned short int", NULL}},
    {TSNodeObjectTypeUInt, 4, {"uint32_t", "u_int32_t", "unsigned", "unsigned int", "u32",
                               "u_int", NULL}},
    {TSNodeObjectTypeUInt, 8, {"uint64_t", "u_int64_t", "size_t", "uintptr_t", "u64",
                               "uintmax_t", "unsigned long", "unsigned long int",
                               "unsigned long long", "unsigned long long int", NULL}},
    // floating point
    {TSNodeObjectTypeDouble, 4, {"float", NULL}},
    {TSNodeObjectTypeDouble, 8, {"double", "long double", NULL}},
    
    // pointer types
    {TSNodeObjectTypePointer, sizeof(void*), {"void*", "char*", "int*", "float*", "double*",
                                              "long*", "short*", "unsigned char*",
                                              "unsigned short*", "unsigned int*", "unsigned long*",
                                              "long long*", "unsigned long long*", NULL}},
};


/*
    The (category, width) of what a pointer declaration points at, from the declared type text:
    "char*" -> char, one byte. Looking the whole text up instead found the pointer entry again, which
    made every element eight bytes wide, so `p[i]`, `*p` and `p++` all stepped in units of a pointer.

    A pointee this table does not name -- `void*`, a struct pointer -- is treated as bytes, the way
    GNU C treats void pointer arithmetic; the declaration has no record info to do better, and it is
    the width the surrounding code steps by.
*/
static void ts_interpreter_pointee_type(const char* type_text, TSNodeObjectType* category,
                                        uint64_t* size) {
    *category = TSNodeObjectTypeInt;
    *size = 1;
    if (type_text == NULL) {
        return;
    }
    char pointee[TS_MAX_TYPE_NAME_SIZE];
    snprintf(pointee, sizeof(pointee), "%s", type_text);
    size_t length = strlen(pointee);
    while (length > 0 && (pointee[length - 1] == '*' || pointee[length - 1] == ' ')) {
        pointee[--length] = '\0';
    }
    for (size_t i = 0; i < sizeof(CAST_TYPE_WIDTH) / sizeof(CAST_TYPE_WIDTH[0]); i++) {
        for (size_t j = 0; CAST_TYPE_WIDTH[i].names[j] != NULL; j++) {
            if (strcmp(pointee, CAST_TYPE_WIDTH[i].names[j]) == 0) {
                *category = CAST_TYPE_WIDTH[i].type;
                *size = CAST_TYPE_WIDTH[i].size;
                return;
            }
        }
    }
}

TSNodeObject ts_interpreter_var_decl(TSNode node, uint64_t var_count, TSNodeObject* vars, TSTypeInfo* type_info_table) {
    // Check type is valid and decide the (type, width) of this declaration.
    const char* type_text = ts_node_find_value(node);
    TSNodeObjectType decl_type = TSNodeObjectTypeUnknown;
    uint64_t decl_size = 0;
    int type_found = 0;
    for (size_t i = 0; type_text != NULL && !type_found &&
                       i < sizeof(CAST_TYPE_WIDTH) / sizeof(CAST_TYPE_WIDTH[0]); i++) {
        for (size_t j = 0; CAST_TYPE_WIDTH[i].names[j] != NULL; j++) {
            if (strcmp(type_text, CAST_TYPE_WIDTH[i].names[j]) == 0) {
                decl_type = CAST_TYPE_WIDTH[i].type;
                decl_size = CAST_TYPE_WIDTH[i].size;
                type_found = 1;
                break;
            }
        }
    }
    if (!type_found) {
        TS_PRINTF_ERROR("Unsupported variable declaration type: %s\n",
                        type_text ? type_text : "(null)");
    }

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
        new_var.type = ts_interpreter_get_type_info(type_text, decl_size, decl_type);
        new_var.node = rhs;
        void* ref = malloc(decl_size);
        memset(ref, 0, decl_size); // No init: initialize to zero
        new_var.reference = ref;

        if (decl_type == TSNodeObjectTypeInt) {
            new_var.value.int64 = 0;
        } else if (decl_type == TSNodeObjectTypeUInt) {
            new_var.value.uint64 = 0;
        } else if (decl_type == TSNodeObjectTypeDouble) {
            new_var.value.double64 = 0.0;
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
        new_var.type = ts_interpreter_get_type_info(type_text, decl_size, decl_type);
        // Element type of the pointer is what it points at (see ts_interpreter_pointee_type)
        TSNodeObjectType elem_type;
        uint64_t elem_size;
        ts_interpreter_pointee_type(type_text, &elem_type, &elem_size);
        new_var.array_element_type.category = elem_type;
        new_var.array_element_type.size = elem_size;
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
        new_var.type = ts_interpreter_get_type_info(type_text, decl_size, decl_type);
        if (strcmp(ts_node_type(name_node), "pointer_declarator") == 0) {
            TSNodeObjectType elem_type;
            uint64_t elem_size;
            ts_interpreter_pointee_type(type_text, &elem_type, &elem_size);
            new_var.array_element_type.category = elem_type;
            new_var.array_element_type.size = elem_size;
        }
        else {
            new_var.array_element_type.category = TSNodeObjectTypeUnknown;
            new_var.array_element_type.size = 0; // Not a pointer
        }
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
        } else {
            TS_PRINTF_ERROR("Unsupported initializer type for variable declaration.\n");
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