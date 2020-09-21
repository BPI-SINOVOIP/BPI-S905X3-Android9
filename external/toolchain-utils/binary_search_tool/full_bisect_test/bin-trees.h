#ifndef _BIN_TREES_H
#define _BIN_TREES_H


struct bin_tree_struct {
  int data;
  char c_data;
  struct bin_tree_struct *left;
  struct bin_tree_struct *right;
};

typedef struct bin_tree_struct * tree_ptr;


struct stack_struct {
  tree_ptr data;
  struct stack_struct *next;
};


void search_tree_insert (tree_ptr *, int);
void pre_order_traverse (tree_ptr);
void pre_order_traverse_no_recurse (tree_ptr);
void in_order_traverse (tree_ptr);
void in_order_traverse_no_recurse (tree_ptr);
void push (struct stack_struct **, tree_ptr);
tree_ptr pop (struct stack_struct **);

#endif /* _BIN_TREES_H */
