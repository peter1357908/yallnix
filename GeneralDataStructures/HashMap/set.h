/* 
 * set.h - header file for CS50 `set` module
 *
 * A *set* maintains an unordered collection of (key,item) pairs;
 * any given key can only occur in the set once. It starts out empty 
 * and grows as the caller inserts new (key,item) pairs.  The caller 
 * can retrieve items by asking for their key, but cannot remove or 
 * update pairs.  Items are distinguished by their key.
 *
 * David Kotz, April 2016, 2017
 * updated by Xia Zhou, July 2016
 *
 * Modified by Shengsong Gao; now:
 * Type of Item: arbitrary pointer
 * Type of Key: integer
 */

#ifndef _set_h
#define _set_h

/**************** global types ****************/
typedef struct set set_t;  // opaque to users of the module

/**************** functions ****************/

/* Creates a new (empty) set; returns NULL if error. */
set_t *set_new(void);

/* Inserts item, identified by a key (integer), into the given set.
 * Returns ERROR if malloc failed, the key exists, or any parameter is NULL;
 * returns 0 iff new item was inserted.
 */
int set_insert(set_t *set, int key, void *item);

/* Returns the item associated with the given key;
 * returns NULL if set is NULL or key is not found.
 */
void *set_find(set_t *set, int key);

/* Iterates over all items in the set, in undefined order.
 * Calls the given function on each item, with (arg, key, item).
 * If set==NULL or itemfunc==NULL, does nothing.
 */
void set_iterate(set_t *set, void *arg,
		 void (*itemfunc)(void *arg, int key, void *item) );

/* Removes and returns a specific item from the set based on 
 * the key, if it exists; returns NULL for NULL set or if fails
 * to find the corresponding item.
 */
void *set_remove(set_t *set, int key);

/* Deletes the whole set; ignores NULL set.
 * Takes in a function for deleting each item (may be NULL).
 */
void set_delete(set_t *set, void (*itemdelete)(void *item) );

#endif // _set_h
