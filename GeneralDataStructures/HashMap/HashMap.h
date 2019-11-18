/*
 * HashMap.h - modified from CS50 `HashMap` module by Shengsong Gao
 *
 * Type of Item: arbitrary pointer
 * Type of Key: integer
 */


#ifndef _HashMap_h
#define _HashMap_h

/**************** global types ****************/
typedef struct HashMap HashMap_t;  // opaque to users of the module

/**************** functions ****************/

/* Create a new (empty) HashMap; return NULL if error. */
HashMap_t *HashMap_new(const int num_slots);

/* Insert item, identified by key (integer), into the given HashMap.
 * Return ERROR if malloc failed, the key exists, or any parameter is NULL;
 * return 0 iff new item was inserted.
 */
int HashMap_insert(HashMap_t *hm, int key, void *item);

/* Return the item associated with the given key;
 * return NULL if HashMap is NULL or key is not found.
 */
void *HashMap_find(HashMap_t *hm, int key);

/* Iterate over all items in the table; in undefined order.
 * Call the given function on each item, with (arg, key, item).
 * If hm==NULL or itemfunc==NULL, do nothing.
 */
void HashMap_iterate(HashMap_t *hm, void *arg,
		       void (*itemfunc)(void *arg, int key, void *item) );

/* Removes and returns a specific item from the HashMap based on 
 * the key, if it exists; returns NULL for NULL hm or if fails
 * to find the corresponding item.
 */
void *HashMap_remove(HashMap_t *hm, int key);

/* Deletes the whole HashMap; ignores NULL hm.
 * Takes in a function for deleting each item (may be NULL).
 */
void HashMap_delete(HashMap_t *hm, void (*itemdelete)(void *item) );

#endif /* _HashMap_h */

