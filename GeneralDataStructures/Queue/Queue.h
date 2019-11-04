#ifndef _Queue_h
#define _Queue_h

// the queue types are exposed so scheduler can have low-level manipulations
typedef struct qnode {
	void *item;
	struct qnode *node_behind;
} qnode_t;

typedef struct queue {
	qnode_t *head;
	qnode_t *tail;
} q_t;


/* -------- basic queue functions -------- */

// returns NULL if malloc failed
q_t *make_q();


void free_q(q_t *queue, void (*itemdelete)(void *item));


// returns ERROR if the queue is NULL or malloc failed; otherwise 0.
int enq_q(q_t *queue, void *item);


/* deq_q() frees the node it deq'd, but the caller is still 
 * responsible for freeing the return item. Returns NULL if
 * the queue is NULL or if the queue is empty.
 */
void *deq_q(q_t *queue);


/* -------- unconventional queue functions -------- */

void iterate_q(q_t *queue, void (*itemfunc)(void *item));


// returns NULL if the queue is NULL or it's empty.
void *peek_q(q_t *queue);


#endif  /* _Queue_h */
