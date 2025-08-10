#include <stdio.h>

typedef struct ht ht;

ht *ht_create(void);

/** Free memory allocated for the table, including allocated keys */
void ht_destroy(ht *table);

/** Get item with given key from hash table.
 *  Return value: data stored with ht_set or NULL if not found.
 */
void *ht_get(ht *table, const char *key);

/** Set item with given key to value (which must not be NULL).
 *  The key is copied to newly allocated memory.
 *  Return the adress of the newly copied key, or NULL if out of memory.
 */
const char *ht_set(ht *table, const char *key, void *value);

size_t ht_length(ht *table);

/** Hash Table Iterator: create with ht_iterator, iterate with ht_next */
typedef struct
{
    const char *key;
    void *value;

    // don't use these directly
    ht *_table;
    size_t _index;
} hti;

/** Returns new HT Iterator */
hti ht_iterator(ht *table);

/** Moves iterator to next item in hash table, update iterator's key and value to current item, and return true.
 *  If there are no more items it will return false.
 *  Don't call ht_set during iteration.
 */
bool ht_next(hti *it);
