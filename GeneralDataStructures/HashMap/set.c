/*
 * set.c - modified from CS50 `set` module by Shengsong Gao
 * 
 * Refer to set.h for usage details
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yalnix.h>  // for ERROR
#include "set.h"


/**************** local types ****************/
typedef struct setnode {
	int key;
	void *item;
	struct setnode *next;
} setnode_t;

/**************** global types ****************/
typedef struct set {
	struct setnode *head;
} set_t;

/**************** set_new() ****************/
set_t *
set_new(void)
{
	set_t *set = malloc(sizeof(set_t));
	if (set != NULL) {
		set->head = NULL;
	}
    return set;
}

/**************** set_insert() ****************/
int
set_insert(set_t *set, int key, void *item)
{
	if (set != NULL && item != NULL) {
		// iterate through the list to see if the key already exists
		// basically set_find():
		setnode_t *node;
		for (node = set->head; node != NULL; node = node->next) {
			if (node->key == key) {
				return ERROR;
			}
		}
		
		// allocate memory for a new node to be added to the list
		node = malloc(sizeof(setnode_t));
	
		if (node == NULL) {
			// error allocating memory for node; return ERROR
			return ERROR;
		} else {
			// the key doesn't exist, and node is allocated - fill it up.
			node->key = key;
			node->item = item;
			
			node->next = set->head;
			set->head = node;
			
			return 0;
		}
	}

	// return ERROR if any of the parameter is NULL
	return ERROR;
}

/**************** set_find() ****************/
void *
set_find(set_t *set, int key)
{
	if (set == NULL) {
		return NULL; // bad set
	} else if (set->head == NULL) {
		return NULL; // set is empty
	} else {
		// else, iterate through the set
		setnode_t *node;
		for (node = set->head; node != NULL; node = node->next) {
			// if key matched, return the item
			if (node->key == key) {
				return node->item;
			}
		}
		// key never matched, return NULL
		return NULL;
	}
}

/**************** set_iterate() ****************/
void
set_iterate(set_t *set, void *arg, void (*itemfunc)(void *arg, int key, void *item) )
{
	if (set != NULL && itemfunc != NULL) {
		// call itemfunc with arg, on each item
		setnode_t * node;
		for (node = set->head; node != NULL; node = node->next) {
		(*itemfunc)(arg, node->key, node->item); 
		}
	}
}

/**************** set_remove() ****************/
void *
set_remove(set_t *set, int key)
{
	if (set == NULL) return NULL;
	setnode_t *currNode = set->head;
	if (currNode == NULL) return NULL;
	
	// checks the head first
	void *item;
	if (currNode->key == key) {
		item = currNode->item;
		set->head = currNode->next;
		free(currNode);
		return item;
	}
	
	// start checking each next node instead
	setnode_t *nextNode = currNode->next;
	setnode_t *nextNextNode;
	while (nextNode != NULL) {
		// if the target item is found, remove and free the node before returning
		if (nextNode->key == key) {
			item = nextNode->item;
			setnode_t *nextNextNode = nextNode->next;
			free(nextNode);
			currNode->next = nextNextNode;
			return item;
		}
		
		// else, advance to the next "nextNode"
		currNode = nextNode;
		nextNode = nextNode->next;
	}
	
	// no item matching the key is found; return NULL
	return NULL;
}

/**************** set_delete() ****************/
void 
set_delete(set_t *set, void (*itemdelete)(void *item) )
{
	if (set != NULL) {
		setnode_t *node;
		setnode_t *next;
		for (node = set->head; node != NULL; ) {
			if (itemdelete != NULL) {		    // if possible...
				(*itemdelete)(node->item);	    // delete node's item
			}
			next = node->next;	    // remember what comes next
			free(node);			    // free the node
			node = next;			    // and move on to next
		}
		free(set);
	}
}
