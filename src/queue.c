#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "queue.h"

queue_item_t *queue_new_item (void *data)
{
  queue_item_t *item = malloc(sizeof(queue_item_t)); 
  item->data = data;
  item->next = NULL;
  return item;
}

bool queue_isempty (myqueue_t queue)
{
  return !queue;
}

void queue_enqueue (myqueue_t *queue, void *data)
{
  assert(queue != NULL);

  while (*queue)
    queue = &(*queue)->next;

  *queue = queue_new_item(data);
}

void *queue_dequeue (myqueue_t *queue)
{
  assert(queue != NULL);
  assert(*queue != NULL);
  queue_item_t *item = *queue;
  *queue = (*queue)->next;
  void *data = item->data;
  free(item);
  return data;
}

void *queue_top (myqueue_t queue)
{
  return queue->data;
}

int queue_count (myqueue_t queue)
{
  if (!queue) return 0;
  return queue_count(queue->next) + 1;
}
