#include <stdlib.h>
#include "bin-trees.h"

tree_ptr
new_node (int value)
{
  tree_ptr node = (tree_ptr) malloc (sizeof (tree_ptr));
  node->data = value;
  node->left = NULL;
  node->right = NULL;
  return node;
}

void
search_tree_insert (tree_ptr *root, int value)
{
  if (*root == NULL)
    *root = new_node (value);
  else if (value < (*root)->data)
    search_tree_insert (&((*root)->left), value);
  else if (value > (*root)->data)
    search_tree_insert (&((*root)->right), value);
}
