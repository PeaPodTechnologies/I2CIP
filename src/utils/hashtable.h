#ifndef I2CIP_UTILS_HASHTABLE_H_
#define I2CIP_UTILS_HASHTABLE_H_

// Constants
#define HASHTABLE_OFFSET 31
#define HASHTABLE_SLOTS  16    // Number of unique ID slots to include in the hashtable (for optimization purposes)

// Hash table entry
template <typename T> class HashTableEntry {
  public:
    HashTableEntry(const char* key, T value, HashTableEntry<T>* last);
    ~HashTableEntry();

    const char* key;                // Pointer to the IMMUTABLE (copied) key - de/allocated on de/construction
    T value;                 // REFERENCE to the value - passed in
    HashTableEntry<T>* next;  // Pointer to the previous entry - passed in
};

// A hash table. Yup.
template <typename T> class HashTable {
  private:
    // Slots are static, entries are dynamic
    HashTableEntry<T>* hashtable[HASHTABLE_SLOTS] = { nullptr }; // FIXED POINTER to the entries - de/allocated on de/construction
  public:
    HashTable();
    ~HashTable();

    /**
     * Put {key: value} in the hash table.
     * @param key String key
     * @param value Pointer to value
     * @param overwrite Overwrite existing value if found? Default: `true`
     * @return Pointer to the new entry
     */
    HashTableEntry<T>* set(const char* key, T value, bool overwrite = true);

    /**
     * Look at the index for that key, down the chain until either key=key or next = nullptr
     * @param key to look for
     * @return Pointer if found, nullptr otherwise
     */
    HashTableEntry<T>* get(const char* key);

    /**
     * Index operator by key.
     * @param key
     * @return Pointer to entry
     */
    HashTableEntry<T>* operator[](const char* key);

    bool remove(const char* key);
};

#endif

#include <utils/hashtable.tpp>