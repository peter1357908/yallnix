/*
 * Queue.c - modified from CS58 project 2 `traffic` module by Shengsong Gao
 *
 * Generic queue, implemented with linked list.
 *
 */


#include <stdio.h>
#include <stdlib.h>  // for malloc()
#include <yalnix.h>  // for ERROR
#include "Queue.h"


/* -------- basic queue functions -------- */

q_t *make_q() {
	q_t *queue = (q_t *)malloc(sizeof(q_t));
	if (queue != NULL) {
		queue->head = NULL;
		queue->tail = NULL;
	}
	return queue;
}

void free_q(q_t *queue, void (*itemdelete)(void *item)) {
	if (queue != NULL) {
		void *item;
		while(queue->head != NULL) {
			item = deq_q(queue);  //deq_q() frees the node it deq'd
			(*itemdelete)(item);
		}
		free(queue);
	}
}

int enq_q(q_t *queue, void *item) {
	if (queue == NULL) return ERROR;
	
	qnode_t *node = (qnode_t *)malloc(sizeof(qnode_t));
	if (node == NULL) return ERROR;
	
	node->item = item;
	node->node_behind = NULL;
	
	if (queue->head == NULL) {
		queue->head = node;
		queue->tail = node;
	} else {
		queue->tail->node_behind = node;
		queue->tail = node;
	}
	
	return 0;
}

void *deq_q(q_t *queue) {
	void *item = NULL;
	if (queue != NULL && queue->head != NULL) {
		item = queue->head->item;
		qnode_t *new_head = queue->head->node_behind;
		free(queue->head);
		queue->head = new_head;
		if (queue->head == NULL) {
			queue->tail = NULL;
		}
	}
	return item;
}


/* -------- unconventional queue functions -------- */

void iterate_q(q_t *queue, void (*itemfunc)(void *item)) {
	if (queue != NULL) {
		qnode_t *currNode = queue->head;
		while(currNode != NULL) {
			(*itemfunc)(currNode->item);
			currNode = currNode->node_behind;
		}
	}
}


void *peek_q(q_t *queue) {
	return queue->head->item;
}
