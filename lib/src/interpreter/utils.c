#include "tree_sitter/api.h"
#include <string.h>

/* Single definition of the struct info, filled by metapro at startup. See api.h */
TSRecordInfo* record_info_table = NULL;

TSNodeObjectType ts_interpreter_get_category_type(const char* category) {
    if (category == NULL) return TSNodeObjectTypeUnknown;
    if (strcmp(category, "int") == 0) return TSNodeObjectTypeInt;
    if (strcmp(category, "uint") == 0) return TSNodeObjectTypeUInt;
    if (strcmp(category, "double") == 0) return TSNodeObjectTypeDouble;
    if (strcmp(category, "ptr") == 0 || strcmp(category, "array") == 0 ||
            strcmp(category, "struct_ptr") == 0 || strcmp(category, "struct_array") == 0) {
        return TSNodeObjectTypePointer;
    }
    if (strcmp(category, "struct") == 0) return TSNodeObjectTypeStruct;
    return TSNodeObjectTypeUnknown;
}

TSTypeInfo ts_interpreter_get_type_info(const char* name, uint32_t size, TSNodeObjectType category) {
    TSTypeInfo type_info;
    sprintf(type_info.name, "%s", name);
    type_info.size = size;
    type_info.category = category;
    return type_info;
}

TSTypeInfo ts_interpreter_get_pointer_type_info(TSTypeInfo pointee_type_info) {
    TSTypeInfo type_info;
    sprintf(type_info.name, "%s*", pointee_type_info.name);
    type_info.size = sizeof(void*);
    type_info.category = TSNodeObjectTypePointer;
    return type_info;
}