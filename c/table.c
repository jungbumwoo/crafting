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
    Entry* tombstone = NULL;
    for (;;) {
        Entry* entry = &entries[index];

        // [[something], [something], [tombstone], [somethong or tombstone], NULL] 인 경우 NULL 일 경우에
        // tombstone을 반환해야함.
        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) {
                // Empty entry.
                return tombstone != NULL ? tombstone : entry;
            } else {
                // We found a tombstone.
                // 꼭 if 문 안쓰고 tombstone = entry 로 해도 될 것 같다?
                // if 문을 써서 최초 발견 tomstone이 사용되네
                if (tombstone == NULL) tombstone = entry;
            }
        } else if (entry->key == key) {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

bool tableGet(Table *table, ObjString* key, Value* value) {
    // if exists, store the resulting value in the value output parameter.
    if (table->count == 0) return false;

    Entry *entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    *value = entry->value;
    return true;
}

static void adjustCapacity(Table* table, int capacity) {
    // allocate new array to hold the new capacity
    Entry* entries = ALLOCATE(Entry, capacity);
    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    // copy the existing entries into the new array.
    table->count = 0;
    for (int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if (entry->key == NULL) continue;

        // dest: new one, entry: existing one.
        Entry* dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool tableSet(Table *table, ObjString* key, Value value) {

    // make sure table is big enough to hold new key and value.
    if (table->count + 1  > table -> capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustTableSize(table, capacity);
    }
    
    Entry* entry = findEntry(table->entries, table->capacity, key);

    bool isNewKey = entry->key == NULL;
    // tombstone 을 재활용하는 case는 count가 증가되지 않는다.
    if (isNewKey) table->count++;

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

void tableAddAll(Table* from, Table* to) {
    for (int i = 0; i < from->count; i++) {
        Entry* entry = &from->entries[i];

        if (entry->key != NULL) {
            tableSet(to, entry->key, entry->value);
        }
    }
}

ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash) {
    if (table->count == 0) return NULL;

    uint32_t index = hash % table->count;

    for (;;) {
        Entry* entry = &table->entries[index];

        if (entry->key != NULL) {
            if (IS_NIL(entry->value)) return NULL;
        } else if (entry->key->length == length &&
            entry->key->hash == hash &&
            memcmp(entry->key->chars, chars, length) == 0) {
            return entry->key;
        }

        index = (index + 1) % table->capacity;
    }

}