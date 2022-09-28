#include <utils/hashtable.h>

#include <stdlib.h>
#include <string.h>

#include <Arduino.h>

// DECLARATIONS

static const char* strdup(const char* s);

unsigned hash(char* s, uint8_t hashsize);

// HASH TABLE ENTRY

template <typename U> HashTableEntry<U>::HashTableEntry(const char* key, U value, HashTableEntry<U>* last) : key(strdup(key)), value(value), next(last) { }

template <typename U> HashTableEntry<U>::~HashTableEntry() {
  // Free our key then trigger the next entry
  free(this->key);
  delete(this->next);
}

// HASH TABLE

template <typename T> HashTable<T>::HashTable(uint8_t hashsize) : hashsize(hashsize) { }

template <typename T> HashTable<T>::~HashTable() {
  // Free all allocated entries (and their keys) recursively; slots are static
  for (unsigned char i = 0; i < this->hashsize; i++) {
    delete(this->hashtable[i]);
  }
}

template <typename T> T* HashTable<T>::operator[](const char* key) {
  HashTableEntry<T>* entry = get(key);
  if (entry == nullptr) return nullptr;
  return &(entry->value);
}

// Public methods

template <typename T> HashTableEntry<T>* HashTable<T>::set(const char* key, T value, bool overwrite) {
  HashTableEntry<T>* head = get(key);

  // Match found?
  if (head != nullptr) {
    if (overwrite) head->value = value;
    return head;
  }
  // No match, allocate new; point "next" to the top entry
  unsigned index = hash(key);
  head = new HashTableEntry<T>(key, value, *(hashtable[index]));

  // If allocation was successful:
  if (head != nullptr && head->key != nullptr) {
    // Point the top of the hashtable to the new entry
    hashtable[index] = head;
  }

  return head;
}

template <typename T> HashTableEntry<T>* HashTable<T>::get(const char* key) {
  HashTableEntry<T>* np;
  for (np = hashtable[hash(key)]; np != nullptr; np = np->next) {
    if (strcmp(key, np->key) == 0) {
      return np; /* found */
    }
  }
  return nullptr; /* not found */
}

template <typename T> bool HashTable<T>::remove(const char* key) {
  // Find ptr.next.key == key
  unsigned index = hash(key);
  HashTableEntry<T>* ptr = hashtable[index];

  // Check top-level
  if (strcmp(key, ptr->key) == 0) {
    // Found; point the slot to next
    hashtable[index] = ptr->next;
    ptr->next = nullptr;
    delete(ptr);
    return true;
  }

  for (; ptr->next != nullptr; ptr = ptr->next) {
    if (strcmp(key, ptr->next->key) == 0) {
      // Found; point the preceding entry to the subsequent one
      HashTableEntry<T>* entry = ptr->next;
      ptr->next = ptr->next->next;
      entry->next = nullptr;
      delete(entry);
      return true;
    }
  }
  // Not found
  return false;
}

// HELPER FUNCTIONS

static const char* strdup(const char* s) {
  char* p = (char*)malloc(strlen(s)+1); // +1 for '\0' terminator
  if (p != nullptr) {
    strcpy(p, s);
  }
  return p;
}

static unsigned hash(char* s, uint8_t hashsize) {
  unsigned index;
  for (index = 0; *s != '\0'; s++) {
    index = *s + HASHTABLE_OFFSET * index;
  }
  return index % hashsize;
}