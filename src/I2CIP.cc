#include <I2CIP.h>

#include <eeprom.h>
#include <debug.h>

using namespace I2CIP;

// Module::Module(const i2cip_fqa_t& eeprom_fqa) { 
//   EEPROM* _eeprom = new EEPROM(eeprom_fqa); 
//   if(_eeprom->pingTimeout() == I2CIP_ERR_NONE) {

//   }
// }

Module::Module(const uint8_t& wire, const uint8_t& mux, const uint8_t& eeprom_addr) : wire(wire), mux(mux), eeprom(EEPROM(wire, mux, eeprom_addr)) {
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("Module "));
    I2CIP_DEBUG_SERIAL.print(mux, HEX);
    I2CIP_DEBUG_SERIAL.print(F(" Constructed\n"));
    DEBUG_DELAY();
  #endif

  add(eeprom);
}

DeviceGroup* Module::deviceGroupFactory(const char* id) {
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("DeviceGroup Factory: Matching '"));
    I2CIP_DEBUG_SERIAL.print(id);
    I2CIP_DEBUG_SERIAL.print("'...\n");
    DEBUG_DELAY();
  #endif

  if(strcmp(id, EEPROM::_id) == 0) {
    return new DeviceGroup(EEPROM::_id, I2CIP_ITYPE_IO, i2cip_eeprom_factory);
  }

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("ID Not Found!\n"));
    DEBUG_DELAY();
  #endif

  return nullptr;
}

bool Module::build(Module& m) {
  // Failsafe
  // if(m.eeprom == nullptr) return false;

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("Module "));
    I2CIP_DEBUG_SERIAL.print(m.getModuleNum(), HEX);
    I2CIP_DEBUG_SERIAL.print(F(" Building...\n"));
    DEBUG_DELAY();
  #endif

  // Read EEPROM
  uint8_t buf[I2CIP_EEPROM_SIZE] = { '\0' };
  size_t len = 0;
  i2cip_errorlevel_t errlev = m.eeprom.readContents(buf, len, I2CIP_EEPROM_SIZE);

  if(errlev != I2CIP_ERR_NONE || len == 0) return false;

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("Module "));
    I2CIP_DEBUG_SERIAL.print(m.getModuleNum(), HEX);
    I2CIP_DEBUG_SERIAL.print(F(" EEPROM Read! Parsing...\n"));
    DEBUG_DELAY();
  #endif

  // Parse EEPROM contents into module devices
  return m.parseEEPROMContents(buf, len);
}

// bool Module::add(Device& device) { // Subnet match check
//   if(this->isFQAinSubnet(device.getFQA()) && (*this)[device.getFQA()] != nullptr) return true;
//   return false;
// }

HashTableEntry<DeviceGroup&>* Module::addEmptyGroup(const char* id) {
  if(this->devices_idgroups.get(id) != nullptr) return nullptr; // Group already exists

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("Creating new DeviceGroup '"));
    I2CIP_DEBUG_SERIAL.print(id);
    I2CIP_DEBUG_SERIAL.print("' @0x");
    I2CIP_DEBUG_SERIAL.print((uint16_t)&id[0], HEX);
    I2CIP_DEBUG_SERIAL.print("...\n");
    DEBUG_DELAY();
  #endif

  // Allocate new DeviceGroup
  DeviceGroup* group = deviceGroupFactory(id);
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

bool Module::add(Device& device, bool overwrite) {
  i2cip_fqa_t fqa = device.getFQA();

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("Adding Device (ID '"));
    I2CIP_DEBUG_SERIAL.print(device.getID());
    I2CIP_DEBUG_SERIAL.print(F("' @0x"));
    I2CIP_DEBUG_SERIAL.print((uint16_t)&device.getID()[0], HEX);  
    I2CIP_DEBUG_SERIAL.print(F("; FQA "));
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_I2CBUS(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(':');
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MODULE(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(':');
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(':');
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(")...\n");
    DEBUG_DELAY();
  #endif

  // Search BST for Device
  Device** dptr = this->devices_fqabst[fqa];
  if(dptr != nullptr) {
    #ifdef I2CIP_DEBUG_SERIAL
      I2CIP_DEBUG_SERIAL.print(F("Device Already Exists (ID '"));
      I2CIP_DEBUG_SERIAL.print((*dptr)->getID());
      I2CIP_DEBUG_SERIAL.print("')\n");
      DEBUG_DELAY();
    #endif

    if (!overwrite) return true;
  }

  // Search HashTable for DeviceGroup
  const char* id = device.getID();
  HashTableEntry<DeviceGroup&>* entry = this->devices_idgroups[id];
  if(entry == nullptr) entry = addEmptyGroup(id);
  if(entry == nullptr) return false;

  if(dptr != nullptr && overwrite) {
    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("Overwriting Device...\n"));
      DEBUG_DELAY();
    #endif

    // Remove old device
    entry->value.remove(*dptr);
    this->devices_fqabst.remove((*dptr)->getFQA());
    delete *dptr;
  }

  // Insert into BST (by pointer copy)
  BSTNode<i2cip_fqa_t, Device*>* ptr = this->devices_fqabst.insert(fqa, &device, true);
  if(ptr == nullptr) return false;
  if(ptr->value != &device || ptr->key != fqa) {
    this->devices_fqabst.remove(fqa);
    return false;
  }

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("BST Success; "));
    DEBUG_DELAY();
  #endif

  // Insert into DeviceGroup (by pointer copy)
  if(!entry->value.add(device)) {
    this->devices_fqabst.remove(fqa);
    return false;
  }

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("DeviceGroup HashTable Success;\n"));
    DEBUG_DELAY();
  #endif

  return true;
}

Device* Module::operator[](const i2cip_fqa_t& fqa) const {
  return *(this->devices_fqabst.operator[](fqa));
}

DeviceGroup& Module::operator[](i2cip_id_t id) {
  HashTableEntry<DeviceGroup&>* entry = this->devices_idgroups[id];
  if(entry == nullptr) return addEmptyGroup(id)->value;
  return entry->value;
}

void Module::remove(Device* device, bool del) {
  if(device == nullptr) return;

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("Removing Device... "));
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
  if(dptr == nullptr) return; // Device not found
  if(*dptr != device) return; // FQA points to different device
  if((*dptr)->getFQA() != fqa) return;  // Device doesn't match

  // Remove from BST
  this->devices_fqabst.remove(fqa);

  // Lookup in HashTable
  HashTableEntry<DeviceGroup&>* entry = this->devices_idgroups[device->getID()];
  if(entry == nullptr) return;

  // Remove from DeviceGroup
  entry->value.remove(device);

  // Delete device
  if(del) delete device;

  #ifdef I2CIP_DEBUG_SERIAL
    I2CIP_DEBUG_SERIAL.print(F("Device Removed!\n"));
    DEBUG_DELAY();
  #endif
}

bool Module::isFQAinSubnet(const i2cip_fqa_t& fqa) { 
  bool match = I2CIP_FQA_SUBNET_MATCH(fqa, this->eeprom.getFQA());
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
    I2CIP_DEBUG_SERIAL.print(F("Module Self-Check...\n"));
    DEBUG_DELAY();
  #endif

  // 1. Check MUX - If we have lost the switch the entire subnet is down!
  i2cip_errorlevel_t errlev = MUX::pingMUX(this->eeprom.getFQA()) ? I2CIP_ERR_NONE : I2CIP_ERR_HARD;
  I2CIP_ERR_BREAK(errlev);

  // 3. Ping EEPROM
  return this->eeprom.pingTimeout(true, false);
}

i2cip_errorlevel_t Module::operator()(const i2cip_fqa_t& fqa, bool update, bool fail) {
  // 1. Self-check
  // i2cip_errorlevel_t errlev = this->operator()();
  // I2CIP_ERR_BREAK(errlev);

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("Module Device Check (FQA "));
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_I2CBUS(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(':');
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MODULE(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(':');
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(':');
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(")...\n");
    DEBUG_DELAY();
  #endif

  // 2. Device check
  if(!this->isFQAinSubnet(fqa)) return I2CIP_ERR_SOFT;
  Device* device = this->operator[](fqa);
  if(device == nullptr) { 
    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("Device Not Found!\n"));
      DEBUG_DELAY();
    #endif
    return I2CIP_ERR_HARD;
  }
  i2cip_errorlevel_t errlev = device->pingTimeout();
  // I2CIP_ERR_BREAK(errlev);

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    if (errlev == I2CIP_ERR_NONE) { 
      I2CIP_DEBUG_SERIAL.print(F("Device Alive!\n"));
    } else {
      I2CIP_DEBUG_SERIAL.print(F("Device Dead!\n"));
    }
    DEBUG_DELAY();
  #endif

  if(update) {
    // 3. Update device (optional)
    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("Updating Device...\n"));
      DEBUG_DELAY();
    #endif
    // Do Input/Output
    if(device->getInput()) {
      errlev = fail ? device->getInput()->get(&InputGetter::failptr_get) : device->getInput()->get();
      I2CIP_ERR_BREAK(errlev);
    }
    if(device->getOutput()) {
      errlev = fail ? device->getOutput()->set(&OutputSetter::failptr_set, &OutputSetter::failptr_set) : device->getOutput()->set();
      I2CIP_ERR_BREAK(errlev);
    }
  }

  return errlev;
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