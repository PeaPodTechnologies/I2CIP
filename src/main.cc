#ifndef UNIT_TEST
#include <Arduino.h>

// Uncomment to enable debug
#define DEBUG_SERIAL Serial
#include <debug.h>

#include <I2CIP.h>

using namespace I2CIP;

class BasicModule : public Module {
  private:
    // TODO: Consider moving to Module?
    // DeviceGroup dg_eeprom = DeviceGroup((const char*&)i2cip_eeprom_id, I2CIP_ITYPE_IO, i2cip_eeprom_factory);
  public:
    BasicModule(const uint8_t& wire, const uint8_t& module) : Module(wire, module) { }
    
    bool parseEEPROMContents(const uint8_t* buffer, size_t buflen) override {
      #ifdef I2CIP_DEBUG_SERIAL
        DEBUG_DELAY();
        I2CIP_DEBUG_SERIAL.print(F("No Parsing Yet!\n"));
        DEBUG_DELAY();
      #endif
      return true;
    }

    // Device* operator[](const i2cip_fqa_t& fqa) const override {
    //   // EEPROM MATCH
    //   if(I2CIP_FQA_MODULE_MATCH(fqa, this->getWireNum(), this->getModuleNum()) && I2CIP_FQA_BUSADR_MATCH(fqa, I2CIP_FQA_SEG_MUXBUS(eeprom.getFQA()), I2CIP_FQA_SEG_DEVADR(eeprom.getFQA()))) {
    //     return (Device*)&eeprom;
    //   }

    //   // DEVICEGROUP CONTAINS
    //   return dg_eeprom[fqa];
    // }
    
    // DeviceGroup* operator[](const i2cip_id_t& id) override {
    //   // EEPROM MATCH
    //   if(strcmp(id, dg_eeprom.key) == 0) {
    //     return &dg_eeprom;
    //   }

    //   return nullptr;
    // }

    // bool add(Device& device) override {
    //   if((*this)[device.getFQA()] != nullptr) return true;
    //   return dg_eeprom.add(device);
    // }

    // void remove(Device* device) override {
    //   if(device == nullptr) return;
    //   if((*this)[device->getFQA()] == nullptr) return;
    //   dg_eeprom.remove(device);
    // }
};

BasicModule* m;  // to be initialized in setup()
char idbuffer[10];

void setup(void) {
  DEBUG_SERIAL.begin(115200);

  DEBUG_SERIAL.println("\nInitializing BasicModule...");

  // Initialize module
  m = new BasicModule(0, 0);

  // if((EEPROM*)(m) == nullptr) while(true);

  i2cip_fqa_t fqa = ((const EEPROM&)(*m)).getFQA();
  DEBUG_SERIAL.print("BasicModule Initialized! EEPROM FQA ");
  DEBUG_SERIAL.print(I2CIP_FQA_SEG_I2CBUS(fqa), HEX);
  DEBUG_SERIAL.print(":");
  DEBUG_SERIAL.print(I2CIP_FQA_SEG_MODULE(fqa), HEX);
  DEBUG_SERIAL.print(":");
  DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(fqa), HEX);
  DEBUG_SERIAL.print(":");
  DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
  DEBUG_SERIAL.print(" ID '");
  DEBUG_SERIAL.print(((const EEPROM&)(*m)).getID());
  DEBUG_SERIAL.print(F("' @0x"));
  DEBUG_SERIAL.print((uint16_t)&((const EEPROM&)(*m)).getID()[0], HEX);  
  DEBUG_SERIAL.print('\n');

  // Build module
  if(!Module::build(*m)) while(true) { // Blink
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}
i2cip_errorlevel_t errlev;

void loop(void) {
  errlev = (*m)();
  DEBUG_SERIAL.print(F("ERRORLEVEL: "));
  DEBUG_SERIAL.print(errlev);
  DEBUG_SERIAL.print('\n');
  delay(1000);

  errlev = (*m)(((const EEPROM&)(*m)).getFQA());
  DEBUG_SERIAL.print(F("ERRORLEVEL: "));
  DEBUG_SERIAL.print(errlev);
  DEBUG_SERIAL.print('\n');
  delay(1000);
}

#endif