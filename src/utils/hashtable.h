#ifndef I2CIP_UTILS_HASHTABLE_H_
#define I2CIP_UTILS_HASHTABLE_H_

// Constants
#define HASHTABLE_OFFSET 31

// Hash table entry
template <typename U> class HashTableEntry {
  public:
    HashTableEntry(const char* key, U value, HashTableEntry<U>* last);
    ~HashTableEntry();

    const char* key;                // Pointer to the IMMUTABLE (copied) key - de/allocated on de/construction
    U value;                 // FIXED POINTER to the value - passed in
    HashTableEntry<U>* const next;  // FIXED POINTER to the previous entry - passed in
};

// A hash table. Yup.
template <typename T> class HashTable {
  private:
    const unsigned char hashsize;     // IMMUTABLE length of entries - passed in
    // unsigned int length = 0;                // number of items in hash table

    // Slots are static, entries are dynamic
    HashTableEntry<T>* const hashtable[hashsize]; // FIXED POINTER to the entries - de/allocated on de/construction
  public:
    HashTable(unsigned char hashsize);
    ~HashTable();

    /**
     * Index operator by key.
     * @param key
     * @return Pointer to value, or nullptr if entry not found
     */
    T* operator[](const char* key);

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

    bool remove(const char* key);
};

#endif