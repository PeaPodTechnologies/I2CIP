#ifndef I2CIP_HASHTABLE_H_
#define I2CIP_HASHTABLE_H_

// Constants
#define HASHTABLE_OFFSET 31
#define HASHTABLE_SLOTS  8    // Number of unique ID slots to include in the hashtable (for optimization purposes)

// Basic Hash Table implementation.

namespace I2CIP {
  class Module;
};

template <typename T> class HashTableEntry {
  public:
    HashTableEntry(const char* key, T* value, HashTableEntry<T>* last);
    ~HashTableEntry();

    const char* key;
    T* value;
    HashTableEntry<T>* next;

    String toString(void) const {
      String str;
      str += "{ ";
      str += this->key;
      str += " : ";
      str += String((uintptr_t)this->value, HEX);
      str += " }";
      if(this->next != nullptr) {
        str += " : ";
        str += this->next->toString();
      }
      return str;
    }
};

template <typename T> class HashTable {
  friend class I2CIP::Module; // Give Module direct access to the hashtable for destruction
  private:
    HashTableEntry<T>* hashtable[HASHTABLE_SLOTS] = { nullptr };

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
    HashTableEntry<T>* set(const char* key, T* value, bool overwrite = true);

    /**
     * Look at the index for that key, down the chain until either key=key or next = nullptr
     * @param key to look for
     * @return Pointer if found, nullptr otherwise
     */
    HashTableEntry<T>* get(const char* key);

    /**
     * Index operator by key. Shorthand for get(key)->value
     * @param key
     * @return Pointer to entry's value
     */
    T* operator[](const char* key);

    bool remove(const char* key);

    String toString(void) const {
      String str = "HashTable [";
      bool content = false;
      for (uint8_t i = 0; i < HASHTABLE_SLOTS; i++) {
        if (this->hashtable[i] != nullptr) {
          if(content) str += " ; ";
          content = true;
          str += this->hashtable[i]->toString();
        }
      }
      if(!content) {
        str += "Empty";
      }
      return str + "]";
    }
};

#include "hashtable.tpp"

#endif