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
	struct set **sets;
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
		// initialize the HashMap as a pointer to an array of pointers to a set
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
HashMap_insert(HashMap_t *hm, const char *key, void *item)
{
	if (hm != NULL && key != NULL && item != NULL) {
		// hash the key, insert at the corresponding set in the array, and return status
		// relies on the convinient set_insert function from the 'set' module
		return set_insert(hm->sets[JenkinsHash(key, hm->size)], key, item);
	}
	// some parameter is NULL, return ERROR
	return ERROR;
}

/**************** HashMap_find() ****************/
void *
HashMap_find(HashMap_t *hm, const char *key)
{
	// set_find actually checks if key is NULL, but we still check it here
	if (hm != NULL && key != NULL) {
		return set_find(hm->sets[JenkinsHash(key, hm->size)], key);
	}
	// HashMap or key is NULL
	return NULL;
}

/**************** HashMap_print() ****************/
void
HashMap_print(HashMap_t *hm, FILE *fp, void (*itemprint)(FILE *fp, const char *key, void *item) )
{
	if (fp != NULL) {
		if (hm != NULL) {
			fputc('[', fp);
			// if itemprint is NULL, only prints "[]"
			if (itemprint != NULL) {
				int i;
				for (i = 0; i < hm->size; i++) {
					// ask the function from the 'set' module to print the current set 
					set_print(hm->sets[i], fp, itemprint); 
					fputc(',', fp);
				}
			}
			fputc(']', fp);
		} else {
			fputs("(null)", fp);
		}
	}
}

/**************** HashMap_iterate() ****************/
void
HashMap_iterate(HashMap_t *hm, void *arg, void (*itemfunc)(void *arg, const char *key, void *item) )
{
	// again, set checks if itemfunc is NULL, but we still do it on this level
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


/**************** JenkinsHash() ****************/
unsigned long
JenkinsHash(const char *str, unsigned long mod)
{
	if (str == NULL || mod <= 1) {
		return 0;
	}

	size_t len = strlen(str);
	unsigned long hash = 0;
	
	int i;
	for (i = 0; i < len; i++) {
		hash += str[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);

	return (hash % mod);
}
