#ifndef I2CIP_H_
#define I2CIP_H_

#include <fqa.h>
#include <mux.h>
#include <device.h>
#include <eeprom.h>
#include <debug.h>

// #define I2CIP_FQA_SUBNET_MATCH(fqa, wire, module) (bool)(I2CIP_FQA_SEG_I2CBUS(fqa) == wire && I2CIP_FQA_SEG_MODULE(fqa) == module)
#define I2CIP_FQA_SUBNET_MATCH(fqa, _fqa) (bool)(I2CIP_FQA_SEG_I2CBUS(fqa) == I2CIP_FQA_SEG_I2CBUS(_fqa) && I2CIP_FQA_SEG_MODULE(fqa) == I2CIP_FQA_SEG_MODULE(_fqa))
#define I2CIP_FQA_MODULE_MATCH(fqa, wire, module) (bool)(I2CIP_FQA_SEG_I2CBUS(fqa) == (wire) && I2CIP_FQA_SEG_MODULE(fqa) == (module))
#define I2CIP_FQA_BUSADR_MATCH(fqa, bus, addr) (bool)(I2CIP_FQA_SEG_MUXBUS(fqa) == (bus) && I2CIP_FQA_SEG_DEVADR(fqa) == (addr))

namespace I2CIP {

  // Enables fundamentals subnet communication and state awareness.
  // State data structure is implemented by the child class. Virtual functions are provided to access the state data structure.
  // The Module itself only provides three functions:
  // 1. Module Self-check
  // 2. Device Check

  class Module {
    private:
      const uint8_t wire; // I2C Wire Number (Index of `wires[]`)
      const uint8_t mux;  // MUX/Module Number (0x00 - 0x07, address range 0x70 - 0x77)

      bool isFQAinSubnet(const i2cip_fqa_t& fqa);

    protected:
      Module(const uint8_t& wire, const uint8_t& module, const uint8_t& eeprom_addr = I2CIP_EEPROM_ADDR);

      EEPROM eeprom; // EEPROM device
    public:
      // Module(const i2cip_fqa_t& fqa);
      
      virtual ~Module() { }

      
      i2cip_errorlevel_t operator()(const i2cip_fqa_t& fqa); // Device Check

      uint8_t getWireNum(void) const;
      uint8_t getModuleNum(void) const;

      /**
       * 1. Module Self-check
       * 1a. Check MUX - If we have lost the switch the entire subnet is down!
       * 1b. Rebuild EEPROM if necessary
       * 1c. Ping EEPROM
       * 1d. TODO: Ping Devices
      */
      i2cip_errorlevel_t operator()(void); // Module Self-check

      /**
       * 2. Device Discovery
       * 2a. Read EEPROM
       * 2b. Parse EEPROM
       *  Intended Implementation:
       *  2b i.   Read EEPROM (by Bus)
       *  2b ii.  Ping Devices (by ID)
       *  2b iii. Create (new) Devices and add to DeviceGroups (Made Available Internal API via `operator[]` functions)
      */
      static bool build(Module& m);
      virtual bool parseEEPROMContents(const uint8_t* buffer, size_t buflen) = 0;

      virtual bool add(Device& device);

      virtual Device* operator[](const i2cip_fqa_t& fqa) const = 0;
      virtual DeviceGroup* operator[](const i2cip_id_t& id) = 0;

      virtual void remove(Device* device) = 0;

      inline operator const EEPROM&() const { return this->eeprom; }
  };
};

#endif