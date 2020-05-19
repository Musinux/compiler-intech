#ifndef STACK_H
#define STACK_H
#include <stdbool.h>

typedef struct stack_item_t {
  void *data;
  struct stack_item_t *next;
} stack_item_t;

typedef struct stack_item_t * mystack_t;

stack_item_t *stack_new_item (void *data);
bool stack_isempty (mystack_t stack);
void stack_push (mystack_t *stack, void *data);
void *stack_pop (mystack_t *stack);
void *stack_top (mystack_t stack);
int stack_count (mystack_t stack);

#endif /* ifndef STACK_H */
