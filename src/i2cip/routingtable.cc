#include <i2cip/routingtable.h>

#include <stdlib.h>
#include <string.h>

i2cip_device_t* RoutingTable::add(const i2cip_fqa_t& fqa, const char* id, bool overwrite) {
  unsigned char numdevices = this->groups[id]->numdevices;
  
  // Allocate new devices
  i2cip_device_t** newdevicegroup = (i2cip_device_t**)malloc(sizeof(i2cip_device_t*)*(numdevices+1));

  // Append new device
  devices.insert(fqa, id);
  newdevicegroup[numdevices] = devices.find(fqa);

  // Copy old BSTNode pointers
  for(int i = 0; i < numdevices; i++) {
    newdevicegroup[i] = this->groups[id]->devices[i];
  }

  // Free old data
  free(this->groups[id]->devices);

  // Reassign
  this->groups[id]->numdevices = numdevices;
  this->groups[id]->devices = newdevicegroup;
  return newdevicegroup[numdevices];
}

bool RoutingTable::remove(const i2cip_fqa_t& fqa) {
  // Find in BST
  const char* id = this->devices[fqa];
  // Remove from hash table
  this->groups.remove(id);
  // Remove from BST
  this->devices.remove(fqa);
}

i2cip_devicegroup_t* RoutingTable::operator[](const char* id) {
  return this->groups[id];
}

const char* RoutingTable::operator[](const i2cip_fqa_t& fqa) {
  return this->devices[fqa];
}