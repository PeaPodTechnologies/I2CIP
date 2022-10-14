#include <i2cip/routingtable.h>

#include <stdlib.h>
#include <string.h>

#include <Arduino.h>

#include <utils/bst.h>
#include <utils/hashtable.h>
#include <i2cip/fqa.h>

DeviceGroup::DeviceGroup(const char*& key, i2cip_device_t** devices, uint8_t numdevices) : key(key), devices(devices), numdevices(numdevices) { }

void DeviceGroup::add(i2cip_device_t* device) {
  // TODO: no duplicates!!
  addGroup(&device, 1);
}

void DeviceGroup::addGroup(i2cip_device_t** devices, uint8_t numdevices) {
  uint8_t totaldevices = this->numdevices+numdevices;
  
  // Allocate new device pointer array
  i2cip_device_t** newdevicegroup = (i2cip_device_t**)malloc(sizeof(i2cip_device_t*)*totaldevices);
  
  if(this->devices != nullptr) {
    // Copy old addresses
    for(int i = 0; i < this->numdevices; i++) {
      newdevicegroup[i] = this->devices[i];
    }

    // Free old addresses, then the group
    free(this->devices);
  }

  // Append new devices
  for(int i = 0; i < numdevices; i++) {
    newdevicegroup[this->numdevices+i] = devices[i];
  }

  this->devices = newdevicegroup;
  this->numdevices = totaldevices;
}

void DeviceGroup::remove(i2cip_device_t* device) {  
  if(this->devices == nullptr) return;

  // Allocate new device pointer array
  i2cip_device_t** newdevicegroup = (i2cip_device_t**)malloc(sizeof(i2cip_device_t*)*(this->numdevices-1));
  
  // Copy old addresses
  bool skip = false;
  for(int i = 0; i < this->numdevices-1; i++) {
    if(this->devices[i]->key == device->key) skip = true;
    newdevicegroup[i] = this->devices[i + (skip ? 1 : 0)];
  }

  // Free old addresses, then the group
  free(this->devices);

  this->devices = newdevicegroup;
  this->numdevices--;
}

HashTableEntry<DeviceGroup*>* RoutingTable::addEmptyGroup(const char* id) {
  HashTableEntry<DeviceGroup*>* group = groups.set(id, nullptr);
  DeviceGroup* newgroup = new DeviceGroup(group->key);
  group->value = newgroup;
  return group;
}

const i2cip_device_t& RoutingTable::add(const char* id, const i2cip_fqa_t& fqa, bool overwrite) {
  i2cip_device_t* device = this->devices[fqa];
  // Remove device IFF:
  // - This address is in the BST, and;
  // - The associated key is different, and;
  // - Overwrite is enabled.
  if(device != nullptr && strcmp(device->value, id) != 0 && overwrite) {
    this->groups[device->value]->value->remove(device);
    this->devices.remove(device->key);
  }

  // Add device to THIS group IFF:
  // - The deviceis NOT in the BST, or;
  // - Overwrite is enabled.
  if(device == nullptr || overwrite) {
    HashTableEntry<DeviceGroup*>* entry = this->groups[id];
    if(entry == nullptr) entry = addEmptyGroup(id);

    entry->value->add(devices.insert(fqa, entry->key, true));
  }

  return *this->devices[fqa];
}

const DeviceGroup& RoutingTable::addGroup(const char* id, i2cip_fqa_t* fqas, uint8_t numdevices, bool overwrite) {
  HashTableEntry<DeviceGroup*>* entry = this->groups[id];
  if(entry == nullptr) entry = addEmptyGroup(id);
  uint8_t totaldevices = entry->value->numdevices+numdevices;
  
  // Allocate new device pointer array
  i2cip_device_t** newdevicegroup = (i2cip_device_t**)malloc(sizeof(i2cip_device_t*)*totaldevices);
  
  if(entry->value->devices != nullptr) {
    // Copy old devices
    for(int i = 0; i < entry->value->numdevices; i++) {
      newdevicegroup[i] = entry->value->devices[i];
    }

    // Free old devices
    free(entry->value->devices);
  }

  // Append new devices
  int addedDevices = 0;
  for(int i = 0; i < numdevices; i++) {
    i2cip_device_t* device = this->devices[fqas[i]];
    // Remove device IFF:
    // - This address is in the BST, and;
    // - The associated key is different, and;
    // - Overwrite is enabled.
    if(device != nullptr && strcmp(device->value, id) != 0 && overwrite) {
      this->groups[device->value]->value->remove(device);
      this->devices.remove(device->key);
    }

    // Add device to THIS group IFF:
    // - The deviceis NOT in the BST, or;
    // - Overwrite is enabled.
    if(device == nullptr || overwrite)  {
      newdevicegroup[entry->value->numdevices+addedDevices] = devices.insert(fqas[i], entry->key, true);
      addedDevices++;
    }
  }

  entry->value->devices = newdevicegroup;
  entry->value->numdevices += addedDevices;

  return *(entry->value);
}

bool RoutingTable::remove(const i2cip_fqa_t& fqa) {
  // Find in BST
  i2cip_device_t* device = this->devices[fqa];
  if(device == nullptr) return false;
  // Remove from hash table
  HashTableEntry<DeviceGroup*>* group = this->groups.get(device->value);
  if(group != nullptr) {
    // Found in hashtable; remove from BST
    group->value->remove(device);
    this->devices.remove(fqa);
    return true;
  }
  return false;
}

DeviceGroup* RoutingTable::operator[](const char* id) {
  HashTableEntry<DeviceGroup*>* group = this->groups[id];
  if(group == nullptr) {
    return nullptr;
  }
  return group->value;
}

const char* RoutingTable::operator[](const i2cip_fqa_t& fqa) {
  i2cip_device_t* device = this->devices[fqa];
  if(device == nullptr) return nullptr;
  return device->value;
}