#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table *table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeTable(Table *table) {
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

static Entry* findEntry(Entry* entries, int capacity, ObjString* key) {
    /*
    Responsible for taking a key and an array of bukets and figuring out which bucket the entry belongs in.
    Used for finding existing entries and to decide where to insert new ones.
    Linear probing and collision handling come into play.
    */
    
    uint32_t index = key->hash % capacity;
    for (;;) {
        Entry* entry = &entries[index];

        if (entry->key == key || entry->key == NULL) {
            /*
            If we go past the end of the array, we're done.

            If the key is NULL, then the bucket is empty.
            If findEntry() to look up something in the hash table, this means it isn't there. 
            If we're using it to insert, it means we've found a place to add it.
            
            return either insert something into it or read from it.
            */
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

bool tableSet(Table *table, ObjString* key, Value value) {
    if (table->count + 1  > table -> capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustTableSize(table, capacity);
    }
    
    Entry* entry = findEntry(table->entries, table->capacity, key);

    bool isNewKey = entry->key == NULL;
    if (isNewKey) table->count++;

    entry->key = key;
    entry->value = value;
    return isNewKey;
}