#include <stdlib.h>
#include <stdio.h>
#include "bin-trees.h"

tree_ptr
pop (struct stack_struct **stack)
{
  if (*stack == NULL)
    return NULL;
  else
    {
      tree_ptr value = (*stack)->data;
      (*stack) = (*stack)->next;
      return value;
    }
}

void
push (struct stack_struct **stack, tree_ptr value)
{
  struct stack_struct *new_node = (struct stack_struct *) malloc (sizeof (struct stack_struct *));
  new_node->data = value;
  new_node->next = *stack;
  *stack = new_node;
}
