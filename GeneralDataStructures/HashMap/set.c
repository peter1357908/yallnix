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
	char *key;
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
set_insert(set_t *set, const char *key, void *item)
{
	if (set != NULL && key != NULL && item != NULL) {
		// iterate through the list to see if the key already exists
		// can't rely on set_find(), because it returns NULL when a key with item NULL exists
		setnode_t *node;
		for (node = set->head; node != NULL; node = node->next) {
			if (strcmp(node->key, key) == 0) {
				return ERROR-1;
			}
		}
		
		// allocate memory for a new node to be added to the list
		node = malloc(sizeof(setnode_t));
	
		if (node == NULL) {
			// error allocating memory for node; return ERROR
			return ERROR;
		} else {
			// the key doesn't exist, and node is allocated - fill it up.
			// first allocate space equal to length + 1 to account for null-terminator
			// then make own copy of key with strcpy, and add it to the head of the list
			node->key = calloc(strlen(key) + 1, sizeof(char));
			strcpy(node->key, key);
			node->item = item;
			
			node->next = set->head;
			set->head = node;
			
			return 0;
		}
	}

	// return ERROR-1 if any of the parameter is NULL
	return ERROR-1;
}

/**************** set_find() ****************/
void *
set_find(set_t *set, const char *key)
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
			if (strcmp(node->key, key) == 0) {
				return node->item;
			}
		}
		// key never matched, return NULL
		return NULL;
	}
}

/**************** set_print() ****************/
void
set_print(set_t *set, FILE *fp, void (*itemprint)(FILE *fp, const char *key, void *item) )
{
	if (fp != NULL && itemprint != NULL) {
		if (set != NULL) {
			fputc('{', fp);
			// if itemprint is NULL, only prints "{}"
			if (itemprint != NULL) {
				setnode_t *node;
				for (node = set->head; node != NULL; node = node->next) {
					// print this node 
					(*itemprint)(fp, node->key, node->item); 
					fputc(',', fp);
				}
			}
			fputc('}', fp);
		} else {
			fputs("(null)", fp);
		}
	}
}

/**************** set_iterate() ****************/
void
set_iterate(set_t *set, void *arg, void (*itemfunc)(void *arg, const char *key, void *item) )
{
	if (set != NULL && itemfunc != NULL) {
		// call itemfunc with arg, on each item
		setnode_t * node;
		for (node = set->head; node != NULL; node = node->next) {
		(*itemfunc)(arg, node->key, node->item); 
		}
	}
}

/**************** set_delete() ****************/
void 
set_delete(set_t *set, void (*itemdelete)(void *item) )
{
	if (set != NULL) {
		setnode_t *node;
		for (node = set->head; node != NULL; ) {
			free(node->key);
			if (itemdelete != NULL) {		    // if possible...
				(*itemdelete)(node->item);	    // delete node's item
			}
			setnode_t *next = node->next;	    // remember what comes next
			free(node);			    // free the node
			node = next;			    // and move on to next
		}
		free(set);
	}
}
