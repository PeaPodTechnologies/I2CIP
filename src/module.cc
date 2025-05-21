#include "module.h"

#include "debug.h"

// #ifndef I2CIP_MODULE_T_FIX
// #define I2CIP_MODULE_T_FIX
// template <> i2cip_errorlevel_t I2CIP::Module::operator()<short unsigned int>(short unsigned int* ptr, bool update, i2cip_args_io_t args, Stream& out) { return I2CIP_ERR_HARD; } // This is the new COCONUT.PNG
// template <> void I2CIP::Module::printCache<short unsigned int>(short unsigned int* that, Stream& out) { return; }
// template <> void I2CIP::Module::toString<short unsigned int>(short unsigned int* that, Stream& out) { return; }
// #endif

using namespace I2CIP;

_NullStream NullStream;

DeviceGroup::DeviceGroup(const i2cip_id_t& key, factory_device_t factory, handler_device_t handler) : key(key), factory(factory), handler(handler) {
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
    I2CIP_DEBUG_SERIAL.print(F("), errlev (*)(Device*, fqa, args) @0x"));
    I2CIP_DEBUG_SERIAL.print((uintptr_t)handler, HEX);
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
    I2CIP_DEBUG_SERIAL.print(")\n");
    DEBUG_DELAY();
  #endif
  // Temporarily Disabled - I GUESS!
  // if(strcmp(device.getID(), this->key) != 0) {
  //   #ifdef I2CIP_DEBUG_SERIAL
  //     DEBUG_DELAY();
  //     I2CIP_DEBUG_SERIAL.print(F(": Failed; '"));
  //     I2CIP_DEBUG_SERIAL.print(this->key);
  //     I2CIP_DEBUG_SERIAL.print(F("' != '"));
  //     I2CIP_DEBUG_SERIAL.print(device.getID());
  //     I2CIP_DEBUG_SERIAL.print("'\n");
  //     DEBUG_DELAY();
  //   #endif
  //   return false;
  // } // else {
  //   // Todo: What was this for?
  // }

  // if(this->contains(&device)) return true; // Already added
  if(this->contains(device->getFQA())) return true; // Already added
  
  unsigned int n = 0;
  while(this->devices[n] != nullptr) { n++; if(n > I2CIP_DEVICES_PER_GROUP) return false;}

  // Append new devices
  this->devices[n] = device;
  this->numdevices = (n + 1);
  return true;
}


bool DeviceGroup::add(Device& device) {
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("DeviceGroup (ID '"));
    I2CIP_DEBUG_SERIAL.print(this->key);
    I2CIP_DEBUG_SERIAL.print(F("' @0x"));
    I2CIP_DEBUG_SERIAL.print((uintptr_t)this->key, HEX);
    I2CIP_DEBUG_SERIAL.print(F(") Add Device @0x"));
    I2CIP_DEBUG_SERIAL.print((uintptr_t)(&device), HEX);
    I2CIP_DEBUG_SERIAL.print(F("(ID '"));
    I2CIP_DEBUG_SERIAL.print(device.getID());
    I2CIP_DEBUG_SERIAL.print(F("' @0x"));
    I2CIP_DEBUG_SERIAL.print((uintptr_t)(device.getID()), HEX);
    I2CIP_DEBUG_SERIAL.print(")\n");
    DEBUG_DELAY();
  #endif
  if(strcmp(device.getID(), this->key) != 0) {
    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F(": Failed; '"));
      I2CIP_DEBUG_SERIAL.print((uintptr_t)this->key, HEX);
      I2CIP_DEBUG_SERIAL.print(F("' != '"));
      I2CIP_DEBUG_SERIAL.print((uintptr_t)device.getID(), HEX);
      I2CIP_DEBUG_SERIAL.print("'\n");
      DEBUG_DELAY();
    #endif
    return false;
  } // else {
    // Todo: What was this for?
  // }

  if(this->contains(&device)) return true; // Already added
  if(this->contains(device.getFQA())) return true; // Already added
  
  unsigned int n = 0;
  while(this->devices[n] != nullptr) { n++; if(n > I2CIP_DEVICES_PER_GROUP) return false;}

  // Append new devices
  this->devices[n] = &device;
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
  return this->contains(device->getFQA());
}

bool DeviceGroup::contains(const i2cip_fqa_t& fqa) const {
  for(int i = 0; i < this->numdevices; i++) {
    if(this->devices[i]->getFQA() == fqa) return true;
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
  if(this->contains(fqa)) {
    return this->operator[](fqa);
  }

  Device* device = this->factory(fqa);

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

void DeviceGroup::unready(const uint8_t& wirenum, const uint8_t& muxnum) {
  for(uint8_t i = 0; i < I2CIP_DEVICES_PER_GROUP; i++) {
    if(this->devices[i] != nullptr) {
      i2cip_fqa_t fqa = this->devices[i]->getFQA();
      if(I2CIP_FQA_SEG_I2CBUS(fqa) == wirenum && I2CIP_FQA_SEG_MODULE(fqa) == muxnum) {
        this->devices[i]->unready();
      }
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

// Module::Module(const i2cip_fqa_t& eeprom_fqa) { 
//   EEPROM* _eeprom = new EEPROM(eeprom_fqa); 
//   if(_eeprom->pingTimeout() == I2CIP_ERR_NONE) {

//   }
// }

i2cip_args_io_t I2CIP::_i2cip_args_io_default = { nullptr, nullptr, nullptr };

typedef struct i2cip_args_io_s i2cip_args_io_t;

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

  delete(this->devicegroups);
}

DeviceGroup* Module::deviceGroupFactory(const i2cip_id_t& id) {
  // #ifdef I2CIP_DEBUG_SERIAL
  //   DEBUG_DELAY();
  //   I2CIP_DEBUG_SERIAL.print(F("-> Builtin EEPROM DeviceGroup Factory: Matching '"));
  //   I2CIP_DEBUG_SERIAL.print(id);
  //   I2CIP_DEBUG_SERIAL.print("':");
  //   DEBUG_DELAY();
  // #endif

  // #ifdef I2CIP_DEBUG_SERIAL
  //   DEBUG_DELAY();
  //   I2CIP_DEBUG_SERIAL.print(F(" EEPROM ID Mismatch!\n"));
  //   DEBUG_DELAY();
  // #endif

  return DeviceGroup::create<EEPROM>(id);
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

DeviceGroup* Module::addEmptyGroup(const char* id) {
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
  return this->devicegroups.set(id, group);
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

  if(entry->value.contains(device)) {
    // Identical ID and FQA

    device = entry->value[device->getFQA()];
    if(device == nullptr) return false;
  }

  BSTNode<i2cip_fqa_t, Device*>* dptr = I2CIP::devicetree.insert(device->getFQA(), device, true);
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
  //     return (dptr == nullptr ? (I2CIP::devicetree.insert(fqa, d) != nullptr) : (strcmp((*dptr)->getID(), device.getID()) == 0));
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

  BSTNode<i2cip_fqa_t, Device*>* dptr = I2CIP::devicetree.insert(fqa, &device, true);
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
  //     dptr = &((I2CIP::devicetree.insert(fqa, &device, true))->value);
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
  // BSTNode<i2cip_fqa_t, Device*>* ptr = I2CIP::devicetree.insert(fqa, &device, true); // Made a change here
  // // return false; // Invoke deletion - wait why??
  // // return strcmp(entry->key, device.getID()) == 0;
  // return entry->value.add(device);

  // // BSTNode<i2cip_fqa_t, Device*>* ptr = I2CIP::devicetree.insert(fqa, &device);
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
  //   I2CIP::devicetree.remove(fqa);
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
  //   I2CIP::devicetree.remove(fqa);
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
  
  Device** d = I2CIP::devicetree.operator[](fqa);

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
    I2CIP_DEBUG_SERIAL.print((uintptr_t)entry->value.factory, HEX);
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
  if(I2CIP_FQA_SEG_MODULE(fqa) == I2CIP_MUX_NUM_FAKE || I2CIP_FQA_SEG_MODULE(fqa) != this->getModuleNum()) return true; // Allows any Module to wrap a faked-out/non-MUX device
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
    HashTableEntry<DeviceGroup&>* entry = this->devicegroups.hashtable[g];
    do {
      if(entry != nullptr && entry->value.numdevices > 0) {
        entry->value.unready(this->wire, this->mux);
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

uint8_t Module::getWireNum(void) const { return this->wire; }

uint8_t Module::getModuleNum(void) const { return this->mux; }