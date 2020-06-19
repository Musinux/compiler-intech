#ifndef QUEUE_H
#define QUEUE_H
#include <stdbool.h>

typedef struct queue_item_t {
  void *data;
  struct queue_item_t *next;
} queue_item_t;

typedef struct queue_item_t * myqueue_t;

queue_item_t *queue_new_item (void *data);
bool queue_isempty (myqueue_t queue);
void queue_enqueue (myqueue_t *queue, void *data);
void *queue_dequeue (myqueue_t *queue);
void *queue_head (myqueue_t queue);
int queue_count (myqueue_t queue);

#endif /* ifndef QUEUE_H */
