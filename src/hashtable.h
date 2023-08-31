#ifndef __SKOOMA_HASHTABLE_H__
#define __SKOOMA_HASHTABLE_H__

#include "common.h"
#include "value.h"

typedef struct {
  Value key;
  Value value;
} Entry;

typedef struct {
  UInt cap;
  UInt len;
  UInt left;
  uint8_t prime;
  Entry *entries;
} HashTable;

void HashTable_init(HashTable *table);
bool HashTable_insert(HashTable *table, Value key, Value value);
bool HashTable_remove(HashTable *table, Value key);
ObjString *HashTable_get_intern(HashTable *table, const char *str, size_t len,
                                uint64_t hash);
bool HashTable_get(HashTable *table, Value key, Value *out);
void HashTable_free(HashTable *table);

#endif