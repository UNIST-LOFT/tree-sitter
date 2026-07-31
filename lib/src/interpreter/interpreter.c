#include "tree_sitter/api.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

TSNodeObject ts_interpreter_variable(TSNode node, uint64_t var_count, TSNodeObject* vars, TSTypeInfo* type_info_table) {
    char* node_name=ts_node_find_value(node);
    // Check newly declared variables first
    for (size_t i=0;i<new_var_count;i++) {
        if (strcmp(node_name, new_variables[i].name)==0) {
            return new_variables[i];
        }
    }

    // It is not a newly declared variable, check the existing variables
    for (size_t i=0;i<var_count;i++) {
        if (strcmp(node_name, vars[i].name)==0) {
            return vars[i];
        }
    }

    TS_PRINTF_ERROR("Variable not found: %s\n", node_name);
}

TSNodeObject ts_interpreter_field(TSNode node, uint64_t var_count, TSNodeObject* vars, TSTypeInfo* type_info_table) {
    TSNodeObject base_obj = ts_interpreter_simulate(ts_node_named_child(node, 0), var_count, vars, type_info_table); // Base object
    char* field_name = ts_node_find_value(ts_node_named_child(node, 1)); // Field name

    if (base_obj.type.category != TSNodeObjectTypeStruct &&
            !(base_obj.type.category == TSNodeObjectTypePointer &&
            base_obj.array_element_type.category == TSNodeObjectTypeStruct)) {
        TS_PRINTF_ERROR("Base object is not a struct or struct pointer: %s\n", base_obj.name);
    }

    // Find the struct type in the record_info_table. A struct is named by its own type, while a struct
    // pointer is named by the type it points to, which is the struct holding the field
    char* record_name = (base_obj.type.category == TSNodeObjectTypeStruct) ? base_obj.type.name
                                                                          : base_obj.array_element_type.name;
    TSRecordInfo* struct_type_info = NULL;
    HASH_FIND_STR(record_info_table, record_name, struct_type_info);
    if (struct_type_info == NULL) {
        TS_PRINTF_ERROR("Struct type not found in record_info_table: %s\n", record_name);
    }

    // Find the field in the struct's field_info_table
    TSFieldInfo* field_info = NULL;
    HASH_FIND_STR(struct_type_info->field_info_table, field_name, field_info);
    if (field_info == NULL) {
        TS_PRINTF_ERROR("Field not found in struct: %s.%s\n", record_name, field_name);
    }

    // Create a new TSNodeObject for the field
    TSNodeObject field_obj = {0}; // TSTypeInfo holds a name, so zero it instead of leaving garbage
    field_obj.type = ts_interpreter_get_type_info(field_info->type_name, field_info->size, field_info->category);
    if (base_obj.type.category == TSNodeObjectTypeStruct) {
        // Normal struct field
        field_obj.name = malloc(strlen(base_obj.name) + strlen(field_name) + 2); // For "base.field" and null terminator
        sprintf(field_obj.name, "%s.%s", base_obj.name, field_name);
    }
    else {
        // struct pointer
        field_obj.name = malloc(strlen(base_obj.name) + strlen(field_name) + 4); // For "base->field" and null terminator
        sprintf(field_obj.name, "%s->%s", base_obj.name, field_name);
    }
    /*
        A pointer or array field carries what it points to, so a field of it can be resolved in turn and it
        can be subscripted. Only an element of a record type is named in the struct info, and the name of a
        struct pointer sits in struct_type while the name of a struct array sits in array_element_type. The
        others only have a category, kept in whichever of the two the struct info did not use for a name.
    */
    if (field_obj.type.category == TSNodeObjectTypePointer) {
        if (strcmp(field_info->type, "struct_ptr") == 0) {
            field_obj.array_element_type = ts_interpreter_get_type_info(field_info->struct_type,
                    field_info->array_element_size, TSNodeObjectTypeStruct);
        }
        else if (strcmp(field_info->type, "struct_array") == 0) {
            field_obj.array_element_type = ts_interpreter_get_type_info(field_info->array_element_type,
                    field_info->array_element_size, TSNodeObjectTypeStruct);
        }
        else if (strcmp(field_info->type, "array") == 0) {
            // An array of a non record type keeps the category of its element in struct_type, and a name
            // in array_element_type only when that element is a pointer to a struct
            field_obj.array_element_type = ts_interpreter_get_type_info(field_info->array_element_type,
                    field_info->array_element_size, ts_interpreter_get_category_type(field_info->struct_type));
        }
        else {
            // A plain pointer keeps the category of its pointee in array_element_type, and names no type
            field_obj.array_element_type = ts_interpreter_get_type_info("", field_info->array_element_size,
                    ts_interpreter_get_category_type(field_info->array_element_type));
        }
    }
    field_obj.node = node;
    
    /*
        Calculate the address of the field based on the address of the struct and the offset. A struct is
        at its own address, while a struct pointer holds the address of the struct as its value, the same
        way `reference` is the address of the variable itself for every other pointer.
    */
    const void* struct_address = (base_obj.type.category == TSNodeObjectTypeStruct) ? base_obj.reference
                                                                                    : base_obj.value.pointer;
    if (struct_address == NULL) {
        TS_PRINTF_ERROR("Address of the struct is NULL for: %s\n", base_obj.name);
    }

    field_obj.reference = (void*)((uint8_t*)struct_address + field_info->offset);

    // Set the value of the field based on its type.
    // Read exactly the size of the field. Reading a smaller field as int64_t/uint64_t/double takes 8 bytes,
    // which reads past the struct when the field is its last member, drops the sign of a negative value,
    // and takes padding or the next field into the high bytes.
    switch (field_obj.type.category) {
        case TSNodeObjectTypeInt:
            switch (field_obj.type.size) {
                case 1:
                    field_obj.value.int64 = *((int8_t*)field_obj.reference);
                    break;
                case 2:
                    field_obj.value.int64 = *((int16_t*)field_obj.reference);
                    break;
                case 4:
                    field_obj.value.int64 = *((int32_t*)field_obj.reference);
                    break;
                case 8:
                    field_obj.value.int64 = *((int64_t*)field_obj.reference);
                    break;
                default:
                    TS_PRINTF_ERROR("Unsupported int field size for %s: %" PRIu32 "\n", field_name, field_obj.type.size);
            }
            break;
        case TSNodeObjectTypeUInt:
            switch (field_obj.type.size) {
                case 1:
                    field_obj.value.uint64 = *((uint8_t*)field_obj.reference);
                    break;
                case 2:
                    field_obj.value.uint64 = *((uint16_t*)field_obj.reference);
                    break;
                case 4:
                    field_obj.value.uint64 = *((uint32_t*)field_obj.reference);
                    break;
                case 8:
                    field_obj.value.uint64 = *((uint64_t*)field_obj.reference);
                    break;
                default:
                    TS_PRINTF_ERROR("Unsupported uint field size for %s: %" PRIu32 "\n", field_name, field_obj.type.size);
            }
            break;
        case TSNodeObjectTypeDouble:
            switch (field_obj.type.size) {
                case 4:
                    field_obj.value.double64 = *((float*)field_obj.reference);
                    break;
                case 8:
                    field_obj.value.double64 = *((double*)field_obj.reference);
                    break;
                case 16:
                    field_obj.value.double64 = *((long double*)field_obj.reference);
                    break;
                default:
                    TS_PRINTF_ERROR("Unsupported double field size for %s: %" PRIu32 "\n", field_name, field_obj.type.size);
            }
            break;
        case TSNodeObjectTypePointer:
            field_obj.value.pointer = *((void**)field_obj.reference); // Always sizeof(void*)
            break;
        default:
            // Just set reference only for struct and general types
            break;
    }
    return field_obj;
}

TSNodeObject ts_interpreter_sizeof(TSNode node, uint64_t var_count, TSNodeObject* vars, TSTypeInfo* type_info_table) {
    if (strcmp(ts_node_type(ts_node_named_child(node, 0)), "type_descriptor") == 0) {
        TS_PRINTF_ERROR("Currently sizeof cannot handle type descriptor (e.g., int)\n");
    }
    TSNodeObject obj = ts_interpreter_simulate(ts_node_named_child(node, 0), var_count, vars, type_info_table);
    TSNodeObject size_obj;
    size_obj.name = NULL; // No name for sizeof result
    size_obj.node = node;
    size_obj.type = ts_interpreter_get_type_info("size_t", sizeof(uint64_t), TSNodeObjectTypeUInt);
    size_obj.value.uint64 = obj.type.size; // Return the size of the object
    size_obj.reference = malloc(sizeof(uint64_t));
    *((uint64_t*)size_obj.reference) = obj.type.size; // Store the size in
    return size_obj;
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
    obj.array_element_type.size = 0;

    if (strcmp(ts_node_type(node),"char_literal")==0) {
        obj.type = ts_interpreter_get_type_info("char", sizeof(char), TSNodeObjectTypeInt);
        obj.value.int64=ts_node_find_value(node)[0];
    }
    else if (in_str(obj.name,'.')) {
        // Float/double
        if (in_str(obj.name,'f') || in_str(obj.name,'F')) {
            obj.type = ts_interpreter_get_type_info("float", sizeof(float), TSNodeObjectTypeDouble);
        }
        else {
            obj.type = ts_interpreter_get_type_info("double", sizeof(double), TSNodeObjectTypeDouble);
        }
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
            obj.type = ts_interpreter_get_type_info("unsigned long long", sizeof(unsigned long long), TSNodeObjectTypeUInt);
        }
        else if (is_postfix(obj.name,"U") || is_postfix(obj.name,"u") ||
                    (long_size==4 && (is_postfix(obj.name,"UL") || is_postfix(obj.name,"ul") ||
                    is_postfix(obj.name,"LU") || is_postfix(obj.name,"lu")))) {
            obj.type = ts_interpreter_get_type_info("unsigned long", sizeof(unsigned long), TSNodeObjectTypeUInt);
        }
        else {
            TS_PRINTF_ERROR("Unknown unsigned type: %s\n", obj.name);
        }
        obj.value.uint64=(uint64_t)atoll(ts_node_find_value(node));
    }
    else {
        // signed
        int long_size=sizeof(long);

        if (is_postfix(obj.name,"LL") || is_postfix(obj.name,"ll") ||
                    (long_size==8 && (is_postfix(obj.name,"L") || is_postfix(obj.name,"l")))) {
            obj.type = ts_interpreter_get_type_info("long long", sizeof(long long), TSNodeObjectTypeInt);
        }
        else if (in_str(obj.name,'\'')) {
            // Char literal
            obj.type = ts_interpreter_get_type_info("char", sizeof(char), TSNodeObjectTypeInt);
        }
        else if (long_size==4 && (is_postfix(obj.name,"L") || is_postfix(obj.name,"l"))) {
            obj.type = ts_interpreter_get_type_info("long", sizeof(long), TSNodeObjectTypeInt);
        }
        else {
            obj.type = ts_interpreter_get_type_info("int", sizeof(int), TSNodeObjectTypeInt);
        }
        obj.value.int64=atoll(ts_node_find_value(node));
    }

    return obj;
}

TSNodeObject ts_interpreter_casting(TSNode node, uint64_t var_count, TSNodeObject* vars, TSTypeInfo* type_info_table) {
    TSNodeObject obj = ts_interpreter_simulate(ts_node_named_child(node, 1), var_count, vars, type_info_table); // Casted value
    char* cast_type = ts_node_find_value(node); // Casted type
    if (strcmp(cast_type, "int8_t") == 0)
        obj.type = ts_interpreter_get_type_info("int8_t", 1, TSNodeObjectTypeInt);
    else if (strcmp(cast_type, "int16_t") == 0)
        obj.type = ts_interpreter_get_type_info("int16_t", 2, TSNodeObjectTypeInt);
    else if (strcmp(cast_type, "int32_t") == 0)
        obj.type = ts_interpreter_get_type_info("int32_t", 4, TSNodeObjectTypeInt);
    else if (strcmp(cast_type, "int64_t") == 0)
        obj.type = ts_interpreter_get_type_info("int64_t", 8, TSNodeObjectTypeInt);
    else if (strcmp(cast_type, "uint8_t") == 0 || strcmp(cast_type, "u8") == 0)
        obj.type = ts_interpreter_get_type_info("uint8_t", 1, TSNodeObjectTypeUInt);
    else if (strcmp(cast_type, "uint16_t") == 0 || strcmp(cast_type, "u16") == 0)
        obj.type = ts_interpreter_get_type_info("uint16_t", 2, TSNodeObjectTypeUInt);
    else if (strcmp(cast_type, "uint32_t") == 0 || strcmp(cast_type, "u32") == 0)
        obj.type = ts_interpreter_get_type_info("uint32_t", 4, TSNodeObjectTypeUInt);
    else if (strcmp(cast_type, "uint64_t") == 0 || strcmp(cast_type, "u64") == 0)
        obj.type = ts_interpreter_get_type_info("uint64_t", 8, TSNodeObjectTypeUInt);
    else if (strcmp(cast_type, "float") == 0)
        obj.type = ts_interpreter_get_type_info("float", 4, TSNodeObjectTypeDouble);
    else if (strcmp(cast_type, "double") == 0)
        obj.type = ts_interpreter_get_type_info("double", 8, TSNodeObjectTypeDouble);
    // Pointer casts
    else if (strcmp(cast_type, "int8_t*") == 0 || strcmp(cast_type, "char*") == 0) {
        obj.array_element_type = ts_interpreter_get_type_info("int8_t", 1, TSNodeObjectTypeInt);
        obj.type = ts_interpreter_get_pointer_type_info(obj.array_element_type);
        if (obj.type.category != TSNodeObjectTypePointer) {
            obj.value.pointer = *((int8_t**)obj.reference);
            
        }
    }
    else if (strcmp(cast_type, "int16_t*") == 0) {
        obj.array_element_type = ts_interpreter_get_type_info("int16_t", 2, TSNodeObjectTypeInt);
        obj.type = ts_interpreter_get_pointer_type_info(obj.array_element_type);
        if (obj.type.category != TSNodeObjectTypePointer) {
            obj.value.pointer = *((int16_t**)obj.reference);
        }
    }
    else if (strcmp(cast_type, "int32_t*") == 0) {
        obj.array_element_type = ts_interpreter_get_type_info("int32_t", 4, TSNodeObjectTypeInt);
        obj.type = ts_interpreter_get_pointer_type_info(obj.array_element_type);
        if (obj.type.category != TSNodeObjectTypePointer) {
            obj.value.pointer = *((int32_t**)obj.reference);
        }
    }
    else if (strcmp(cast_type, "int64_t*") == 0) {
        obj.array_element_type = ts_interpreter_get_type_info("int64_t", 8, TSNodeObjectTypeInt);
        obj.type = ts_interpreter_get_pointer_type_info(obj.array_element_type);
        if (obj.type.category != TSNodeObjectTypePointer) {
            obj.value.pointer = *((int64_t**)obj.reference);
        }
    }
    else if (strcmp(cast_type, "uint8_t*") == 0 || strcmp(cast_type, "u8*") == 0) {
        obj.array_element_type = ts_interpreter_get_type_info("uint8_t", 1, TSNodeObjectTypeUInt);
        obj.type = ts_interpreter_get_pointer_type_info(obj.array_element_type);
        if (obj.type.category != TSNodeObjectTypePointer) {
            obj.value.pointer = *((uint8_t**)obj.reference);
        }
    }
    else if (strcmp(cast_type, "uint16_t*") == 0 || strcmp(cast_type, "u16*") == 0) {
        obj.array_element_type = ts_interpreter_get_type_info("uint16_t", 2, TSNodeObjectTypeUInt);
        obj.type = ts_interpreter_get_pointer_type_info(obj.array_element_type);
        if (obj.type.category != TSNodeObjectTypePointer) {
            obj.value.pointer = *((uint16_t**)obj.reference);
        }
    }
    else if (strcmp(cast_type, "uint32_t*") == 0 || strcmp(cast_type, "u32*") == 0) {
        obj.array_element_type = ts_interpreter_get_type_info("uint32_t", 4, TSNodeObjectTypeUInt);
        obj.type = ts_interpreter_get_pointer_type_info(obj.array_element_type);
        if (obj.type.category != TSNodeObjectTypePointer) {
            obj.value.pointer = *((uint32_t**)obj.reference);
        }
    }
    else if (strcmp(cast_type, "uint64_t*") == 0 || strcmp(cast_type, "u64*") == 0) {
        obj.array_element_type = ts_interpreter_get_type_info("uint64_t", 8, TSNodeObjectTypeUInt);
        obj.type = ts_interpreter_get_pointer_type_info(obj.array_element_type);
        if (obj.type.category != TSNodeObjectTypePointer) {
            obj.value.pointer = *((uint64_t**)obj.reference);
        }
    }
    else if (strlen(cast_type) > 0 && cast_type[strlen(cast_type) - 1] == '*') {
        // Casting to non-primitive pointer type
        const char* pointee_type_name = malloc(strlen(cast_type));
        strncpy((char*)pointee_type_name, cast_type, strlen(cast_type) - 1);
        ((char*)pointee_type_name)[strlen(cast_type) - 1] = '\0';
        obj.array_element_type = ts_interpreter_get_type_info(pointee_type_name, 1, TSNodeObjectTypeUnknown);
        obj.type = ts_interpreter_get_pointer_type_info(obj.array_element_type);
        obj.value.pointer = *((void**)obj.reference);
    }
    else if (strcmp(cast_type, "void") == 0) {
        // Casting to void, do nothing
    }
    else if (strlen(cast_type) >= 4 && cast_type[0] == 'e' && cast_type[1] == 'n' &&
             cast_type[2] == 'u' && cast_type[3] == 'm') {
        // Casting to enum, treat as int
        obj.type = ts_interpreter_get_type_info("int", sizeof(int), TSNodeObjectTypeInt);
    }
    else {
        TS_PRINTF_ERROR("Unsupported cast type: %s\n", cast_type);
    }
    return obj;
}

TSNodeObject ts_interpreter_subscript(TSNode node, uint64_t var_count, TSNodeObject* vars, TSTypeInfo* type_info_table) {
    // Base is pointer variable
    TSNodeObject base_obj = ts_interpreter_simulate(ts_node_named_child(node, 0), var_count, vars, type_info_table);
    TSNodeObject index_obj = ts_interpreter_simulate(ts_node_named_child(node, 1), var_count, vars, type_info_table);

    TSNodeObject obj;
    obj.name = NULL; // No name for subscript result
    obj.node = node;
    obj.type = base_obj.array_element_type;
    uint32_t index;
    // Compute index
    if (index_obj.type.category == TSNodeObjectTypeInt)
        index = (uint32_t)(index_obj.value.int64);
    else if (index_obj.type.category == TSNodeObjectTypeUInt)
        index = (uint32_t)(index_obj.value.uint64);
    else
        TS_PRINTF_ERROR("Array index must be int or uint type\n");

    if (base_obj.array_element_type.category == TSNodeObjectTypeInt) {
        obj.type.category = TSNodeObjectTypeInt;
        switch (base_obj.array_element_type.size) {
            case 1:                
                obj.reference = (void*)((unsigned char*)base_obj.reference + 
                        (index * base_obj.array_element_type.size)); // Compute offset
                obj.value.int64 = *((int8_t*)obj.reference); // Store pointer value as int64
                break;
            case 2:
                obj.reference = (void*)((unsigned char*)base_obj.reference + 
                        (index * base_obj.array_element_type.size)); // Compute offset
                obj.value.int64 = *((int16_t*)obj.reference); // Store pointer value as int64
                break;
            case 4:
                obj.reference = (void*)((unsigned char*)base_obj.reference + 
                        (index * base_obj.array_element_type.size)); // Compute offset
                obj.value.int64 = *((int32_t*)obj.reference); // Store pointer value as int64
                break;
            case 8:
                obj.reference = (void*)((unsigned char*)base_obj.reference + 
                        (index * base_obj.array_element_type.size)); // Compute offset
                obj.value.int64 = *((int64_t*)obj.reference); // Store pointer value as int64
                break;
            default:
                TS_PRINTF_ERROR("Unsupported int array element size: %d\n", base_obj.array_element_type.size);
        }
        obj.array_element_type.size = 0; // Not an array
    }
    else if (base_obj.array_element_type.category == TSNodeObjectTypeUInt) {
        obj.type.category = TSNodeObjectTypeUInt;
        switch (base_obj.array_element_type.size) {
            case 1:
                obj.reference = (void*)((unsigned char*)base_obj.reference + (index * base_obj.array_element_type.size)); // Compute offset
                obj.value.uint64 = *((uint8_t*)obj.reference); // Store pointer value as uint64
                break;
            case 2:
                obj.reference = (void*)((unsigned char*)base_obj.reference + (index * base_obj.array_element_type.size)); // Compute offset
                obj.value.uint64 = *((uint16_t*)obj.reference); // Store pointer value as uint64
                break;
            case 4:
                obj.reference = (void*)((unsigned char*)base_obj.reference + (index * base_obj.array_element_type.size)); // Compute offset
                obj.value.uint64 = *((uint32_t*)obj.reference); // Store pointer value as uint64
                break;
            case 8:
                obj.reference = (void*)((unsigned char*)base_obj.reference + (index * base_obj.array_element_type.size)); // Compute offset
                obj.value.uint64 = *((uint64_t*)obj.reference); // Store pointer value as uint64
                break;
            default:
                TS_PRINTF_ERROR("Unsupported uint array element size: %d\n", base_obj.array_element_type.size);
        }
        obj.array_element_type.size = 0; // Not an array
    }
    else if (base_obj.array_element_type.category == TSNodeObjectTypeDouble) {
        obj.type.category = TSNodeObjectTypeDouble;
        switch (base_obj.array_element_type.size) {
            case 4:
                obj.reference = (void*)((unsigned char*)base_obj.reference + (index * base_obj.array_element_type.size)); // Compute offset
                obj.value.double64 = *((float*)obj.reference); // Store pointer value as double64
                break;
            case 8:
                obj.reference = (void*)((unsigned char*)base_obj.reference + (index * base_obj.array_element_type.size)); // Compute offset
                obj.value.double64 = *((double*)obj.reference); // Store pointer value as double64
                break;
            default:
                TS_PRINTF_ERROR("Unsupported double array element size: %d\n", base_obj.array_element_type.size);
        }
        obj.array_element_type.size = 0; // Not an array
    }
    else if (base_obj.array_element_type.category == TSNodeObjectTypePointer) {
        obj.type.category = TSNodeObjectTypePointer;
        obj.reference = (void*)((unsigned char*)base_obj.reference + (index * base_obj.array_element_type.size)); // Compute offset
        obj.value.pointer = *((void**)obj.reference); // Store pointer value
        obj.array_element_type.size = 1; // Unknown size for pointer array element
    }
    else {
        TS_PRINTF_ERROR("Unsupported array subscript type: %d\n", base_obj.array_element_type.category);
    }
    return obj;
}

TSNodeObject ts_interpreter_simulate(TSNode node, uint64_t var_count, TSNodeObject* vars, TSTypeInfo* type_info_table) {
    if (strcmp(ts_node_type(node),"identifier")==0 ||
        strcmp(ts_node_type(node),"statement_identifier")==0) {
        return ts_interpreter_variable(node,var_count,vars,type_info_table);
    }
    else if (strcmp(ts_node_type(node),"number_literal")==0 || strcmp(ts_node_type(node),"char_literal")==0) {
        return ts_interpreter_literal(node);
    }
    else if (strcmp(ts_node_type(node),"field_expression")==0) {
        return ts_interpreter_field(node,var_count,vars,type_info_table);
    }
    else if (strcmp(ts_node_type(node),"unary_expression")==0 ||
             strcmp(ts_node_type(node),"pointer_expression")==0 ||
             strcmp(ts_node_type(node),"update_expression")==0) {
        return ts_interpreter_unary(node,var_count,vars,type_info_table);
    }
    else if (strcmp(ts_node_type(node),"binary_expression")==0) {
        return ts_interpreter_binary(node,var_count,vars,type_info_table);
    }
    else if (strcmp(ts_node_type(node), "assignment_expression") == 0) {
        return ts_interpreter_assign(node,var_count,vars,type_info_table);
    }
    else if (strcmp(ts_node_type(node),"string_literal")==0) {
        // For string literal, we convert it to pointer of char(s)
        TSNodeObject obj;
        char* value = malloc(sizeof(char)*(strlen(ts_node_find_value(node))+1));
        strcpy(value, ts_node_find_value(node));
        value[strlen(value)] = '\0';
        obj.name=value;
        obj.node=node;
        obj.type = ts_interpreter_get_type_info("char*",
            sizeof(char)*(strlen(ts_node_find_value(node))-2), TSNodeObjectTypeString);
        obj.reference=&value;
        obj.value.pointer=value;
        return obj;
    }
    else if (strcmp(ts_node_type(node),"true")==0) {
        TSNodeObject obj;
        obj.name="true";
        obj.node=node;
        obj.type = ts_interpreter_get_type_info("unsigned int", sizeof(unsigned int), TSNodeObjectTypeUInt);
        obj.value.uint64=1;
        return obj;
    }
    else if (strcmp(ts_node_type(node),"false")==0) {
        TSNodeObject obj;
        obj.name="false";
        obj.node=node;
        obj.type = ts_interpreter_get_type_info("unsigned int", sizeof(unsigned int), TSNodeObjectTypeUInt);
        obj.value.uint64=0;
        return obj;
    }
    else if (strcmp(ts_node_type(node), "null")==0) {
        TSNodeObject obj;
        obj.name="null";
        obj.node=node;
        obj.type = ts_interpreter_get_type_info("void*", sizeof(void*), TSNodeObjectTypePointer);
        obj.value.pointer=NULL;
        return obj;
    }
    else if (strcmp(ts_node_type(node), "cast_expression") == 0) {
        return ts_interpreter_casting(node, var_count, vars, type_info_table);
    }
    else if (strcmp(ts_node_type(node), "sizeof_expression") == 0) {
        return ts_interpreter_sizeof(node, var_count, vars, type_info_table);
    }
    else if (strcmp(ts_node_type(node), "conditional_expression") == 0) {
        TSNodeObject obj;
        TSNodeObject cond_result = ts_interpreter_simulate(ts_node_named_child(node, 0),var_count, vars, type_info_table);
        if (cond_result.value.int64) {
            obj = ts_interpreter_simulate(ts_node_named_child(node, 1), var_count, vars, type_info_table);
        }
        else {
            obj = ts_interpreter_simulate(ts_node_named_child(node, 2), var_count, vars, type_info_table);
        }
        return obj;
    }
    else if (strcmp(ts_node_type(node),"call_expression")==0) {
        return ts_interpreter_function(node,var_count,vars,type_info_table);
    }
    else if (strcmp(ts_node_type(node), "compound_statement") == 0) {
        TSNodeObject obj;
        for (size_t i = 0; i < ts_node_named_child_count(node); i++) {
            obj = ts_interpreter_simulate(ts_node_named_child(node, i), var_count, vars, type_info_table);
        }
        return obj;
    }
    else if (strcmp(ts_node_type(node), "subscript_expression") == 0) {
        return ts_interpreter_subscript(node, var_count, vars, type_info_table);
    }
    else if (strcmp(ts_node_type(node),"parenthesized_expression")==0 ||
            strcmp(ts_node_type(node),"expression_statement")==0 ||
            strcmp(ts_node_type(node),"subscript_argument_list")==0 ||
            strcmp(ts_node_type(node),"ERROR")==0) {
        return ts_interpreter_simulate(ts_node_named_child(node,0),var_count,vars,type_info_table);
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
        if (!found || obj.type.category != TSNodeObjectTypeJmpBuf) {
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
        if (!found || obj.type.category != TSNodeObjectTypeJmpBuf) {
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
        if (!found || obj.type.category != TSNodeObjectTypeJmpBuf) {
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
        if (!found || obj.type.category != TSNodeObjectTypeJmpBuf) {
            TS_PRINTF_ERROR("Return statement found but no corresponding jmp_buf in vars\n");
        }
        longjmp(*(obj.value.jmpbuf), (int)obj.array_element_type.size); // array_element_size is patch ID
    }
    else if (strcmp(ts_node_type(node), "if_statement") == 0) {
        TSNodeObject cond_result = ts_interpreter_simulate(ts_node_named_child(node, 0), var_count, vars, type_info_table);
        int is_then = 0;
        if (cond_result.type.category == TSNodeObjectTypeInt && cond_result.value.int64) {
            is_then = 1;
        }
        else if (cond_result.type.category == TSNodeObjectTypeUInt && cond_result.value.uint64) {
            is_then = 1;
        }
        else if (cond_result.type.category == TSNodeObjectTypeDouble && cond_result.value.double64) {
            is_then = 1;
        }
        else if (cond_result.type.category == TSNodeObjectTypePointer && cond_result.value.pointer) {
            is_then = 1;
        }
        
        if (is_then) {
            // Then branch
            return ts_interpreter_simulate(ts_node_named_child(node, 1), var_count, vars, type_info_table);
        }
        else if (ts_node_named_child_count(node) > 2) {
            // Else branch if exists
            TSNode else_branch = ts_node_named_child(node, 2);
            if (strcmp(ts_node_type(else_branch), "else_clause") == 0)
                else_branch = ts_node_named_child(else_branch, 0); // Skip "else" node
            return ts_interpreter_simulate(else_branch, var_count, vars, type_info_table);
        }
        else {
            // No else branch, just return dummy value
            TSNodeObject obj;
            obj.name = NULL;
            obj.node = node;
            obj.type = ts_interpreter_get_type_info("int", sizeof(int), TSNodeObjectTypeInt);
            obj.value.int64 = 0;
            return obj;
        }
    }
    /* Variable declaration */
    else if (strcmp(ts_node_type(node), "declaration") == 0) {
        return ts_interpreter_var_decl(node, var_count, vars, type_info_table);
    }
    else if (strcmp(ts_node_type(node), "comment") == 0) {
        TSNodeObject obj;
        obj.name = NULL;
        obj.node = node;
        obj.type = ts_interpreter_get_type_info("int", sizeof(int), TSNodeObjectTypeInt);
        obj.value.int64 = 0;
        return obj; // Ignore comments, return dummy value
    }
    else {
        TS_PRINTF_ERROR("Unsupported node type in interpreter: %s\n", ts_node_type(node));
    }
}