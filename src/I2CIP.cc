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
    I2CIP_DEBUG_SERIAL.print("Module ");
    I2CIP_DEBUG_SERIAL.print(mux, HEX);
    I2CIP_DEBUG_SERIAL.print(" Constructed\n");
    DEBUG_DELAY();
  #endif
}

bool Module::build(Module& m) {
  // Failsafe
  // if(m.eeprom == nullptr) return false;

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print("Module ");
    I2CIP_DEBUG_SERIAL.print(m.getModuleNum(), HEX);
    I2CIP_DEBUG_SERIAL.print(" Building...\n");
    DEBUG_DELAY();
  #endif

  // Read EEPROM
  uint8_t buf[I2CIP_EEPROM_SIZE] = { '\0' };
  size_t len = 0;
  i2cip_errorlevel_t errlev = m.eeprom.readContents(buf, len, I2CIP_EEPROM_SIZE);

  if(errlev != I2CIP_ERR_NONE || len == 0) return false;

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print("Module ");
    I2CIP_DEBUG_SERIAL.print(m.getModuleNum(), HEX);
    I2CIP_DEBUG_SERIAL.print(" EEPROM Read! Parsing...\n");
    DEBUG_DELAY();
  #endif

  // Parse EEPROM contents into module devices
  return m.parseEEPROMContents(buf, len);
}

bool Module::add(Device& device) { // Subnet match check
  if(this->isFQAinSubnet(device.getFQA()) && (*this)[device.getFQA()] != nullptr) return true;
  return false;
}

// bool Module::add(Device& device) {
//   if(!this->isFQAinSubnet(device.getFQA()) || this->contains(&device)) return;
  
//   #ifdef I2CIP_DEBUG_SERIAL
//     DEBUG_DELAY();
//     I2CIP_DEBUG_SERIAL.print("Adding Device!\n");
//     DEBUG_DELAY();
//   #endif

//   unsigned int n = 0;
//   while(this->devices[n] != nullptr) { n++; if(n > I2CIP_DEVICES_PER_GROUP) return;}

//   // Append new devices
//   this->devices[n] = &device;
//   this->numdevices = (n + 1);
// }

// void Module::remove(Device* device) {
//   #ifdef I2CIP_DEBUG_SERIAL
//     DEBUG_DELAY();
//     I2CIP_DEBUG_SERIAL.print("Removing Device!\n");
//     DEBUG_DELAY();
//   #endif

//   bool swap = false;
//   for(int i = 0; i < this->numdevices-1; i++) {
//     if(swap) {
//       this->devices[i - 1] = this->devices[i];
//     }
//     if(this->devices[i]->getFQA() == device->getFQA()) { 
//       this->devices[i] = nullptr;
//       this->numdevices--;
//       swap = true;
//     }
//   }
// }

bool Module::isFQAinSubnet(const i2cip_fqa_t& fqa) { 
  bool match = I2CIP_FQA_SUBNET_MATCH(fqa, this->eeprom.getFQA());
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    if(match) {
      I2CIP_DEBUG_SERIAL.print("Subnet Match!\n");
    } else {
      I2CIP_DEBUG_SERIAL.print("Subnet Mismatch!\n");
    }
    DEBUG_DELAY();
  #endif
  return match;
}

// bool Module::contains(Device* device) {
//   #ifdef I2CIP_DEBUG_SERIAL
//     DEBUG_DELAY();
//     I2CIP_DEBUG_SERIAL.print("Module Contains Device Check!\n");
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
    I2CIP_DEBUG_SERIAL.print("Module Self-Check...\n");
    DEBUG_DELAY();
  #endif

  // 1. Check MUX - If we have lost the switch the entire subnet is down!
  i2cip_errorlevel_t errlev = MUX::pingMUX(this->eeprom.getFQA()) ? I2CIP_ERR_NONE : I2CIP_ERR_HARD;
  I2CIP_ERR_BREAK(errlev);

  // 3. Ping EEPROM
  return this->eeprom.pingTimeout(true, false);
}

i2cip_errorlevel_t Module::operator()(const i2cip_fqa_t& fqa) {
  // 1. Self-check
  // i2cip_errorlevel_t errlev = this->operator()();
  // I2CIP_ERR_BREAK(errlev);

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print("Module Device Check (FQA ");
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_I2CBUS(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(":");
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MODULE(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(":");
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(":");
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
      I2CIP_DEBUG_SERIAL.print("Device Not Found!\n");
      DEBUG_DELAY();
    #endif
    return I2CIP_ERR_HARD;
  }
  i2cip_errorlevel_t errlev = device->pingTimeout();
  // I2CIP_ERR_BREAK(errlev);

  // TODO: Anything?

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    if (errlev == I2CIP_ERR_NONE) { 
      I2CIP_DEBUG_SERIAL.print("Device Alive!\n");
    } else {
      I2CIP_DEBUG_SERIAL.print("Device Dead!\n");
    }
    DEBUG_DELAY();
  #endif

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