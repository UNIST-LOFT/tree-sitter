#include "tree_sitter/api.h"
#include <string.h>

/* Single definition of the globals declared `extern` in api.h. These are
 * shared by every translation unit that includes the header. */
TSNodeObject new_variables[MAX_VARS];
uint32_t new_var_count = 0;


TSNodeObject ts_interpreter_var_decl(TSNode node, uint64_t var_count, TSNodeObject* vars) {
    if (new_var_count >= MAX_VARS) {
        TS_PRINTF_ERROR("Exceeded maximum number of new variables: %d\n", MAX_VARS);
    }
    
    TSNode type = ts_node_named_child(node, 0);
    // Check type is valid

    TSNode rhs = ts_node_named_child(node, 1);
    if (strcmp(ts_node_type(type), "identifier") == 0) {
        // Without initialization, e.g., int x;
        
    }
    else if (strcmp(ts_node_type(type), "init_declarator") == 0) {
        // Pointer type, e.g., int* x;
        TSNode name = ts_node_named_child(type, 0);
        TSNode value = ts_node_named_child(type, 1);
        if (strcmp(ts_node_type(name), "pointer_declarator") == 0) {
            name = ts_node_named_child(name, 0); // identifier
        }
    }
    
}