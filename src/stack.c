#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "stack.h"

stack_item_t *stack_new_item (void *data)
{
  stack_item_t *item = malloc(sizeof(stack_item_t)); 
  item->data = data;
  item->next = NULL;
  return item;
}
bool stack_isempty (stack_t stack)
{
  return !stack;
}
void stack_push (stack_t *stack, void *data)
{
  assert(stack != NULL);
  stack_item_t *item = stack_new_item(data);
  if (*stack == NULL) {
    *stack = item;
    return;
  }
  item->next = *stack;
  *stack = item;
}

void *stack_pop (stack_t *stack)
{
  assert(stack != NULL);
  assert(*stack != NULL);
  stack_item_t *item = *stack;
  *stack = (*stack)->next;
  void *data = item->data;
  free(item);
  return data;
}

void *stack_top (stack_t stack)
{
  return stack->data;
}

int stack_count (stack_t stack)
{
  if (!stack) return 0;
  return stack_count(stack->next) + 1;
}
