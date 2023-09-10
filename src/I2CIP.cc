#include <I2CIP.h>

#include <eeprom.h>

using namespace I2CIP;

// Module::Module(const i2cip_fqa_t& eeprom_fqa) { 
//   EEPROM* _eeprom = new EEPROM(eeprom_fqa); 
//   if(_eeprom->pingTimeout() == I2CIP_ERR_NONE) {

//   }
// }

Module::Module(const uint8_t& wire, const uint8_t& module, const uint8_t& eeprom_addr) {
  this->eeprom = new EEPROM(wire, module, eeprom_addr);
}

bool Module::build(Module& m) {
  if(m.eeprom == nullptr) return false;

  uint8_t buf[I2CIP_EEPROM_SIZE] = { '\0' };
  size_t len = 0;
  i2cip_errorlevel_t errlev = m.eeprom->readContents(buf, len, I2CIP_EEPROM_SIZE);

  if(errlev != I2CIP_ERR_NONE || len == 0) return false;
  return m.parseEEPROMContents(buf, len);
}

Module::~Module() { delete eeprom; }

void Module::add(Device& device) {
  if(!this->isFQAinSubnet(device.getFQA()) || this->contains(&device)) return;
  
  unsigned int n = 0;
  while(this->devices[n] != nullptr) { n++; if(n > I2CIP_DEVICES_PER_GROUP) return;}

  // Append new devices
  this->devices[n] = &device;
  this->numdevices = (n + 1);
}

void Module::remove(Device* device) {
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

bool Module::isFQAinSubnet(const i2cip_fqa_t& fqa) { 
  return I2CIP_FQA_SUBNET_MATCH(fqa, this->eeprom->getFQA());
}

bool Module::contains(Device* device) {
  if(device == nullptr || !this->isFQAinSubnet(device->getFQA())) return false;
  for(int i = 0; i < this->numdevices; i++) {
    if(this->devices[i]->getFQA() == device->getFQA()) return true;
  }
  return false;
}

i2cip_errorlevel_t Module::operator()(void) {
  // 1. Check MUX - If we have lost the switch the entire subnet is down!
  i2cip_errorlevel_t errlev = MUX::pingMUX(this->eeprom->getFQA()) ? I2CIP_ERR_NONE : I2CIP_ERR_HARD;
  I2CIP_ERR_BREAK(errlev);

  // 2. Check EEPROM - Alive?
  errlev = this->eeprom->pingTimeout();
  I2CIP_ERR_BREAK(errlev);

  // TODO: 3. Read eeprom - buffer change?
  return errlev;
}

i2cip_errorlevel_t Module::operator()(const i2cip_fqa_t& fqa) {
  // 1. Self-check
  // i2cip_errorlevel_t errlev = this->operator()();
  // I2CIP_ERR_BREAK(errlev);

  // 2. Device check
  if(!this->isFQAinSubnet(fqa)) return I2CIP_ERR_SOFT;
  Device* device = this->operator[](fqa);
  if(device == nullptr) return I2CIP_ERR_HARD;
  i2cip_errorlevel_t errlev = device->pingTimeout();
  I2CIP_ERR_BREAK(errlev);
}

Device* Module::operator[](const i2cip_fqa_t& fqa) {
  if(!this->isFQAinSubnet(fqa)) return nullptr;
  for(int i = 0; i < this->numdevices; i++) {
    if(this->devices[i]->getFQA() == fqa) return this->devices[i];
  }
  return nullptr;
}

uint8_t Module::getWireNum(void) { return I2CIP_FQA_SEG_I2CBUS(this->eeprom->getFQA()); }

uint8_t Module::getModuleNum(void) { return I2CIP_FQA_SEG_MODULE(this->eeprom->getFQA()); }