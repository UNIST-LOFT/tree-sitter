#ifndef TREE_SITTER_TREE_H_
#define TREE_SITTER_TREE_H_

#include "./subtree.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  const Subtree *child;
  const Subtree *parent;
  Length position;
  TSSymbol alias_symbol;
} ParentCacheEntry;

struct TSTree {
  Subtree root;
  const TSLanguage *language;
  TSRange *included_ranges;
  unsigned included_range_count;

  // Added by FreddyYJ
  uint32_t node_value_count;
  TSNode node_value_keys[10000];
  char* node_value_values[10000];

  uint32_t node_value_2_count;
  TSNode node_value_2_keys[10000];
  char* node_value_2_values[10000];
};

TSTree *ts_tree_new(Subtree root, const TSLanguage *language, const TSRange *, unsigned);
TSNode ts_node_new(const TSTree *, const Subtree *, Length, TSSymbol);

#ifdef __cplusplus
}
#endif

#endif  // TREE_SITTER_TREE_H_
