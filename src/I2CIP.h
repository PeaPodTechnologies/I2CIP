#ifndef I2CIP_H_
#define I2CIP_H_

#include <fqa.h>
#include <mux.h>
#include <device.h>
#include <eeprom.h>

// #define I2CIP_FQA_SUBNET_MATCH(fqa, wire, module) (bool)(I2CIP_FQA_SEG_I2CBUS(fqa) == wire && I2CIP_FQA_SEG_MODULE(fqa) == module)
#define I2CIP_FQA_SUBNET_MATCH(fqa, _fqa) (bool)(I2CIP_FQA_SEG_I2CBUS(fqa) == I2CIP_FQA_SEG_I2CBUS(_fqa) && I2CIP_FQA_SEG_MODULE(fqa) == I2CIP_FQA_SEG_MODULE(_fqa))

namespace I2CIP {

  // enables subnet communication and state awareness.
  class Module {
    private:
      EEPROM* eeprom = nullptr;

      uint8_t numdevices = 0;
      Device* devices[I2CIP_DEVICES_PER_GROUP] = { nullptr };

      bool isFQAinSubnet(const i2cip_fqa_t& fqa);

    protected:
      bool contains(Device* device);
      void remove(Device* device);

      Module(const uint8_t& wire, const uint8_t& module, const uint8_t& eeprom_addr = I2CIP_EEPROM_ADDR);

    public:
      // Module(const i2cip_fqa_t& fqa);
      static bool build(Module& m);
      ~Module();

      // TODO: When to add devices?
      void add(Device& device);

      i2cip_errorlevel_t operator()(void); // Module Self-check
      i2cip_errorlevel_t operator()(const i2cip_fqa_t& fqa); // Device Check

      Device* operator[](const i2cip_fqa_t& fqa);

      uint8_t getWireNum(void);
      uint8_t getModuleNum(void);

      virtual bool parseEEPROMContents(const uint8_t* buffer, size_t buflen) = 0;
  };
};

#endif