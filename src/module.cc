#include "module.h"

#include "debug.h"

// #ifndef I2CIP_MODULE_T_FIX
// #define I2CIP_MODULE_T_FIX
// template <> i2cip_errorlevel_t I2CIP::Module::operator()<short unsigned int>(short unsigned int* ptr, bool update, i2cip_args_io_t args, Stream& out) { return I2CIP_ERR_HARD; } // This is the new COCONUT.PNG
// template <> void I2CIP::Module::printCache<short unsigned int>(short unsigned int* that, Stream& out) { return; }
// template <> void I2CIP::Module::toString<short unsigned int>(short unsigned int* that, Stream& out) { return; }
// #endif

using namespace I2CIP;

// Globals

_NullStream NullStream;

// ========== DEVICE GROUP ==========

DeviceGroup::DeviceGroup(const i2cip_id_t& key, factory_device_t factory, jsonhandler_device_t handler, cleanup_device_t cleanup) : key(key), factory(factory), handler(handler), cleanup(cleanup) {
  for(uint8_t i = 0; i < I2CIP_DEVICES_PER_GROUP; i++) {
    devices[i] = nullptr;
  }

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("Constructed DeviceGroup('"));
    I2CIP_DEBUG_SERIAL.print(key);
    I2CIP_DEBUG_SERIAL.print(F("' @0x"));
    I2CIP_DEBUG_SERIAL.print((uintptr_t)key, HEX);
    I2CIP_DEBUG_SERIAL.print(F(", Device* (*)(fqa) @0x"));
    I2CIP_DEBUG_SERIAL.print((uintptr_t)factory, HEX);
    I2CIP_DEBUG_SERIAL.print(F(")\n"));
    DEBUG_DELAY();
  #endif
}

DeviceGroup::~DeviceGroup() { 
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("~DeviceGroup() '"));
    I2CIP_DEBUG_SERIAL.print(key);
    I2CIP_DEBUG_SERIAL.print(F("' @0x"));
    I2CIP_DEBUG_SERIAL.print((uintptr_t)(this), HEX);
    I2CIP_DEBUG_SERIAL.print(F(" x"));
    I2CIP_DEBUG_SERIAL.println(numdevices);
    DEBUG_DELAY();
  #endif
  for(uint8_t i = 0; i < I2CIP_DEVICES_PER_GROUP; i++) {
    if(this->devices[i] != nullptr) {
      i2cip_fqa_t fqa = this->devices[i]->getFQA();
      I2CIP::devicetree.remove(fqa);
      delete(this->devices[i]);
      this->devices[i] = nullptr;
    }
  }
}

bool DeviceGroup::add(Device* device) {
  if(device == nullptr) return false;
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("DeviceGroup (ID '"));
    I2CIP_DEBUG_SERIAL.print(this->key);
    I2CIP_DEBUG_SERIAL.print(F("' @0x"));
    I2CIP_DEBUG_SERIAL.print((uintptr_t)this->key, HEX);
    I2CIP_DEBUG_SERIAL.print(F(") Add Device @0x"));
    I2CIP_DEBUG_SERIAL.print((uintptr_t)(device), HEX);
    I2CIP_DEBUG_SERIAL.print(F(" (ID '"));
    I2CIP_DEBUG_SERIAL.print(device->getID());
    I2CIP_DEBUG_SERIAL.print(F("' @0x"));
    I2CIP_DEBUG_SERIAL.print((uintptr_t)(device->getID()), HEX);
    I2CIP_DEBUG_SERIAL.print(")");
    DEBUG_DELAY();
  #endif
  // Temporarily Disabled - I GUESS!
  if(strcmp(device->getID(), this->key) != 0) {
    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F(": Failed; '"));
      I2CIP_DEBUG_SERIAL.print(this->key);
      I2CIP_DEBUG_SERIAL.print(F("' != '"));
      I2CIP_DEBUG_SERIAL.print(device->getID());
      I2CIP_DEBUG_SERIAL.print("'\n");
      DEBUG_DELAY();
    #endif
    return false;
  } // else {
  //   // Todo: What was this for?
  // }

  // if(this->contains(&device)) return true; // Already added
  // if(this->contains(device->getFQA()) == device) return true; // Already added
  if(this->contains(device)) {
    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F(": Found; Do Nothing\n"));
      DEBUG_DELAY();
    #endif
    return true;
  } // Already added
  
  unsigned int n = 0;
  while(this->devices[n] != nullptr) { n++; if(n > I2CIP_DEVICES_PER_GROUP) return false;}

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F(": Pass; Now x"));
    I2CIP_DEBUG_SERIAL.println((n+1));
    DEBUG_DELAY();
  #endif

  // Append new devices
  this->devices[n] = device;
  this->numdevices = (n + 1);
  return true;
}

bool DeviceGroup::addGroup(Device* devices[], uint8_t numdevices) {
  unsigned int n = 0;
  while(this->devices[n] != nullptr) { n++; if(n + numdevices > I2CIP_DEVICES_PER_GROUP) return false; }
  
  // Append new devices
  for(unsigned int i = 0; i < numdevices && (n + i) < I2CIP_DEVICES_PER_GROUP; i++) {
    this->devices[n+i] = devices[i];
    this->numdevices++;
  }
  return true;
}

void DeviceGroup::remove(Device* device) {
  if(device == nullptr) return;
  bool swap = false;
  for(int i = 0; i < this->numdevices-1; i++) {
    if(swap) {
      this->devices[i - 1] = this->devices[i];
    }
    if(this->devices[i]->getFQA() == device->getFQA()) { 
      this->devices[i] = nullptr;
      this->numdevices--;
      swap = true;
    }
  }
}

bool DeviceGroup::contains(Device* device) const {
  if(device == nullptr || (device->getID() != this->key && strcmp(device->getID(), this->key) != 0)) return false;
  for(int i = 0; i < this->numdevices; i++) {
    if(this->devices[i] == nullptr) continue;
    if(this->devices[i] == device || this->devices[i]->getFQA() == device->getFQA()) return true;
  }
  return false;
}

Device* DeviceGroup::operator[](const i2cip_fqa_t& fqa) const {
  for(int i = 0; i < this->numdevices; i++) {
    if(this->devices[i]->getFQA() == fqa) return this->devices[i];
  }
  return nullptr;
}

Device* DeviceGroup::operator()(i2cip_fqa_t fqa) {
  Device* device = this->operator[](fqa);
  if(device != nullptr) return device;
  
  device = this->factory(fqa);

  if(device != nullptr) {
    bool b = this->add(device);
    
    if(!b) {
      // Serial.println("MEGAFAIL");
      delete device;
      return nullptr;
    }
  }

  return device;
}

void DeviceGroup::unready(void) {
  for(uint8_t i = 0; i < I2CIP_DEVICES_PER_GROUP; i++) {
    if(this->devices[i] != nullptr) {
      this->devices[i]->unready();
    }
  }
}

// DeviceGroup& DeviceGroup::operator=(const DeviceGroup& rhs) {
//   for(unsigned char i = 0; i < I2CIP_DEVICES_PER_GROUP; i++) this->devices[i] = rhs.devices[i];
//   this->numdevices = rhs.numdevices;
//   this->factory = rhs.factory;
//   this->key = rhs.key;
//   return *this;
// }

// ========== MODULE ==========

Module::Module(const i2cip_fqa_t& eeprom_fqa) : wire(I2CIP_FQA_SEG_I2CBUS(eeprom_fqa)), mux(I2CIP_FQA_SEG_MODULE(eeprom_fqa)), eeprom(new EEPROM(eeprom_fqa)) {
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("Module "));
    I2CIP_DEBUG_SERIAL.print(mux, HEX);
    I2CIP_DEBUG_SERIAL.print(F(" Constructed, EEPROM @0x"));
    I2CIP_DEBUG_SERIAL.println((uintptr_t)eeprom, HEX);
    DEBUG_DELAY();
  #endif

  // eeprom->clearCache(); // ensure cache is nullptr
  // eeprom->resetFailsafe(); // ensure default EEPROM buffer is cached for next write

  // !! Super important: prevents EEPROM from being overwritten during call operator update
  // eeprom->removeOutput(); // on second thought, this is bad form

  // NOTE: I MOVED THIS STEP TO DISCOVER, FLAGGED WITH `eeprom_added`, & MADE DEVICEGROUPFACTORY PURE VIRTUAL
  // This potentially useful if I decide to ping in deviceFactory
  // this->add(*eeprom); // Add EEPROM to module - note that this will call Module::deviceGroupFactory()
}

// Module::Module(const i2cip_fqa_t& eeprom_fqa) : Module(I2CIP_FQA_SEG_I2CBUS(eeprom_fqa), I2CIP_FQA_SEG_MODULE(eeprom_fqa), I2CIP_EEPROM_ADDR) { }
Module::Module(const uint8_t& wire, const uint8_t& mux, const uint8_t& eeprom_addr) : Module(I2CIP_FQA_CREATE(wire, mux, I2CIP_MUX_BUS_DEFAULT, eeprom_addr)) { }

Module::~Module() {
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.println(F("~Module()"));
    DEBUG_DELAY();
  #endif
}

void Module::toJSON(JsonObject obj, bool pingFilter) const {
  for(uint8_t i = 0; i < HASHTABLE_SLOTS; i++) {
    HashTableEntry<DeviceGroup>* ptr = this->devicegroups.hashtable[i];
    do{
      if(ptr != nullptr) {
        DeviceGroup* group = ptr->value;
        if(group == nullptr) continue;
        if(group->getNumDevices() == 0) {
          ptr = ptr->next;
          continue; // Skip empty groups
        }
        JsonArray arr = obj[group->key].to<JsonArray>();
        for(uint8_t j = 0; j < group->getNumDevices(); j++) {
          Device* d = group->getDevice(j);
          if(d != nullptr) {
            if(pingFilter) {
              i2cip_errorlevel_t errlev = d->pingTimeout();
              if(errlev != I2CIP_ERR_NONE) {
                continue; // Skip devices that are not pingable
              }
            }
            arr.add(d->getFQA());
          }
        }
        ptr = ptr->next;
      }
    } while(ptr != nullptr);
  }
}

// DeviceGroup* Module::deviceGroupFactory(const i2cip_id_t& id) {
//   #ifdef I2CIP_DEBUG_SERIAL
//     DEBUG_DELAY();
//     I2CIP_DEBUG_SERIAL.print(F("-> DeviceGroup Factory Not Implemented!\n"));
//     DEBUG_DELAY();
//   #endif

//   return nullptr;
// }

i2cip_errorlevel_t Module::discoverEEPROM(bool recurse) {
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("-> Module "));
    I2CIP_DEBUG_SERIAL.print(getModuleNum(), HEX);
    I2CIP_DEBUG_SERIAL.print(F(" Discovering...\n"));
    DEBUG_DELAY();
  #endif

  if(I2CIP_FQA_SEG_MODULE(this->eeprom->getFQA()) == I2CIP_MUX_NUM_FAKE) {
    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("Invalid Fake Module Rejected\n"));
      DEBUG_DELAY();
    #endif
    return I2CIP_ERR_HARD;
  }

  // 1. EEPROM MODULE OOP

  if(!this->eeprom_added) { 

    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("-> First-Time EEPROM Addition @0x"));
      I2CIP_DEBUG_SERIAL.println((uintptr_t)eeprom, HEX);
      DEBUG_DELAY();
    #endif
    bool r = this->add(this->eeprom, true);
    if(!r) {
      #ifdef I2CIP_DEBUG_SERIAL
        DEBUG_DELAY();
        I2CIP_DEBUG_SERIAL.println(F("-> ABORT!"));
        DEBUG_DELAY();
      #endif
      return I2CIP_ERR_SOFT;
    }
    #ifdef I2CIP_DEBUG_SERIAL
      else {
        DEBUG_DELAY();
        I2CIP_DEBUG_SERIAL.println(F("-> EEPROM Self-Add Success"));
        DEBUG_DELAY();
      }
    #endif

    Device** temp = I2CIP::devicetree[this->eeprom->getFQA()];
    // r = (temp != nullptr && *temp == this->eeprom);
    r = (temp != nullptr && *temp != nullptr && EEPROM::getID() != nullptr && EEPROM::getID()[0] != '\0' && strcmp((*temp)->getID(), EEPROM::getID()) == 0); // TODO: THIS MIGHT BE A PROBLEM

    if(!r) {
      #ifdef I2CIP_DEBUG_SERIAL
        DEBUG_DELAY();
        I2CIP_DEBUG_SERIAL.print(F("-> FQA LOOKUP EEPROM* MISMATCH: "));
        I2CIP_DEBUG_SERIAL.print((uintptr_t)temp, HEX);
        I2CIP_DEBUG_SERIAL.print(" != ");
        I2CIP_DEBUG_SERIAL.println((uintptr_t)this->eeprom, HEX);
        DEBUG_DELAY();
      #endif
      return I2CIP_ERR_SOFT;
    }

    this->eeprom_added = true;
  }

  
  // 2. SELF CHECK MUX & EEPROM
  if(!MUX::pingMUX(this->wire, this->mux) || eeprom == nullptr || eeprom->getInput() == nullptr) return I2CIP_ERR_HARD; // REBUILD PLZ

  // 3. READ EEPROM CONTENTS
  const uint16_t len = I2CIP_EEPROM_SIZE;
  i2cip_errorlevel_t errlev = eeprom->getInput()->get(&len);
  // i2cip_errorlevel_t errlev = eeprom->getInput()->failGet();
  // eeprom->readContents(buf, len, I2CIP_EEPROM_SIZE);
  // I2CIP_ERR_BREAK(MUX::resetBus(this->eeprom->getFQA()));

  if(errlev != I2CIP_ERR_NONE || eeprom->getCache() == nullptr || eeprom->getCache()[0] == '\0') {
    if (errlev == I2CIP_ERR_SOFT && recurse) {
      // BAD EEPROM CONTENT - OVERWRITE WITH FAILSAFE
      #ifdef I2CIP_DEBUG_SERIAL
        DEBUG_DELAY();
        I2CIP_DEBUG_SERIAL.println(F("Overwriting EEPROM with Failsafe and Retrying...\n"));
        DEBUG_DELAY();
      #endif
      errlev = ((OutputSetter*)eeprom)->reset();
      I2CIP_ERR_BREAK(errlev);

      return discoverEEPROM(false);
    }
    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.println(F("EEPROM Get Failed!"));
      DEBUG_DELAY();
    #endif
    return errlev;
  }

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("Module "));
    I2CIP_DEBUG_SERIAL.print(getModuleNum(), HEX);
    I2CIP_DEBUG_SERIAL.print(F(" EEPROM Read! Parsing...\n"));
    DEBUG_DELAY();
  #endif

  // Parse EEPROM contents into module devices
  bool r = parseEEPROMContents(eeprom->getCache());
  if(r) return errlev; // All done
  else if (recurse) {
    // BAD EEPROM CONTENT - OVERWRITE WITH FAILSAFE
    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("EEPROM Parse Failed! Overwriting with Failsafe and Retrying...\n"));
      DEBUG_DELAY();
    #endif
    errlev = ((OutputSetter*)eeprom)->reset();
    I2CIP_ERR_BREAK(errlev);

    return discoverEEPROM(false);
  } else {
    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("EEPROM Parse Failed! Aborting...\n"));
      DEBUG_DELAY();
    #endif
    return I2CIP_ERR_SOFT;
  }
}

// bool Module::add(Device& device) { // Subnet match check
//   if(this->isFQAinSubnet(device.getFQA()) && (*this)[device.getFQA()] != nullptr) return true;
//   return false;
// }

DeviceGroup* Module::addEmptyGroup(const char* id) {
  // Look for existing group
  I2CIP::DeviceGroup* ptr = this->devicegroups[id];
  if(ptr != nullptr) return ptr; // Group already exists

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("Creating new DeviceGroup '"));
    I2CIP_DEBUG_SERIAL.print(id);
    I2CIP_DEBUG_SERIAL.print("' @0x");
    I2CIP_DEBUG_SERIAL.print((uintptr_t)id, HEX);
    I2CIP_DEBUG_SERIAL.print("\n");
    DEBUG_DELAY();
  #endif

  // Allocate new DeviceGroup
  DeviceGroup* group = this->deviceGroupFactory(id);
  if(group == nullptr) {
    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("DeviceGroup Factory Failed!\n"));
      DEBUG_DELAY();
    #endif
    return nullptr;
  }

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("DeviceGroup Factory Success! Inserting into HashTable\n"));
    DEBUG_DELAY();
  #endif

  // Insert into HashTable
  HashTableEntry<DeviceGroup>* entry = this->devicegroups.set(group->key, group);
  return entry != nullptr ? entry->value : nullptr;
}

bool Module::add(Device* device, bool overwrite) {
  if(device == nullptr) return false;

  i2cip_fqa_t fqa = device->getFQA();
  i2cip_id_t id = device->getID();

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("-> Module Add Device* @0x"));
    I2CIP_DEBUG_SERIAL.print((uintptr_t)device, HEX);
    I2CIP_DEBUG_SERIAL.print(F("(ID '"));
    I2CIP_DEBUG_SERIAL.print(device->getID());
    I2CIP_DEBUG_SERIAL.print(F("' @0x"));
    I2CIP_DEBUG_SERIAL.print((uintptr_t)(device->getID()), HEX);  
    I2CIP_DEBUG_SERIAL.print(F("; FQA "));
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_I2CBUS(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(':');
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MODULE(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(':');
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(':');
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print("):\n");
    DEBUG_DELAY();
  #endif

  // 1. Search HashTable for DeviceGroup; If not found attempt to create
  DeviceGroup* entry = this->devicegroups[id];
  if(entry == nullptr) {
    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("DeviceGroup Not Found in HashTable, Creating\n"));
      DEBUG_DELAY();
    #endif
    entry = addEmptyGroup(id);
  }
  if(entry == nullptr) {
    // STILL?
    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("Failed to Create DeviceGroup! Check Libraries.\n"));
      DEBUG_DELAY();
    #endif
    return false;
  } else {
    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("DeviceGroup Found in HashTable\n"));
      DEBUG_DELAY();
    #endif
  }

  // 2. Has the device already been added? (PTR || FQA)
  if(entry->contains(device)) {
    device = entry->operator[](device->getFQA());
    if(device == nullptr) return false;
  }

  // 3. Overwrite BST
  BSTNode<i2cip_fqa_t, Device*>* dptr = I2CIP::devicetree.insert(device->getFQA(), device, overwrite);
  bool r = dptr != nullptr && dptr->value != nullptr;
  if (r) { r = entry->add(dptr->value); }
  #ifdef I2CIP_DEBUG_SERIAL
    else {
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("Failed to Save Device*!\n"));
      DEBUG_DELAY();
  }
  #endif

  #ifdef I2CIP_DEBUG_SERIAL
    if(r) {
        DEBUG_DELAY();
        I2CIP_DEBUG_SERIAL.print(F("Device* Saved!\n"));
        DEBUG_DELAY();
    }
  #endif
  return r;
}

// bool Module::add(Device& device, bool overwrite) {
//   i2cip_fqa_t fqa = device.getFQA();

//   #ifdef I2CIP_DEBUG_SERIAL
//     DEBUG_DELAY();
//     I2CIP_DEBUG_SERIAL.print(F("-> Module Add Device @0x"));
//     I2CIP_DEBUG_SERIAL.print((uintptr_t)&device, HEX);
//     I2CIP_DEBUG_SERIAL.print(F("(ID '"));
//     I2CIP_DEBUG_SERIAL.print(device.getID());
//     I2CIP_DEBUG_SERIAL.print(F("' @0x"));
//     I2CIP_DEBUG_SERIAL.print((uintptr_t)&device.getID()[0], HEX);  
//     I2CIP_DEBUG_SERIAL.print(F("; FQA "));
//     I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_I2CBUS(fqa), HEX);
//     I2CIP_DEBUG_SERIAL.print(':');
//     I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MODULE(fqa), HEX);
//     I2CIP_DEBUG_SERIAL.print(':');
//     I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(fqa), HEX);
//     I2CIP_DEBUG_SERIAL.print(':');
//     I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
//     I2CIP_DEBUG_SERIAL.print("):\n");
//     DEBUG_DELAY();
//   #endif

//   // Search HashTable for DeviceGroup
//   const char* id = device.getID();
//   DeviceGroup* entry = this->devicegroups[id];
//   if(entry == nullptr) {
//     #ifdef I2CIP_DEBUG_SERIAL
//       DEBUG_DELAY();
//       I2CIP_DEBUG_SERIAL.print(F("DeviceGroup Not Found in HashTable, Creating\n"));
//       DEBUG_DELAY();
//     #endif
//     entry = addEmptyGroup(id);
//   }
//   if(entry == nullptr) {
//     // STILL?
//     #ifdef I2CIP_DEBUG_SERIAL
//       DEBUG_DELAY();
//       I2CIP_DEBUG_SERIAL.print(F("Failed to Create DeviceGroup! Check Libraries.\n"));
//       DEBUG_DELAY();
//     #endif
//     return false;
//   } else {
//     #ifdef I2CIP_DEBUG_SERIAL
//       DEBUG_DELAY();
//       I2CIP_DEBUG_SERIAL.print(F("DeviceGroup Found in HashTable\n"));
//       DEBUG_DELAY();
//     #endif
//   }

//   // if(d != nullptr || dptr != nullptr) {
//   //   if(overwrite) {
//   //     #ifdef I2CIP_DEBUG_SERIAL
//   //       DEBUG_DELAY();
//   //       I2CIP_DEBUG_SERIAL.print(F("Overwriting Device.\n"));
//   //       DEBUG_DELAY();
//   //     #endif

//   //     // Delete old device
//   //     if(d != nullptr) this->remove(d, &device != d);
//   //     else if(dptr != nullptr) this->remove(*dptr, &device != *dptr);
//   //   } else {
//   //     return (dptr == nullptr ? (I2CIP::devicetree.insert(fqa, d) != nullptr) : (strcmp((*dptr)->getID(), device.getID()) == 0));
//   //   }
//   // }

//   if(entry->contains(device) && overwrite) {
//   #ifdef I2CIP_DEBUG_SERIAL
//       DEBUG_DELAY();
//       I2CIP_DEBUG_SERIAL.print(F("Removing Old Device\n"));
//       DEBUG_DELAY();
//     #endif

//     // Delete old device
//     this->remove(entry->operator[](device.getFQA()), strcmp(entry->key, device.getID()) != 0);
//   }

//   BSTNode<i2cip_fqa_t, Device*>* dptr = I2CIP::devicetree.insert(fqa, &device, true);
//   bool r = dptr != nullptr && dptr->value != nullptr;
//   if (r) { r = entry->add(dptr->value); }
//   #ifdef I2CIP_DEBUG_SERIAL
//     else {
//       DEBUG_DELAY();
//       I2CIP_DEBUG_SERIAL.print(F("Failed to Save Device!\n"));
//       DEBUG_DELAY();
//   }
//   #endif

//   #ifdef I2CIP_DEBUG_SERIAL
//     if(r) {
//         DEBUG_DELAY();
//         I2CIP_DEBUG_SERIAL.print(F("Device Saved!\n"));
//         DEBUG_DELAY();
//     }
//   #endif
//   return r;

//   // } else if(dptr != nullptr) {
//   //   // In bst; not device group
//   //   if(overwrite && strcmp((*dptr)->getID(), device.getID()) != 0) {
//   //     #ifdef I2CIP_DEBUG_SERIAL
//   //       DEBUG_DELAY();
//   //       I2CIP_DEBUG_SERIAL.print(F("Overwriting Device\n"));
//   //       DEBUG_DELAY();
//   //     #endif

//   //     // Overwrite old device
//   //     dptr = &((I2CIP::devicetree.insert(fqa, &device, true))->value);
//   //   }
//   //   return strcmp((*dptr)->getID(), device.getID()) == 0;
//   // } else {
//   //   if(overwrite) {
//   //     #ifdef I2CIP_DEBUG_SERIAL
//   //       DEBUG_DELAY();
//   //       I2CIP_DEBUG_SERIAL.print(F("Removing Old Devices\n"));
//   //       DEBUG_DELAY();
//   //     #endif

//   //     // Delete old device
//   //     this->remove(d, &device != d);
//   //   }
//   //   return strcmp(d->getID(), device.getID()) == 0;
//   // }

//   // // Was in HashTable but not BST
//   // BSTNode<i2cip_fqa_t, Device*>* ptr = I2CIP::devicetree.insert(fqa, &device, true); // Made a change here
//   // // return false; // Invoke deletion - wait why??
//   // // return strcmp(entry->key, device.getID()) == 0;
//   // return entry->value.add(device);

//   // // BSTNode<i2cip_fqa_t, Device*>* ptr = I2CIP::devicetree.insert(fqa, &device);
//   // if(ptr == nullptr) {
//   //   #ifdef I2CIP_DEBUG_SERIAL
//   //     DEBUG_DELAY();
//   //     I2CIP_DEBUG_SERIAL.print(F("-> Failed to Save Device!\n"));
//   //     DEBUG_DELAY();
//   //   #endif
//   //   return false;
//   // }
//   // if(ptr->value != &device || ptr->key != fqa) {
//   //   #ifdef I2CIP_DEBUG_SERIAL
//   //     DEBUG_DELAY();
//   //     I2CIP_DEBUG_SERIAL.print(F("-> BST Node Mismatch!\n"));
//   //     DEBUG_DELAY();
//   //   #endif
//   //   I2CIP::devicetree.remove(fqa);
//   //   return false;
//   // }

//   // #ifdef I2CIP_DEBUG_SERIAL
//   //   DEBUG_DELAY();
//   //   I2CIP_DEBUG_SERIAL.print(F("-> BST Success (FQA "));
//   //   I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_I2CBUS(fqa), HEX);
//   //   I2CIP_DEBUG_SERIAL.print(':');
//   //   I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MODULE(fqa), HEX);
//   //   I2CIP_DEBUG_SERIAL.print(':');
//   //   I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(fqa), HEX);
//   //   I2CIP_DEBUG_SERIAL.print(':');
//   //   I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
//   //   I2CIP_DEBUG_SERIAL.print(F(", Device @0x"));
//   //   I2CIP_DEBUG_SERIAL.print((uintptr_t)&device, HEX);
//   //   I2CIP_DEBUG_SERIAL.print(")\n");
//   //   DEBUG_DELAY();
//   // #endif

//   // // Insert into DeviceGroup (by pointer copy)
//   // if(!entry->value.add(device)) {
//   //   I2CIP::devicetree.remove(fqa);
//   //   return false;
//   // }

//   // #ifdef I2CIP_DEBUG_SERIAL
//   //   DEBUG_DELAY();
//   //   I2CIP_DEBUG_SERIAL.print(F("-> HashTable Success (ID '"));
//   //   I2CIP_DEBUG_SERIAL.print(entry->value.key);
//   //   I2CIP_DEBUG_SERIAL.print(F("' @0x"));
//   //   I2CIP_DEBUG_SERIAL.print((uintptr_t)&entry->value.key[0], HEX);
//   //   I2CIP_DEBUG_SERIAL.print(F(", DeviceGroup["));
//   //   I2CIP_DEBUG_SERIAL.print(entry->value.numdevices);
//   //   I2CIP_DEBUG_SERIAL.print(F("] @0x"));
//   //   I2CIP_DEBUG_SERIAL.print((uintptr_t)&entry->value, HEX);
//   //   I2CIP_DEBUG_SERIAL.print(F(", Factory @0x"));
//   //   I2CIP_DEBUG_SERIAL.print((uintptr_t)entry->value.factory, HEX);
//   //   I2CIP_DEBUG_SERIAL.print(")\n");
//   //   DEBUG_DELAY();
//   // #endif
// }

// OLD ADD
// bool Module::add(Device& device, bool overwrite) {
//   i2cip_fqa_t fqa = device.getFQA();

//   #ifdef I2CIP_DEBUG_SERIAL
//     DEBUG_DELAY();
//     I2CIP_DEBUG_SERIAL.print(F("-> Module Add Device @0x"));
//     I2CIP_DEBUG_SERIAL.print((uintptr_t)&device, HEX);
//     I2CIP_DEBUG_SERIAL.print(F("(ID '"));
//     I2CIP_DEBUG_SERIAL.print(device.getID());
//     I2CIP_DEBUG_SERIAL.print(F("' @0x"));
//     I2CIP_DEBUG_SERIAL.print((uintptr_t)&device.getID()[0], HEX);  
//     I2CIP_DEBUG_SERIAL.print(F("; FQA "));
//     I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_I2CBUS(fqa), HEX);
//     I2CIP_DEBUG_SERIAL.print(':');
//     I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MODULE(fqa), HEX);
//     I2CIP_DEBUG_SERIAL.print(':');
//     I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(fqa), HEX);
//     I2CIP_DEBUG_SERIAL.print(':');
//     I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
//     I2CIP_DEBUG_SERIAL.print("):\n");
//     DEBUG_DELAY();
//   #endif

//   // Search BST for Device
//   Device** dptr = I2CIP::devicetree[fqa];
//   if(dptr != nullptr) {
//     #ifdef I2CIP_DEBUG_SERIAL
//       I2CIP_DEBUG_SERIAL.print(F("Device Already Exists (ID '"));
//       I2CIP_DEBUG_SERIAL.print((*dptr)->getID());
//       I2CIP_DEBUG_SERIAL.print("')\n");
//       DEBUG_DELAY();
//     #endif

//     // Return false to invoke deletion
//     if (!overwrite) return false;
//   }

//   // Search HashTable for DeviceGroup
//   const char* id = device.getID();
//   HashTableEntry<DeviceGroup&>* entry = this->devicegroups[id];
//   if(entry == nullptr) entry = addEmptyGroup(id);
//   if(entry == nullptr) {
//     #ifdef I2CIP_DEBUG_SERIAL
//       DEBUG_DELAY();
//       I2CIP_DEBUG_SERIAL.print(F("Failed to Create DeviceGroup! Check Libraries.\n"));
//       DEBUG_DELAY();
//     #endif
//     return false;
//   }
  
//   Device* d = entry->value[device.getFQA()];

//   if(d != nullptr) {
//     if(dptr != nullptr && overwrite) {
//       #ifdef I2CIP_DEBUG_SERIAL
//         DEBUG_DELAY();
//         I2CIP_DEBUG_SERIAL.print(F("Overwriting Device.\n"));
//         DEBUG_DELAY();
//       #endif

//       // Delete old device
//       this->remove(d, true);
//     } else {
//       // Was in HashTable but not BST
//       I2CIP::devicetree.insert(fqa, d, true);
//       return false; // Invoke deletion
//     }
//   }

//   BSTNode<i2cip_fqa_t, Device*>* ptr = I2CIP::devicetree.insert(fqa, &device);
//   if(ptr == nullptr) {
//     #ifdef I2CIP_DEBUG_SERIAL
//       DEBUG_DELAY();
//       I2CIP_DEBUG_SERIAL.print(F("-> Failed to Save Device!\n"));
//       DEBUG_DELAY();
//     #endif
//     return false;
//   }
//   if(ptr->value != &device || ptr->key != fqa) {
//     #ifdef I2CIP_DEBUG_SERIAL
//       DEBUG_DELAY();
//       I2CIP_DEBUG_SERIAL.print(F("-> BST Node Mismatch!\n"));
//       DEBUG_DELAY();
//     #endif
//     I2CIP::devicetree.remove(fqa);
//     return false;
//   }

//   #ifdef I2CIP_DEBUG_SERIAL
//     DEBUG_DELAY();
//     I2CIP_DEBUG_SERIAL.print(F("-> BST Success (FQA "));
//     I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_I2CBUS(fqa), HEX);
//     I2CIP_DEBUG_SERIAL.print(':');
//     I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MODULE(fqa), HEX);
//     I2CIP_DEBUG_SERIAL.print(':');
//     I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(fqa), HEX);
//     I2CIP_DEBUG_SERIAL.print(':');
//     I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
//     I2CIP_DEBUG_SERIAL.print(F(", Device @0x"));
//     I2CIP_DEBUG_SERIAL.print((uintptr_t)&device, HEX);
//     I2CIP_DEBUG_SERIAL.print(")\n");
//     DEBUG_DELAY();
//   #endif

//   // Insert into DeviceGroup (by pointer copy)
//   if(!entry->value.add(device)) {
//     I2CIP::devicetree.remove(fqa);
//     return false;
//   }

//   #ifdef I2CIP_DEBUG_SERIAL
//     DEBUG_DELAY();
//     I2CIP_DEBUG_SERIAL.print(F("-> HashTable Success (ID '"));
//     I2CIP_DEBUG_SERIAL.print(entry->value.key);
//     I2CIP_DEBUG_SERIAL.print(F("' @0x"));
//     I2CIP_DEBUG_SERIAL.print((uintptr_t)&entry->value.key[0], HEX);
//     I2CIP_DEBUG_SERIAL.print(F(", DeviceGroup["));
//     I2CIP_DEBUG_SERIAL.print(entry->value.numdevices);
//     I2CIP_DEBUG_SERIAL.print(F("] @0x"));
//     I2CIP_DEBUG_SERIAL.print((uintptr_t)&entry->value, HEX);
//     I2CIP_DEBUG_SERIAL.print(F(", Factory @0x"));
//     I2CIP_DEBUG_SERIAL.print((uintptr_t)entry->value.factory, HEX);
//     I2CIP_DEBUG_SERIAL.print(")\n");
//     DEBUG_DELAY();
//   #endif

//   return true;
// }

DeviceGroup* Module::operator[](i2cip_id_t id) {
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("-> Module DeviceGroup Lookup (ID '"));
    I2CIP_DEBUG_SERIAL.print(id);
    I2CIP_DEBUG_SERIAL.print("'):");
    DEBUG_DELAY();
  #endif
  DeviceGroup* entry = this->devicegroups[id];
  if(entry == nullptr) {
    #ifdef I2CIP_DEBUG_SERIAL
      I2CIP_DEBUG_SERIAL.print(F(" Not Found, Creating...\n"));
      DEBUG_DELAY();
    #endif
    
    return addEmptyGroup(id);
  }
  
  #ifdef I2CIP_DEBUG_SERIAL
    I2CIP_DEBUG_SERIAL.print(F(" Found! (Factory @0x"));
    I2CIP_DEBUG_SERIAL.print((uintptr_t)entry->factory, HEX);
    I2CIP_DEBUG_SERIAL.print(")\n");
    DEBUG_DELAY();
  #endif

  return entry;
}

void Module::remove(Device* device, bool del) {
  if(device == nullptr) return;
  i2cip_fqa_t fqa = device->getFQA();
  this->remove(fqa, del);
}

void Module::remove(const i2cip_fqa_t& fqa, bool del) {
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("-> Removing Device... "));
  #endif

  // bool swap = false;
  // for(int i = 0; i < this->numdevices-1; i++) {
  //   if(swap) {
  //     this->devices[i - 1] = this->devices[i];
  //   }
  //   if(this->devices[i]->getFQA() == device->getFQA()) { 
  //     this->devices[i] = nullptr;
  //     this->numdevices--;
  //     swap = true;
  //   }
  // }

  // Lookup in BST
  Device** dptr = I2CIP::devicetree[fqa];
  if(dptr != nullptr && (*dptr)->getFQA() == fqa) {
    I2CIP::devicetree.remove(fqa);
  
    // Lookup in HashTable
    DeviceGroup* entry = this->devicegroups[(*dptr)->getID()];
    if(entry != nullptr) entry->remove(entry->operator[](fqa));

    // Delete device
    if(del) { delete (*dptr); }

    #ifdef I2CIP_DEBUG_SERIAL
      I2CIP_DEBUG_SERIAL.print(F("Removed!\n"));
      DEBUG_DELAY();
    #endif
  } else {
    #ifdef I2CIP_DEBUG_SERIAL
      I2CIP_DEBUG_SERIAL.print(F("Not Found!\n"));
      DEBUG_DELAY();
    #endif
  }
}

bool Module::isFQAinSubnet(const i2cip_fqa_t& fqa) { 
  if(I2CIP_FQA_SEG_MODULE(fqa) == I2CIP_MUX_NUM_FAKE || I2CIP_FQA_SEG_MUXBUS(fqa) == I2CIP_MUX_BUS_FAKE) return true; // Allows any Module to wrap a faked-out/non-MUX device
  bool match = I2CIP_FQA_SUBNET_MATCH(fqa, this->eeprom->getFQA());
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    String m = fqaToString(fqa);
    if(match) {
      I2CIP_DEBUG_SERIAL.print(m + F(" Subnet Match!\n"));
    } else {
      I2CIP_DEBUG_SERIAL.print(m + F(" Subnet Mismatch!\n"));
    }
    DEBUG_DELAY();
  #endif
  return match;
}

// bool Module::contains(Device* device) {
//   #ifdef I2CIP_DEBUG_SERIAL
//     DEBUG_DELAY();
//     I2CIP_DEBUG_SERIAL.print(F("Module Contains Device Check!\n");
//     DEBUG_DELAY();
//   #endif

//   if(device == nullptr || !this->isFQAinSubnet(device->getFQA())) return false;
//   for(int i = 0; i < this->numdevices; i++) {
//     if(this->devices[i]->getFQA() == device->getFQA()) return true;
//   }
//   return false;
// }

void Module::unready(void) {
  if(this->eeprom != nullptr) this->eeprom->unready();
  for(uint8_t g = 0; g < HASHTABLE_SLOTS; g++) {
    HashTableEntry<DeviceGroup>* entry = this->devicegroups.hashtable[g];
    do {
      if(entry != nullptr && entry->value->numdevices > 0) {
        #ifdef I2CIP_DEBUG_SERIAL
          DEBUG_DELAY();
          I2CIP_DEBUG_SERIAL.print(F("-> Unreadying DeviceGroup '"));
          I2CIP_DEBUG_SERIAL.print(entry->key);
          I2CIP_DEBUG_SERIAL.print("' @0x");
          I2CIP_DEBUG_SERIAL.print((uintptr_t)entry->value, HEX);
          I2CIP_DEBUG_SERIAL.print(" with ");
          I2CIP_DEBUG_SERIAL.print(entry->value->numdevices);
          I2CIP_DEBUG_SERIAL.println(F(" Devices"));
          DEBUG_DELAY();
        #endif
        entry->value->unready();
      }
      entry = entry->next;
    } while(entry != nullptr);
  }
}

i2cip_errorlevel_t Module::operator()(void) {
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("-> Module Self-Check!\n"));
    DEBUG_DELAY();
  #endif

  if(this->eeprom == nullptr) { return I2CIP_ERR_HARD; }
  if(!this->eeprom_added) {
    return this->discoverEEPROM();
    // Serial.print("EEPROM 0x");
    // Serial.println(I2CIP_FQA_SEG_DEVADR(this->eeprom->getFQA()), HEX);
  } else {
    if(!MUX::pingMUX(eeprom->getFQA())) {
      #ifdef I2CIP_DEBUG_SERIAL
        DEBUG_DELAY();
        I2CIP_DEBUG_SERIAL.print(F("MUX Ping Failed! Unreadying All Devices...\n"));
        DEBUG_DELAY();
      #endif
      this->unready();
      return I2CIP_ERR_HARD;
    }
  }

  // 3. Ping EEPROM until ready
  return this->eeprom->pingTimeout(true, true);
}


  // i2cip_fqa_t fqa = d.getFQA();
  // #ifdef I2CIP_DEBUG_SERIAL
  //   DEBUG_DELAY();
  //   I2CIP_DEBUG_SERIAL.print(F("-> Module Device @0x"));
  //   I2CIP_DEBUG_SERIAL.print((uintptr_t)&d, HEX);
  //   I2CIP_DEBUG_SERIAL.print(F(" Check (FQA "));
  //   I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_I2CBUS(fqa), HEX);
  //   I2CIP_DEBUG_SERIAL.print(':');
  //   I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MODULE(fqa), HEX);
  //   I2CIP_DEBUG_SERIAL.print(':');
  //   I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(fqa), HEX);
  //   I2CIP_DEBUG_SERIAL.print(':');
  //   I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
  //   I2CIP_DEBUG_SERIAL.print(") ");
  //   DEBUG_DELAY();
  // #endif

  // // 2. Device check
  // if(!this->isFQAinSubnet(fqa)) return I2CIP_ERR_SOFT;
  // // Device* device = this->operator[](fqa);
  // // if(device == nullptr) { 
  // //   #ifdef I2CIP_DEBUG_SERIAL
  // //     DEBUG_DELAY();
  // //     I2CIP_DEBUG_SERIAL.print(F("-> Device Not Found!\n"));
  // //     DEBUG_DELAY();
  // //   #endif
  // //   return I2CIP_ERR_SOFT;
  // // }
  // i2cip_errorlevel_t errlev;
  // if(update) {
  //   errlev = MUX::setBus(fqa);
  //   I2CIP_ERR_BREAK(errlev);

  //   // 3. Update device (optional)
  //   #ifdef I2CIP_DEBUG_SERIAL
  //     DEBUG_DELAY();
  //     I2CIP_DEBUG_SERIAL.print(F("-> Updating Device\n"));
  //     DEBUG_DELAY();
  //   #endif
  //   // Do Output, then Input
  //   if(d.getOutput()) {
  //     // #ifdef I2CIP_DEBUG_SERIAL
  //     //   DEBUG_DELAY();
  //     //   I2CIP_DEBUG_SERIAL.print(F("Output Set:\n"));
  //     //   DEBUG_DELAY();
  //     // #endif
  //     errlev = fail ? d.getOutput()->failSet() : d.getOutput()->set();
  //     I2CIP_ERR_BREAK(errlev);
  //   }
  //   if(d.getInput()) {
  //     // #ifdef I2CIP_DEBUG_SERIAL
  //     //   DEBUG_DELAY();
  //     //   I2CIP_DEBUG_SERIAL.print(F("Input Get:\n"));
  //     //   DEBUG_DELAY();
  //     // #endif
  //     errlev = fail ? d.getInput()->failGet() : d.getInput()->get();
  //     I2CIP_ERR_BREAK(errlev);
  //   }
  // } else {
  //   // Just Ping
  //   errlev = d.pingTimeout(true);

  //   #ifdef I2CIP_DEBUG_SERIAL
  //     DEBUG_DELAY();
  //     if (errlev == I2CIP_ERR_NONE) { 
  //       I2CIP_DEBUG_SERIAL.print(F("Alive!"));
  //     } else {
  //       I2CIP_DEBUG_SERIAL.print(F("Dead!"));
  //     }
  //     DEBUG_DELAY();
  //   #endif
  // }

  // return errlev;
// }

// Device* Module::operator[](const i2cip_fqa_t& fqa) {
//   if(!this->isFQAinSubnet(fqa)) return nullptr;
//   for(int i = 0; i < this->numdevices; i++) {
//     if(this->devices[i]->getFQA() == fqa) return this->devices[i];
//   }
//   return nullptr;
// }

i2cip_errorlevel_t I2CIP::Module::operator()(Device* d, bool update, i2cip_args_io_t args, Print& out) {
  if(d == nullptr) { return I2CIP_ERR_SOFT; }

  i2cip_fqa_t fqa = d->getFQA();
  // if(!this->isFQAinSubnet(fqa)) return I2CIP_ERR_SOFT;

  String m = fqaToString(fqa);
  m += " '"; m += d->getID(); m += "' ";

  unsigned long now = millis();
  i2cip_errorlevel_t errlev = I2CIP_ERR_NONE;
  if(update) {
    errlev = MUX::setBus(fqa);
    I2CIP_ERR_BREAK(errlev); // Critical

    // Do Output, then Input
    if(d->getOutput() != nullptr && (args.s != nullptr || args.b != nullptr)) {
      // #ifdef I2CIP_DEBUG_SERIAL
      //   DEBUG_DELAY();
      //   I2CIP_DEBUG_SERIAL.print(F("Output Set:\n"));
      //   DEBUG_DELAY();
      // #endif
      errlev = d->set(args.s, args.b);
    }
    if(errlev == I2CIP_ERR_NONE && (d->getInput() != nullptr)) {
      // #ifdef I2CIP_DEBUG_SERIAL
      //   DEBUG_DELAY();
      //   I2CIP_DEBUG_SERIAL.print(F("Input Get:\n"));
      //   DEBUG_DELAY();
      // #endif
      errlev = d->get(args.a);
      // errlev = d->getInput()->get(args.a); // .a defaults to nullptr which triggers failGet anyway
    }
  } else {
    // Just Ping
    errlev = d->pingTimeout(true);
  }

  unsigned long delta = millis() - now;

  // if(out.peek() == 37) return errlev; // Probabaly NullStream; Refer

  switch(errlev){
    case I2CIP_ERR_NONE: m += "PASS"; break;
    case I2CIP_ERR_SOFT: m += "EINVAL"; break;
    case I2CIP_ERR_HARD: m += "EIO"; break;
    default: out.print("ERR???"); break;
  }
  m += (' ');
  m += String(delta / 1000.0, 3);
  m += ('s');

  if(update && errlev == I2CIP_ERR_NONE) {
    if(d->getInput() != nullptr) {
      m += (F(" INPGET ")); 
      #ifdef I2CIP_INPUTS_USE_TOSTRING
        m += (d->getInput()->printCache());
      #endif
    }
    if(d->getOutput() != nullptr) {
      m += (F(" OUTSET "));
      #ifdef I2CIP_OUTPUTS_USE_TOSTRING
        m += ((args.s == nullptr) ? "NULL" : (d->getOutput()->valueToString()));
      #endif
    }
    // if(d->getInput() == nullptr && d->getOutput() == nullptr) { m += (F(" NOP")); }
  }
  out.println(m);

  return errlev;
}