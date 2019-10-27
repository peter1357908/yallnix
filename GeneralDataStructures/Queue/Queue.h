#ifndef _Queue_h
#define _Queue_h

typedef struct queue q_t;

// returns NULL if malloc failed
q_t *make_q();

void free_q(q_t *queue, void (*itemdelete)(void *item));

// returns ERROR if malloc failed; otherwise 0.
int enq_q(q_t *queue, void *item);

/* deq_q() frees the node it deq'd, but the caller is still 
 * responsible for freeing the return item
 */
void *deq_q(q_t *queue);


void *peek_q(q_t *queue);
#endif  /* _Queue_h */