#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "ht.h"

/*
 * Simple Hashtable Implementation in C
 *
 * This file provides a basic hash table (associative array) implementation
 * using open addressing and linear probing for collision resolution.
 * Keys are strings, and values are generic pointers (void *).
 *
 * The hash function used is FNV-1a, which is fast and has good distribution.
 *
 * Usage:
 * ht *table = ht_create();
 * ht_set(table, "key", value);
 * void *value = ht_get(table, "key");
 * ht_destroy(table);
 *
 * Iterator Usage:
 * hti it = ht_iterator(table);
 * while (ht_next(&it)) {
 * printf("Key: %s, Value: %p\n", it.key, it.value);
 * }
 */

// --- Constants for FNV-1a Hash Function ---
#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

// --- Hashtable Entry ---
// Each entry holds a key-value pair.
typedef struct
{
    const char *key;
    void *value;
} ht_entry;

// --- Hashtable Structure ---
// The hashtable itself contains an array of entries, its capacity, and current length.
struct ht
{
    ht_entry *entries;
    size_t capacity; // Total number of slots in the table
    size_t length;   // Number of key-value pairs stored
};

// --- Hashtable Iterator Structure ---
struct hti
{
    const ht *_table; // Pointer to the hash table being iterated
    size_t _index;    // Current index in the table's entries array
    const char *key;  // Key of the current entry (updated by ht_next)
    void *value;      // Value of the current entry (updated by ht_next)
};

// --- Initial Capacity ---
// The table starts with this many slots and can grow as needed.
// A capacity with power of 2 is defined to help with performance
#define INITIAL_CAPACITY 16

/*
 * hash_key
 * ----------
 * Computes the FNV-1a hash for a given string key.
 * Returns a 64-bit unsigned integer hash value.
 */
static uint64_t hash_key(const char *key)
{
    uint64_t hash = FNV_OFFSET;
    for (const char *p = key; *p; p++)
    {
        hash ^= (uint64_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }
    return hash;
}

/*
 * ht_create
 * ----------
 * Allocates and initializes a new hashtable.
 * Returns a pointer to the new table, or NULL on failure.
 */
ht *ht_create(void)
{
    ht *table = malloc(sizeof(ht));
    if (table == NULL)
        return NULL;

    table->length = 0;
    table->capacity = INITIAL_CAPACITY;
    table->entries = calloc(table->capacity, sizeof(ht_entry));

    if (table->entries == NULL)
    {
        free(table);
        return NULL;
    }
    return table;
}

/*
 * ht_destroy
 * ----------
 * Frees all memory used by the hashtable, including keys and entries.
 * After calling this, the table pointer is invalid.
 */
void ht_destroy(ht *table)
{
    if (table == NULL)
        return;

    for (size_t i = 0; i < table->capacity; i++)
    {
        // Only free if a key exists at this slot
        if (table->entries[i].key != NULL)
        {
            free((void *)table->entries[i].key); // Free each key string allocated by strdup
        }
    }

    free(table->entries); // Free the entries array
    free(table);          // Free the table structure
}

/*
 * ht_get
 * ----------
 * Looks up a key in the hashtable and returns its value.
 * Returns NULL if the key is not found.
 *
 * Uses linear probing to resolve collisions.
 */
void *ht_get(ht *table, const char *key)
{
    if (table == NULL || key == NULL)
        return NULL;

    uint64_t hash = hash_key(key);
    size_t index = (size_t)(hash & (uint64_t)(table->capacity - 1)); // the bitwise & here works as a faster modulo operator.

    // Probe until we find the key or hit an empty slot
    while (table->entries[index].key != NULL)
    {
        if (strcmp(key, table->entries[index].key) == 0)
            return table->entries[index].value;

        index++;
        if (index >= table->capacity)
            index = 0; // Wrap around to the start
    }
    return NULL; // Not found
}

/*
 * ht_set_entry
 * ----------
 * Helper function to set a new entry or update an existing one in a given array of entries.
 * This function is used by ht_set and ht_expand.
 * If plength is not NULL, it means a new unique key is being inserted,
 * so the key string is duplicated and the length counter is incremented.
 * Returns a pointer to the stored key string on success, NULL on failure (e.g., strdup fails).
 */
static const char *ht_set_entry(ht_entry *entries, size_t capacity, const char *key, void *value, size_t *plength)
{
    uint64_t hash = hash_key(key);
    size_t index = (size_t)(hash & (uint64_t)(capacity - 1));

    while (entries[index].key != NULL)
    {
        if (strcmp(key, entries[index].key) == 0)
        {
            entries[index].value = value;
            return entries[index].key;
        }

        index++;
        if (index >= capacity)
            index = 0; // Wrap around to the start of the array
    }

    // If plength is not NULL, this is a new unique insertion (not a rehash during expansion)
    if (plength != NULL)
    {
        // Duplicate the key string to ensure the table owns its memory
        char *new_key_copy = strdup(key);
        if (new_key_copy == NULL)
            return NULL;

        key = new_key_copy;

        (*plength)++;
    }

    entries[index].key = key;
    entries[index].value = value;
    return key;
}

/*
 * ht_expand
 * ----------
 * Doubles the capacity of the hashtable and rehashes all existing entries
 * into the new, larger array.
 * Returns true on success, false on failure (e.g., memory allocation fails).
 */
static bool ht_expand(ht *table)
{
    size_t new_capacity = table->capacity * 2;
    if (new_capacity < table->capacity)
    {
        fprintf(stderr, "Error: Hashtable capacity overflow during expansion.\n");
        return false;
    }

    ht_entry *new_entries = calloc(new_capacity, sizeof(ht_entry));
    if (new_entries == NULL)
    {
        fprintf(stderr, "Error: Failed to allocate memory for new hashtable entries.\n");
        return false;
    }

    // Rehash all existing entries from the old table into the new table
    for (size_t i = 0; i < table->capacity; i++)
    {
        ht_entry entry = table->entries[i];
        if (entry.key != NULL)
        {
            // Reinsert the entry into the new array.
            // Pass NULL for plength as we are not incrementing the length
            // and not duplicating keys (they are already owned).
            ht_set_entry(new_entries, new_capacity, entry.key, entry.value, NULL);
        }
    }

    // Free the old entries array and update the table with the new array
    free(table->entries);
    table->entries = new_entries;
    table->capacity = new_capacity;

    return true;
}

/** Set value in the table.
 * This function takes the table, the value to map to the key, and of course the key.
 * Its main function it's to check wheter the inserted value is valid.
 * After the insertion we check for the size of the table, and we decide wether to expand it or not.
 * Setting the value to the table is delegated to the ht_set_entry helper function.
 * The expansion of the table is delegated to the ht_expand function.
 * Returns a pointer to the stored key string on success, NULL on failure.
 */
const char *ht_set(ht *table, const char *key, void *value)
{
    assert(value != NULL); // (debug builds)
    if (table == NULL || key == NULL || value == NULL)
        return NULL;

    // Check if the table needs to expand. Load factor is length / capacity.
    // Expand when length reaches 50% of capacity to maintain good performance.
    if (table->length >= table->capacity / 2)
    {
        if (!ht_expand(table))
            return NULL;
    }

    return (char *)ht_set_entry(table->entries, table->capacity, key, value, &table->length);
}

/*
 * ht_length
 * ----------
 * Returns the current number of key-value pairs stored in the hashtable.
 */
size_t ht_length(ht *table)
{
    if (table == NULL)
        return 0;
    return table->length;
}

/*
 * ht_iterator
 * ----------
 * Initializes and returns a new hashtable iterator.
 */
hti ht_iterator(ht *table)
{
    hti it;
    it._table = table;
    it._index = 0;
    it.key = NULL;
    it.value = NULL;
    return it;
}

/*
 * ht_next
 * ----------
 * Advances the iterator to the next entry in the hashtable.
 * Returns true if there is a next entry, false otherwise.
 * Updates the iterator's key and value fields.
 */
bool ht_next(hti *it)
{
    if (it == NULL || it->_table == NULL)
        return false;

    ht *table = (ht *)it->_table;

    while (it->_index < table->capacity)
    {
        size_t i = it->_index;
        it->_index++;

        if (table->entries[i].key != NULL)
        {
            ht_entry entry = table->entries[i];
            it->key = entry.key;
            it->value = entry.value;
            return true;
        }
    }

    it->key = NULL; // No more entries, clear iterator's key and value
    it->value = NULL;
    return false;
}

int main(void) {}