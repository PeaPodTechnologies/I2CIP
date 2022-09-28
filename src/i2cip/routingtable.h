#ifndef I2CIP_ROUTINGTABLE_H_
#define I2CIP_ROUTINGTABLE_H_

#include <utils/bst.h>
#include <utils/hashtable.h>
#include <i2cip/fqa.h>

#define I2CIP_ROUTING_NUMIDS  16

// BST of device IDs by FQA
typedef BST<const char*> i2cip_devicetree_t;
// BST node; key: FQA, value: ID
typedef BSTNode<const char*> i2cip_device_t;

// Hash table entry
typedef struct {
  // List of pointers to BSTNodes
  i2cip_device_t** devices;
  unsigned char numdevices;
} i2cip_devicegroup_t;
// Hash table of device groups by ID
typedef HashTable<i2cip_devicegroup_t> i2cip_devicetable_t;

// Route table
class RoutingTable {
  private:
    // Tables/trees are allocated STATICALLY, their entries are dynamic
    i2cip_devicetree_t devices = i2cip_devicetree_t();
    i2cip_devicetable_t groups = i2cip_devicetable_t(I2CIP_ROUTING_NUMIDS);
  public:
    /**
     * Allocates a slot for a new device and inserts it into the table.
     * @param fqa
     * @param id
     * @param overwrite
     * @return Pointer to the new device
     */
    i2cip_device_t* add(const i2cip_fqa_t& fqa, const char* id, bool overwrite = true);

    i2cip_devicegroup_t* RoutingTable::addGroup(const char* id, i2cip_fqa_t* fqas, unsigned char numdevices, bool overwrite = true);
    
    /**
     * Finds a device and removes it from the table.
     * @param fqa
     * @param id
     * @param overwrite
     * @return Boolean: was the element found?
     */
    bool RoutingTable::remove(const i2cip_fqa_t& fqa);

    i2cip_devicegroup_t* operator[](const char* id);

    const char* operator[](const i2cip_fqa_t& fqa);

};

#endif