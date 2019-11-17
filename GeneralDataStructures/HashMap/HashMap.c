/*
 * HashMap.c - modified from CS50 `HashMap` module by Shengsong Gao
 * 
 * Refer to HashMap.h for usage details
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yalnix.h>  // for ERROR
#include "set.h"
#include "HashMap.h"

/**************** global types ****************/
typedef struct HashMap {
	int size;
	struct set **sets;  // an array of set pointers
} HashMap_t;


/**************** HashMap_new() ****************/
HashMap_t *
HashMap_new(const int num_slots)
{
	HashMap_t *hm = malloc(sizeof(HashMap_t));

	if (hm == NULL) {
		return NULL; // error allocating HashMap
	} else {
		hm->size = num_slots;
		// initialize the HashMap as an array of set pointers
		hm->sets = calloc(num_slots, sizeof(struct set*));
		// and initialize each element of the array to be an empty set
		// another way is to initialize as necessary, in the HashMap_insert function
		int i;
		for (i = 0; i < num_slots; i++) {
			hm->sets[i] = set_new();
		}
		return hm;
	}
}

/**************** HashMap_insert() ****************/
int
HashMap_insert(HashMap_t *hm, int key, void *item)
{
	if (hm != NULL && item != NULL) {
		// hash the key, insert at the corresponding set in the array, and return status
		// rely on set_insert() from the 'set' module
		return set_insert(hm->sets[(key % (hm->size))], key, item);
	}
	// some parameter is NULL, return ERROR
	return ERROR;
}

/**************** HashMap_find() ****************/
void *
HashMap_find(HashMap_t *hm, int key)
{
	if (hm != NULL) {
		return set_find(hm->sets[(key % (hm->size))], key);
	}
	// HashMap is NULL
	return NULL;
}

/**************** HashMap_iterate() ****************/
void
HashMap_iterate(HashMap_t *hm, void *arg, void (*itemfunc)(void *arg, int key, void *item) )
{
	// set_iterate() checks if itemfunc is NULL, but we still do it on this level
	if (hm != NULL && itemfunc != NULL) {
		int i;
		for (i = 0; i < hm->size; i++) {
			set_iterate(hm->sets[i], arg, itemfunc);
		}
	}
}

/**************** HashMap_delete() ****************/
void 
HashMap_delete(HashMap_t *hm, void (*itemdelete)(void *item) )
{
	if (hm != NULL) {
		int i;
		for (i = 0; i < hm->size; i++) {
			set_delete(hm->sets[i], itemdelete);	    // delete the set's content...
		}
		free(hm->sets);
		free(hm);
	}
}
