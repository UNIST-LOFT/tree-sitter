#include "tree_sitter/api.h"
#include <string.h>

/* Single definition of the struct info, filled by metapro at startup. See api.h */
TSRecordInfo* record_info_table = NULL;

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

TSNodeObjectType ts_interpreter_get_category_type(const char* category) {
    if (category == NULL) return TSNodeObjectTypeUnknown;
    else if (strcmp(category, "int") == 0) return TSNodeObjectTypeInt;
    else if (strcmp(category, "uint") == 0) return TSNodeObjectTypeUInt;
    else if (strcmp(category, "double") == 0) return TSNodeObjectTypeDouble;
    else if (strcmp(category, "ptr") == 0 || strcmp(category, "array") == 0 ||
            strcmp(category, "struct_ptr") == 0 || strcmp(category, "struct_array") == 0) {
        return TSNodeObjectTypePointer;
    }
    else if (strcmp(category, "struct") == 0) return TSNodeObjectTypeStruct;
    else return TSNodeObjectTypeUnknown;
}

TSTypeInfo ts_interpreter_get_type_info(const char* name, uint32_t size, TSNodeObjectType category) {
    TSTypeInfo type_info;
    type_info.element_name = NULL; // Only a type read from the type info names its element
    snprintf(type_info.name, sizeof(type_info.name), "%s", name);
    type_info.size = size;
    type_info.category = category;
    return type_info;
}

TSTypeInfo ts_interpreter_get_pointer_type_info(TSTypeInfo pointee_type_info) {
    TSTypeInfo type_info;
    type_info.element_name = NULL; // The pointee is passed by value, so its name is not kept
    /* The "*" is kept even when the name of the pointee has to give way for it, so that the result still
       reads as a pointer rather than as the type it points at */
    size_t length = strlen(pointee_type_info.name);
    if (length + 1 >= sizeof(type_info.name)) {
        length = sizeof(type_info.name) - 2;
    }
    memcpy(type_info.name, pointee_type_info.name, length);
    type_info.name[length] = '*';
    type_info.name[length + 1] = '\0';
    type_info.size = sizeof(void*);
    type_info.category = TSNodeObjectTypePointer;
    return type_info;
}

/*
    Split a type spelling into the name of what it points at and the number of "*" written after it:
    "int32_t **" -> "int32_t", 2. Trailing spaces of the name are dropped as well.
*/
static uint32_t ts_interpreter_split_pointers(const char* type_text, char* base, size_t base_size) {
    snprintf(base, base_size, "%s", type_text);
    size_t length = strlen(base);
    uint32_t stars = 0;
    while (length > 0 && (base[length - 1] == '*' || base[length - 1] == ' ')) {
        if (base[length - 1] == '*') stars++;
        base[--length] = '\0';
    }
    return stars;
}

/* Look a spelling up among the types the grammar knows by itself, so a cast or a declaration of one
   of them still resolves when the program has no type info at all */
static int ts_interpreter_builtin_type(const char* name, TSTypeInfo* type) {
    for (size_t i = 0; i < sizeof(CAST_TYPE_WIDTH) / sizeof(CAST_TYPE_WIDTH[0]); i++) {
        for (size_t j = 0; CAST_TYPE_WIDTH[i].names[j] != NULL; j++) {
            if (strcmp(name, CAST_TYPE_WIDTH[i].names[j]) == 0) {
                *type = ts_interpreter_get_type_info(name, (uint32_t)CAST_TYPE_WIDTH[i].size,
                                                     CAST_TYPE_WIDTH[i].type);
                return 1;
            }
        }
    }
    return 0;
}

/*
    Look a spelling up in the type info of metapro, which names a pointer with a space before the
    stars, e.g. "int32_t *", while the source code writes it either way. The spelling as written is
    tried first, then the one metapro would have used.
*/
static int ts_interpreter_named_type(const char* type_text, TSTypeInfo* type_info_table,
                                     TSTypeInfo* type) {
    if (type_info_table == NULL) {
        return 0;
    }
    TSTypeInfo* found = NULL;
    HASH_FIND_STR(type_info_table, type_text, found);
    if (found == NULL) {
        char base[TS_MAX_TYPE_NAME_SIZE];
        uint32_t stars = ts_interpreter_split_pointers(type_text, base, sizeof(base));
        if (stars == 0) {
            return 0;
        }
        // The name metapro would have used: the base, one space, then the stars
        char spaced[TS_MAX_TYPE_NAME_SIZE];
        size_t length = strlen(base);
        if (length + 1 + stars >= sizeof(spaced)) {
            return 0; // Does not fit, so no such name is stored either
        }
        memcpy(spaced, base, length);
        spaced[length] = ' ';
        for (uint32_t i = 0; i < stars; i++) {
            spaced[length + 1 + i] = '*';
        }
        spaced[length + 1 + stars] = '\0';
        HASH_FIND_STR(type_info_table, spaced, found);
        if (found == NULL) {
            return 0;
        }
    }
    *type = *found;
    return 1;
}

int ts_interpreter_resolve_type(const char* type_text, TSTypeInfo* type_info_table,
                                TSTypeInfo* type, TSTypeInfo* element_type) {
    memset(element_type, 0, sizeof(TSTypeInfo));
    element_type->category = TSNodeObjectTypeUnknown;
    if (type_text == NULL) {
        return 0;
    }

    if (!ts_interpreter_builtin_type(type_text, type) &&
            !ts_interpreter_named_type(type_text, type_info_table, type)) {
        /*
            A pointer to a type neither knows is still a pointer: it is the width of one, and the
            code around it only needs to know what it steps by, which the pointee below gives.
        */
        char base[TS_MAX_TYPE_NAME_SIZE];
        if (ts_interpreter_split_pointers(type_text, base, sizeof(base)) == 0) {
            return 0;
        }
        *type = ts_interpreter_get_type_info(type_text, sizeof(void*), TSNodeObjectTypePointer);
    }

    if (type->category != TSNodeObjectTypePointer) {
        return 1;
    }

    /*
        What the pointer points at, which is one "*" less than the pointer itself. A pointee neither
        the table nor the type info names is treated as bytes, the way GNU C treats void pointer
        arithmetic, so that stepping and subscripting it still move by something sensible.
    */
    char pointee[TS_MAX_TYPE_NAME_SIZE];
    uint32_t stars = ts_interpreter_split_pointers(type_text, pointee, sizeof(pointee));
    if (stars == 0) {
        /*
            A typedef of a pointer, e.g. `typedef int32_t* int_ptr`, is spelled without a "*", so what it
            points at is only known from the element the type info records for it. An older type info
            names no element, and then it stays unknown rather than being resolved to the pointer itself,
            which would step by its own width.
        */
        if (type->element_name != NULL && type->element_name[0] != '\0' &&
                (ts_interpreter_builtin_type(type->element_name, element_type) ||
                 ts_interpreter_named_type(type->element_name, type_info_table, element_type))) {
            return 1;
        }
        return 1;
    }
    for (uint32_t i = 1; i < stars; i++) {
        strncat(pointee, "*", sizeof(pointee) - strlen(pointee) - 1);
    }
    if (!ts_interpreter_builtin_type(pointee, element_type) &&
            !ts_interpreter_named_type(pointee, type_info_table, element_type)) {
        *element_type = ts_interpreter_get_type_info(pointee, 1, TSNodeObjectTypeInt);
    }
    return 1;
}
