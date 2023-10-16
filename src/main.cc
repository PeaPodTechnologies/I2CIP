#include <Arduino.h>

// Uncomment to enable debug
#define DEBUG_SERIAL Serial
#include <debug.h>

#include <I2CIP.h>

using namespace I2CIP;

class BasicModule : public Module {
  private:
    // TODO: Consider moving to Module?
    DeviceGroup dg_eeprom = DeviceGroup((const char*&)i2cip_eeprom_id, I2CIP_ITYPE_IO, i2cip_eeprom_factory);
  public:
    BasicModule(const uint8_t& wire, const uint8_t& module) : Module(wire, module) { }
    
    bool parseEEPROMContents(const uint8_t* buffer, size_t buflen) override {
      #ifdef I2CIP_DEBUG_SERIAL
        DEBUG_DELAY();
        I2CIP_DEBUG_SERIAL.print("No Parsing Yet!\n");
        DEBUG_DELAY();
      #endif
      dg_eeprom.add(*((EEPROM*)this));
      return true;
    }

    Device* operator[](const i2cip_fqa_t& fqa) const override {
      // EEPROM MATCH
      if(I2CIP_FQA_MODULE_MATCH(fqa, this->getWireNum(), this->getModuleNum()) && I2CIP_FQA_BUSADR_MATCH(fqa, I2CIP_FQA_SEG_MUXBUS(eeprom.getFQA()), I2CIP_FQA_SEG_DEVADR(eeprom.getFQA()))) {
        return (Device*)&eeprom;
      }

      // DEVICEGROUP CONTAINS
      return dg_eeprom[fqa];
    }
    
    DeviceGroup* operator[](const i2cip_id_t& id) override {
      // EEPROM MATCH
      if(strcmp(id, dg_eeprom.key) == 0) {
        return &dg_eeprom;
      }

      return nullptr;
    }

    bool add(Device& device) override {
      if((*this)[device.getFQA()] != nullptr) return true;
      return dg_eeprom.add(device);
    }

    void remove(Device* device) override {
      if(device == nullptr) return;
      if((*this)[device->getFQA()] == nullptr) return;
      dg_eeprom.remove(device);
    }
};

BasicModule* m;  // to be initialized in setup()

void setup(void) {
  DEBUG_SERIAL.begin(115200);
  DEBUG_SERIAL.println("Initializing BasicModule...");

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
  DEBUG_SERIAL.print("\n");

  // Build module
  if(!Module::build(*m)) while(true);
}

void loop(void) {
  (*m)();
  (*m)(((const EEPROM&)(*m)).getFQA());
  delay(1000);
}


// char* str = new char[buflen + 1];
// memcpy(str, buffer, buflen);
// str[buflen] = '\0';

// String json = String(str);
// delete[] str;

// int i = json.indexOf("{");
// if(i == -1) return false;

// i = json.indexOf("sht31");
// if(i == -1) return false;

// i = json.indexOf("[", i);
// if(i == -1) return false;

// int j = json.indexOf("]", i);
// if(j == -1) return false;

// for (int k = (i + 1); k < (j - 1); k++) {
//   switch (json.charAt(k)) {
//     case ',':
//     case ' ':
//       continue;
//     case '\n':
//     case '\r':
//     case '\t':
//     case '\0':
//     case ']':
//     case '"':
//       return false;
//     default:
//       // parse JSON integer
//       uint8_t addr = json.substring(k, json.indexOf(",", k)).toInt();

//       // Acceptable address range for SHT31 is 0x44-0x45

//       // What does this bit logic do?:
//       // 0x44 = 0b01000100 from "68"
//       // 0x45 = 0b01000101 from "69"
//       // ~0x44 = 0b10111011 = 0xBB
//       // ~0x45 = 0b10111010 = 0xBA
//       // If addr == 0x44, then addr & 0xBB == 0x00 == false, but `addr & 0xBA == 0x00 == false`
//       // If addr == 0x45, then addr & 0xBA == 0x00 == false, but addr & 0xBB == 
//       // If addr == 0x46, then addr & 0xBB == 0x02 == true
//       if(addr & 0xBB || addr & 0xBA) {
//         // Add device to module
//         this->add(Device(0, 0, addr));
//         return true;
//       } else {
//         // Invalid address
//         return false;
//       }
//   }
// }