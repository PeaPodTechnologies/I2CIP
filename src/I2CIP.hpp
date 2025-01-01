#ifndef I2CIP_H_
#define I2CIP_H_

#include <Arduino.h>
#include <Wire.h>

#include "fqa.h"
#include "mux.h"

#include "device.h"
#include "interface.h"
#include "eeprom.h"

#include "bst.h"
#include "hashtable.h"
#include "module.h"

#include "debug.h"

namespace I2CIP {
  class JsonModule : public Module {
    public:
      JsonModule(const uint8_t& wire, const uint8_t& module, const uint8_t& eeprom_addr = I2CIP_EEPROM_ADDR) : Module(wire, module, eeprom_addr) { }
      JsonModule(const i2cip_fqa_t& eeprom_fqa) : Module(eeprom_fqa) { }

    protected:
      bool parseEEPROMContents(const char* buffer) override;

      // NOTE: Still virtual; need to implement deviceGroupFactory
  };
};


#endif