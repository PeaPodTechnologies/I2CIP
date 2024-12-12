#include <I2CIP.h>

#include <eeprom.h>
#include <debug.h>

using namespace I2CIP;

// Module::Module(const i2cip_fqa_t& eeprom_fqa) { 
//   EEPROM* _eeprom = new EEPROM(eeprom_fqa); 
//   if(_eeprom->pingTimeout() == I2CIP_ERR_NONE) {

//   }
// }

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
  eeprom->removeOutput();

  // EEPROM::loadEEPROMID(); // THIS IS COCONUT.PNG DO NOT ALTER

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

  // Delete all DeviceGroups, deleting all Devices (incl. EEPROM)
  for(uint8_t i = 0; i < HASHTABLE_SLOTS; i++) {
    if(this->devices_idgroups.hashtable[i] != nullptr) {
      // delete (&(this->devices_idgroups.hashtable[i]->value));
      this->devices_idgroups.hashtable[i]->value.destruct();
      delete this->devices_idgroups.hashtable[i];
    }
  }

  // BST, Hashtable are static and delete their own entries
}

DeviceGroup* Module::deviceGroupFactory(const i2cip_id_t& id) {
  // #ifdef I2CIP_DEBUG_SERIAL
  //   DEBUG_DELAY();
  //   I2CIP_DEBUG_SERIAL.print(F("-> Builtin EEPROM DeviceGroup Factory: Matching '"));
  //   I2CIP_DEBUG_SERIAL.print(id);
  //   I2CIP_DEBUG_SERIAL.print("':");
  //   DEBUG_DELAY();
  // #endif

  if(id == EEPROM::getStaticIDBuffer() || strcmp(id, EEPROM::getStaticIDBuffer()) == 0) {
    // #ifdef I2CIP_DEBUG_SERIAL
    //   DEBUG_DELAY();
    //   I2CIP_DEBUG_SERIAL.print(F(" Match! Creating...\n"));
    //   DEBUG_DELAY();
    // #endif

    return new DeviceGroup(EEPROM::getStaticIDBuffer(), EEPROM::eepromFactory);
  }

  // #ifdef I2CIP_DEBUG_SERIAL
  //   DEBUG_DELAY();
  //   I2CIP_DEBUG_SERIAL.print(F(" EEPROM ID Mismatch!\n"));
  //   DEBUG_DELAY();
  // #endif

  return nullptr;
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

  // 1. EEPROM MODULE OOP

  if(!this->eeprom_added) { 
    EEPROM::loadEEPROMID(); // THIS IS COCONUT.PNG DO NOT ALTER
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

    Device** temp = this->devices_fqabst[this->eeprom->getFQA()];
    // r = (temp != nullptr && *temp == this->eeprom);
    r = (temp != nullptr && *temp != nullptr && EEPROM::getStaticIDBuffer() != nullptr && EEPROM::getStaticIDBuffer()[0] != '\0' && strcmp((*temp)->getID(), EEPROM::getStaticIDBuffer()) == 0);

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

  i2cip_errorlevel_t errlev = this->operator()();
  I2CIP_ERR_BREAK(errlev);

  // 3. READ EEPROM CONTENTS
  if(eeprom == nullptr || eeprom->getInput() == nullptr) return I2CIP_ERR_HARD; // REBUILD PLZ
  const uint16_t len = I2CIP_EEPROM_SIZE;
  errlev = eeprom->getInput()->get(&len);
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

HashTableEntry<DeviceGroup&>* Module::addEmptyGroup(const char* id) {
  HashTableEntry<I2CIP::DeviceGroup &> * ptr = this->devices_idgroups.get(id);
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
  return this->devices_idgroups.set(id, *group);
}

bool Module::parseEEPROMContents(const char* contents) {
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("No Parsing Yet! EEPROM Contents '"));
    I2CIP_DEBUG_SERIAL.print(contents);
    I2CIP_DEBUG_SERIAL.print("' @0x");
    I2CIP_DEBUG_SERIAL.print((uintptr_t)contents, HEX);
    I2CIP_DEBUG_SERIAL.print("\n");
    DEBUG_DELAY();
  #endif
  return true;
}

bool Module::add(Device* device, bool overwrite) {
  if(device == nullptr) return false;

  i2cip_fqa_t fqa = device->getFQA();

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

  // Search HashTable for DeviceGroup
  const char* id = device->getID();
  HashTableEntry<DeviceGroup&>* entry = this->devices_idgroups[id];
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

  if(entry->value.contains(device)) {
    // Identical ID and FQA

    device = entry->value[device->getFQA()];
    if(device == nullptr) return false;
  }

  BSTNode<i2cip_fqa_t, Device*>* dptr = this->devices_fqabst.insert(device->getFQA(), device, true);
  bool r = dptr != nullptr && dptr->value != nullptr;
  if (r) { r = entry->value.add(dptr->value); }
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

bool Module::add(Device& device, bool overwrite) {
  i2cip_fqa_t fqa = device.getFQA();

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("-> Module Add Device @0x"));
    I2CIP_DEBUG_SERIAL.print((uintptr_t)&device, HEX);
    I2CIP_DEBUG_SERIAL.print(F("(ID '"));
    I2CIP_DEBUG_SERIAL.print(device.getID());
    I2CIP_DEBUG_SERIAL.print(F("' @0x"));
    I2CIP_DEBUG_SERIAL.print((uintptr_t)&device.getID()[0], HEX);  
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

  // Search HashTable for DeviceGroup
  const char* id = device.getID();
  HashTableEntry<DeviceGroup&>* entry = this->devices_idgroups[id];
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

  // if(d != nullptr || dptr != nullptr) {
  //   if(overwrite) {
  //     #ifdef I2CIP_DEBUG_SERIAL
  //       DEBUG_DELAY();
  //       I2CIP_DEBUG_SERIAL.print(F("Overwriting Device.\n"));
  //       DEBUG_DELAY();
  //     #endif

  //     // Delete old device
  //     if(d != nullptr) this->remove(d, &device != d);
  //     else if(dptr != nullptr) this->remove(*dptr, &device != *dptr);
  //   } else {
  //     return (dptr == nullptr ? (this->devices_fqabst.insert(fqa, d) != nullptr) : (strcmp((*dptr)->getID(), device.getID()) == 0));
  //   }
  // }

  if(entry->value.contains(device.getFQA()) && overwrite) {
  #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("Removing Old Device\n"));
      DEBUG_DELAY();
    #endif

    // Delete old device
    this->remove(entry->value[device.getFQA()], strcmp(entry->value.key, device.getID()) != 0);
  }

  BSTNode<i2cip_fqa_t, Device*>* dptr = this->devices_fqabst.insert(fqa, &device, true);
  bool r = dptr != nullptr && dptr->value != nullptr;
  if (r) { r = entry->value.add(dptr->value); }
  #ifdef I2CIP_DEBUG_SERIAL
    else {
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("Failed to Save Device!\n"));
      DEBUG_DELAY();
  }
  #endif

  #ifdef I2CIP_DEBUG_SERIAL
    if(r) {
        DEBUG_DELAY();
        I2CIP_DEBUG_SERIAL.print(F("Device Saved!\n"));
        DEBUG_DELAY();
    }
  #endif
  return r;

  // } else if(dptr != nullptr) {
  //   // In bst; not device group
  //   if(overwrite && strcmp((*dptr)->getID(), device.getID()) != 0) {
  //     #ifdef I2CIP_DEBUG_SERIAL
  //       DEBUG_DELAY();
  //       I2CIP_DEBUG_SERIAL.print(F("Overwriting Device\n"));
  //       DEBUG_DELAY();
  //     #endif

  //     // Overwrite old device
  //     dptr = &((this->devices_fqabst.insert(fqa, &device, true))->value);
  //   }
  //   return strcmp((*dptr)->getID(), device.getID()) == 0;
  // } else {
  //   if(overwrite) {
  //     #ifdef I2CIP_DEBUG_SERIAL
  //       DEBUG_DELAY();
  //       I2CIP_DEBUG_SERIAL.print(F("Removing Old Devices\n"));
  //       DEBUG_DELAY();
  //     #endif

  //     // Delete old device
  //     this->remove(d, &device != d);
  //   }
  //   return strcmp(d->getID(), device.getID()) == 0;
  // }

  // // Was in HashTable but not BST
  // BSTNode<i2cip_fqa_t, Device*>* ptr = this->devices_fqabst.insert(fqa, &device, true); // Made a change here
  // // return false; // Invoke deletion - wait why??
  // // return strcmp(entry->key, device.getID()) == 0;
  // return entry->value.add(device);

  // // BSTNode<i2cip_fqa_t, Device*>* ptr = this->devices_fqabst.insert(fqa, &device);
  // if(ptr == nullptr) {
  //   #ifdef I2CIP_DEBUG_SERIAL
  //     DEBUG_DELAY();
  //     I2CIP_DEBUG_SERIAL.print(F("-> Failed to Save Device!\n"));
  //     DEBUG_DELAY();
  //   #endif
  //   return false;
  // }
  // if(ptr->value != &device || ptr->key != fqa) {
  //   #ifdef I2CIP_DEBUG_SERIAL
  //     DEBUG_DELAY();
  //     I2CIP_DEBUG_SERIAL.print(F("-> BST Node Mismatch!\n"));
  //     DEBUG_DELAY();
  //   #endif
  //   this->devices_fqabst.remove(fqa);
  //   return false;
  // }

  // #ifdef I2CIP_DEBUG_SERIAL
  //   DEBUG_DELAY();
  //   I2CIP_DEBUG_SERIAL.print(F("-> BST Success (FQA "));
  //   I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_I2CBUS(fqa), HEX);
  //   I2CIP_DEBUG_SERIAL.print(':');
  //   I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MODULE(fqa), HEX);
  //   I2CIP_DEBUG_SERIAL.print(':');
  //   I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(fqa), HEX);
  //   I2CIP_DEBUG_SERIAL.print(':');
  //   I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
  //   I2CIP_DEBUG_SERIAL.print(F(", Device @0x"));
  //   I2CIP_DEBUG_SERIAL.print((uintptr_t)&device, HEX);
  //   I2CIP_DEBUG_SERIAL.print(")\n");
  //   DEBUG_DELAY();
  // #endif

  // // Insert into DeviceGroup (by pointer copy)
  // if(!entry->value.add(device)) {
  //   this->devices_fqabst.remove(fqa);
  //   return false;
  // }

  // #ifdef I2CIP_DEBUG_SERIAL
  //   DEBUG_DELAY();
  //   I2CIP_DEBUG_SERIAL.print(F("-> HashTable Success (ID '"));
  //   I2CIP_DEBUG_SERIAL.print(entry->value.key);
  //   I2CIP_DEBUG_SERIAL.print(F("' @0x"));
  //   I2CIP_DEBUG_SERIAL.print((uintptr_t)&entry->value.key[0], HEX);
  //   I2CIP_DEBUG_SERIAL.print(F(", DeviceGroup["));
  //   I2CIP_DEBUG_SERIAL.print(entry->value.numdevices);
  //   I2CIP_DEBUG_SERIAL.print(F("] @0x"));
  //   I2CIP_DEBUG_SERIAL.print((uintptr_t)&entry->value, HEX);
  //   I2CIP_DEBUG_SERIAL.print(F(", Factory @0x"));
  //   I2CIP_DEBUG_SERIAL.print((uintptr_t)entry->value.factory, HEX);
  //   I2CIP_DEBUG_SERIAL.print(")\n");
  //   DEBUG_DELAY();
  // #endif
}

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
//   Device** dptr = this->devices_fqabst[fqa];
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
//   HashTableEntry<DeviceGroup&>* entry = this->devices_idgroups[id];
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
//       this->devices_fqabst.insert(fqa, d, true);
//       return false; // Invoke deletion
//     }
//   }

//   BSTNode<i2cip_fqa_t, Device*>* ptr = this->devices_fqabst.insert(fqa, &device);
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
//     this->devices_fqabst.remove(fqa);
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
//     this->devices_fqabst.remove(fqa);
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

Device* Module::operator[](const i2cip_fqa_t& fqa) const {
  // #ifdef I2CIP_DEBUG_SERIAL
  //   DEBUG_DELAY();
  //   I2CIP_DEBUG_SERIAL.print(F("-> Module Device Lookup (FQA "));
  //   I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_I2CBUS(fqa), HEX);
  //   I2CIP_DEBUG_SERIAL.print(':');
  //   I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MODULE(fqa), HEX);
  //   I2CIP_DEBUG_SERIAL.print(':');
  //   I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(fqa), HEX);
  //   I2CIP_DEBUG_SERIAL.print(':');
  //   I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
  //   I2CIP_DEBUG_SERIAL.print("):");
  //   DEBUG_DELAY();
  //   delay(10);
  // #endif
  
  Device** d = this->devices_fqabst.operator[](fqa);

  // #ifdef I2CIP_DEBUG_SERIAL
  //   if(d == nullptr) {
  //     I2CIP_DEBUG_SERIAL.print(F(" Not Found!\n"));
  //   } else {
  //     I2CIP_DEBUG_SERIAL.print(F(" Found!\n"));
  //   }
  //   DEBUG_DELAY();
  // #endif

  // return *d;

  return d == nullptr ? nullptr : *d;
}

DeviceGroup* Module::operator[](i2cip_id_t id) {
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("-> Module DeviceGroup Lookup (ID '"));
    I2CIP_DEBUG_SERIAL.print(id);
    I2CIP_DEBUG_SERIAL.print("'):");
    DEBUG_DELAY();
  #endif
  HashTableEntry<DeviceGroup&>* entry = this->devices_idgroups[id];
  if(entry == nullptr) {
    #ifdef I2CIP_DEBUG_SERIAL
      I2CIP_DEBUG_SERIAL.print(F(" Not Found, Creating...\n"));
      DEBUG_DELAY();
    #endif
    
    entry = addEmptyGroup(id);
    return entry == nullptr ? nullptr : &entry->value;
  }
  
  #ifdef I2CIP_DEBUG_SERIAL
    I2CIP_DEBUG_SERIAL.print(F(" Found! (Factory @0x"));
    I2CIP_DEBUG_SERIAL.print((uintptr_t)entry->value.factory, HEX);
    I2CIP_DEBUG_SERIAL.print(")\n");
    DEBUG_DELAY();
  #endif

  return &entry->value;
}

void Module::remove(Device* device, bool del) {
  if(device == nullptr) return;

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

  i2cip_fqa_t fqa = device->getFQA();

  // Lookup in BST
  Device** dptr = this->devices_fqabst[fqa];
  if(dptr != nullptr && (*dptr == device || (*dptr)->getFQA() == fqa)) this->devices_fqabst.remove(fqa);  // Device doesn't match
  
  // Lookup in HashTable
  HashTableEntry<DeviceGroup&>* entry = this->devices_idgroups[device->getID()];
  if(entry != nullptr) entry->value.remove(entry->value[fqa]);

  // Delete device
  if(del) delete device;

  #ifdef I2CIP_DEBUG_SERIAL
    I2CIP_DEBUG_SERIAL.print(F("Removed!\n"));
    DEBUG_DELAY();
  #endif
}

bool Module::isFQAinSubnet(const i2cip_fqa_t& fqa) { 
  bool match = I2CIP_FQA_SUBNET_MATCH(fqa, this->eeprom->getFQA());
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    if(match) {
      I2CIP_DEBUG_SERIAL.print(F("Subnet Match!\n"));
    } else {
      I2CIP_DEBUG_SERIAL.print(F("Subnet Mismatch!\n"));
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

i2cip_errorlevel_t Module::operator()(void) {
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("-> Module Self-Check!\n"));
    DEBUG_DELAY();
  #endif

  if(this->eeprom == nullptr) { return I2CIP_ERR_HARD; }
  if(!this->eeprom_added) {
    return this->discoverEEPROM() ? I2CIP_ERR_NONE : I2CIP_ERR_HARD;
    // Serial.print("EEPROM 0x");
    // Serial.println(I2CIP_FQA_SEG_DEVADR(this->eeprom->getFQA()), HEX);
  } else {
    if(!MUX::pingMUX(eeprom->getFQA())) {
      #ifdef I2CIP_DEBUG_SERIAL
        DEBUG_DELAY();
        I2CIP_DEBUG_SERIAL.print(F("MUX Ping Failed! Aborting...\n"));
        DEBUG_DELAY();
      #endif
      return I2CIP_ERR_HARD;
    }
  }

  // 3. Ping EEPROM until ready
  return this->eeprom->pingTimeout(true, true, I2CIP_EEPROM_TIMEOUT);
}

i2cip_errorlevel_t Module::operator()(Device& d, bool update, bool fail) { return this->operator()(&d, update, fail); }
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

i2cip_errorlevel_t Module::operator()(Device* d, bool update, bool fail) { // eturn d == nullptr ? I2CIP_ERR_SOFT : this->operator()(*d, update, fail); }
  if(d == nullptr) return I2CIP_ERR_SOFT;
  i2cip_fqa_t fqa = d->getFQA();

  // 2. Device check
  if(!this->isFQAinSubnet(fqa)) return I2CIP_ERR_SOFT;
  // Device* device = this->operator[](fqa);
  // if(device == nullptr) { 
  //   #ifdef I2CIP_DEBUG_SERIAL
  //     DEBUG_DELAY();
  //     I2CIP_DEBUG_SERIAL.print(F("-> Device Not Found!\n"));
  //     DEBUG_DELAY();
  //   #endif
  //   return I2CIP_ERR_SOFT;
  // }
  i2cip_errorlevel_t errlev;
  // errlev = d->pingTimeout(true, false);
  // #ifdef I2CIP_DEBUG_SERIAL
  //   DEBUG_DELAY();
  //   if (errlev == I2CIP_ERR_NONE) { 
  //     I2CIP_DEBUG_SERIAL.print(F("Alive!"));
  //   } else {
  //     I2CIP_DEBUG_SERIAL.print(F("Dead!"));
  //   }
  //   DEBUG_DELAY();
  // #endif
  // I2CIP_ERR_BREAK(errlev);

  if(update) {
    errlev = MUX::setBus(fqa);
    I2CIP_ERR_BREAK(errlev);

    // 3. Update device (optional)
    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("-> Updating Device\n"));
      DEBUG_DELAY();
    #endif
    // Do Output, then Input
    if(d->getOutput()) {
      // #ifdef I2CIP_DEBUG_SERIAL
      //   DEBUG_DELAY();
      //   I2CIP_DEBUG_SERIAL.print(F("Output Set:\n"));
      //   DEBUG_DELAY();
      // #endif
      errlev = fail ? d->getOutput()->failSet() : d->getOutput()->set();
      I2CIP_ERR_BREAK(errlev);
    }
    if(d->getInput()) {
      // #ifdef I2CIP_DEBUG_SERIAL
      //   DEBUG_DELAY();
      //   I2CIP_DEBUG_SERIAL.print(F("Input Get:\n"));
      //   DEBUG_DELAY();
      // #endif
      errlev = fail ? d->getInput()->failGet() : d->getInput()->get();
      I2CIP_ERR_BREAK(errlev);
    }
  } 
  else {
    // Just Ping
    errlev = d->pingTimeout(true);

    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      if (errlev == I2CIP_ERR_NONE) { 
        I2CIP_DEBUG_SERIAL.print(F("Alive!"));
      } else {
        I2CIP_DEBUG_SERIAL.print(F("Dead!"));
      }
      DEBUG_DELAY();
    #endif
  }

  return errlev;
}

i2cip_errorlevel_t Module::operator()(const i2cip_fqa_t& fqa, bool update, bool fail) {
  // 1. Self-check
  // i2cip_errorlevel_t errlev = this->operator()();
  // I2CIP_ERR_BREAK(errlev);

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("-> Module Device Check (FQA "));
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_I2CBUS(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(':');
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MODULE(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(':');
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(':');
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
    I2CIP_DEBUG_SERIAL.println(")");
    DEBUG_DELAY();
  #endif

  // 2. Device check
  Device* device = this->operator[](fqa);
  if(device == nullptr) {
    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.println(F("-> Device Not Found!\n"));
      DEBUG_DELAY();
    #endif
    return I2CIP_ERR_SOFT;
  }

  return this->operator()(device, update, fail);
}

// Device* Module::operator[](const i2cip_fqa_t& fqa) {
//   if(!this->isFQAinSubnet(fqa)) return nullptr;
//   for(int i = 0; i < this->numdevices; i++) {
//     if(this->devices[i]->getFQA() == fqa) return this->devices[i];
//   }
//   return nullptr;
// }

uint8_t Module::getWireNum(void) const { return this->wire; }

uint8_t Module::getModuleNum(void) const { return this->mux; }