#include <stdio.h>
#include <stdlib.h>
#include "bin-trees.h"

static void
real_preorder (tree_ptr root)
{
  if (root == NULL)
    return;

  printf ("%d ", root->data);
  real_preorder (root->left);
  real_preorder (root->right);
}


void
pre_order_traverse (tree_ptr root)
{
  printf ("pre-order traversal, with recursion: \n");
  real_preorder (root) ;
  printf ("\n");
}
