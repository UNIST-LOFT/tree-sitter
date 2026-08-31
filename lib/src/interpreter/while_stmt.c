#include "tree_sitter/api.h"

TSNodeObject ts_interpreter_while_stmt(TSNode node, uint64_t var_count, TSNodeObject* vars, TSTypeInfo* type_info_table) {
    TSNode condition_node = ts_node_child_by_field_name(node, "condition", strlen("condition"));
    TSNode body_node = ts_node_child_by_field_name(node, "body", strlen("body"));

    // loop
    while (1) {
        // condition: return int or uint, boolean value
        if (!ts_node_is_null(condition_node)) {
            TSNodeObject condition_result = ts_interpreter_simulate(condition_node, var_count, vars, type_info_table);
            if (condition_result.type.category == TSNodeObjectTypeInt) {
                if (condition_result.value.int64 == 0) {
                    break;
                }
            } else if (condition_result.type.category == TSNodeObjectTypeUInt) {
                if (condition_result.value.uint64 == 0) {
                    break;
                }
            } else {
                TS_PRINTF_ERROR("Unsupported type in while loop condition: %s\n", condition_result.type.name);
            }
        }

        // body: no return value
        if (!ts_node_is_null(body_node)) {
            ts_interpreter_simulate(body_node, var_count, vars, type_info_table);
        }
    }

    // While stmt not return value, create dummy
    TSNodeObject result = {0};
    result.name = "while_stmt";
    result.node = node;
    result.type = ts_interpreter_get_type_info("void", 0, TSNodeObjectTypeUnknown);
    return result;
}