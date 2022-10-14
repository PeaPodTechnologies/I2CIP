#ifndef I2CIP_ROUTINGTABLE_H_
#define I2CIP_ROUTINGTABLE_H_

#include <Arduino.h>

#include <utils/bst.h>
#include <utils/hashtable.h>
#include <i2cip/fqa.h>

// HashTable
  // HashTableEntry
  // key/ID (1) - heap
    // DeviceGroup - heap
      // key (ref to 1)
      // numdevices
      // devices[] (pointers to 2)

// BST
  // BSTNode (2) - heap
    // key/FQA
    // value (ref to 1)

// BST of device IDs by FQA
typedef BST<i2cip_fqa_t, const char*&> i2cip_devicetree_t;
// BST node; key: FQA, value: ID
typedef BSTNode<i2cip_fqa_t, const char*&> i2cip_device_t;

class DeviceGroup {
  public:
    const char*& key;
    i2cip_device_t** devices;
    uint8_t numdevices;

    DeviceGroup(const char*& key, i2cip_device_t** devices = nullptr, uint8_t numdevices = 0);

    void add(i2cip_device_t* device);
    void addGroup(i2cip_device_t** devices, uint8_t numdevices);
    void remove(i2cip_device_t* device);
    bool contains(i2cip_device_t* device);
};

typedef HashTable<DeviceGroup*> i2cip_devicetable_t;

class RoutingTable {
  private:
    // Tables/trees are allocated STATICALLY, their entries are dynamic
    i2cip_devicetree_t devices = i2cip_devicetree_t();
    i2cip_devicetable_t groups = i2cip_devicetable_t();

    HashTableEntry<DeviceGroup*>* addEmptyGroup(const char* id);
  public:
    /**
     * Allocates a slot for a new device and inserts it into the table.
     * @param fqa
     * @param id
     * @param overwrite
     * @return Pointer to the device group
     */
    const i2cip_device_t& add(const char* id, const i2cip_fqa_t& fqa, bool overwrite = true);

    const DeviceGroup& addGroup(const char* id, i2cip_fqa_t* fqas, uint8_t numdevices, bool overwrite = true);
    
    /**
     * Finds a device and removes it from the table.
     * @param fqa
     * @param id
     * @param overwrite
     * @return Boolean: was the element found?
     */
    bool remove(const i2cip_fqa_t& fqa);

    /**
     * Find a device group by ID.
     * @param id
     * @returns Pointer to the device group (`nullptr` if none)
     */
    DeviceGroup* operator[](const char* id);

    /**
     * Find a device ID by FQA.
     * @param fqa
     * @returns Pointer to the device ID (`nullptr` if none)
     */
    const char* operator[](const i2cip_fqa_t& fqa);

    const i2cip_devicetree_t& getDevices(void);

    const i2cip_devicetable_t& getDeviceGroups(void);
};

#endif