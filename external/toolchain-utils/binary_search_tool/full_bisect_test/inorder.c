#include <stdio.h>
#include <stdlib.h>
#include "bin-trees.h"

static void
real_inorder (tree_ptr root)
{
  if (root == NULL)
    return;

  real_inorder (root->left);
  printf ("%d ", root->data);
  real_inorder (root->right);
}

void
in_order_traverse (tree_ptr root)
{
  printf ("in-order traversal, with recursion: \n");
  real_inorder (root);
  printf ("\n");
}
