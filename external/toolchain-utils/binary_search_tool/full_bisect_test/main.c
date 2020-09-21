#include <stdlib.h>
#include <stdio.h>
#include "bin-trees.h"

int integers[] = {35, 28, 20, 30, 25, 23, 26, 60, 70, 65, 64, 68 };

char pre_order[] = { '/', '-', '+', '*', 'a', '^', 'x', '2', '&', 'b', 'y',
                     'c', '3' };
char in_order[]  = { 'a', '*', 'x', '^', '2', '+', 'b', '&', 'y', '-', 'c',
                     '/', '3' };

int
main (int argc, char ** argv)
{
  int intlist_size = 12;
  int i;
  tree_ptr root = NULL;
  for (i = 0; i < intlist_size; ++i)
    {
      search_tree_insert (&root, integers[i]);
    }
  pre_order_traverse (root);
  printf ("\n");
  pre_order_traverse_no_recurse (root);
  printf ("\n");
  in_order_traverse (root);
  printf ("\n");
  in_order_traverse_no_recurse (root);
  return 0;
}
