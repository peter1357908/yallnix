/*
 * HashMap.h - modified from CS50 `HashMap` module by Shengsong Gao
 */


#ifndef _HashMap_h
#define _HashMap_h

#include <stdio.h>

/**************** global types ****************/
typedef struct HashMap HashMap_t;  // opaque to users of the module

/**************** functions ****************/

/* Create a new (empty) HashMap; return NULL if error. */
HashMap_t *HashMap_new(const int num_slots);

/* Insert item, identified by key (string), into the given HashMap.
 * The key string is copied for use by the HashMap.
 * Return false if key exists in hm, any parameter is NULL, or error;
 * return true iff new item was inserted.
 */
int HashMap_insert(HashMap_t *hm, const char *key, void *item);

/* Return the item associated with the given key;
 * return NULL if HashMap is NULL, key is NULL, key is not found.
 */
void *HashMap_find(HashMap_t *hm, const char *key);

/* Print the whole table; provide the output file and func to print each item.
 * Ignore if NULL fp. Print (null) if NULL hm.
 * Print a table with no items if NULL itemprint.
 */
void HashMap_print(HashMap_t *hm, FILE *fp,
		     void (*itemprint)(FILE *fp, const char *key, void *item));

/* Iterate over all items in the table; in undefined order.
 * Call the given function on each item, with (arg, key, item).
 * If hm==NULL or itemfunc==NULL, do nothing.
 */
void HashMap_iterate(HashMap_t *hm, void *arg,
		       void (*itemfunc)(void *arg, const char *key, void *item) );

/* Delete the whole HashMap; ignore NULL hm.
 * Provide a function that will delete each item (may be NULL).
 */
void HashMap_delete(HashMap_t *hm, void (*itemdelete)(void *item) );

/*
 * jenkins_hash - Bob Jenkins' one_at_a_time hash function
 * @str: char buffer to hash (non-NULL)
 * @mod: desired hash modulus (>0)
 *
 * Returns hash(str) % mod.
 *
 * Reference: hmtp://www.burtleburtle.net/bob/hash/doobs.hmml
 */
unsigned long JenkinsHash(const char *str, unsigned long mod);

#endif /* _HashMap_h */

