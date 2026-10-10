#ifndef I2CIP_H_
#define I2CIP_H_

#include <Arduino.h>
#include <Wire.h>

#include <ArduinoJson.h>

#include "fqa.h"
#include "mux.h"

#include "device.h"
#include "interface.h"
#include "eeprom.h"

#include "bst.h"
#include "hashtable.h"
#include "module.h"

#define I2CIP_REVISION 0

namespace I2CIP {

  /**
   * An abstract base class for JSON SPRT-enabled modules.
   */
  class JsonModule : public Module {
    public:
      JsonModule(const uint8_t& wire, const uint8_t& module, const uint8_t& eeprom_addr = I2CIP_EEPROM_ADDR) : Module(wire, module, eeprom_addr) { }
      JsonModule(const i2cip_fqa_t& eeprom_fqa) : Module(eeprom_fqa) { }

    protected:
      bool parseEEPROMContents(const char* buffer) override;

      // NOTE: Still virtual; need to implement deviceGroupFactory, handleCommand, handleConfig
  };

  /**
   * A barebones module implementation that does not correspond to any real hardware module. Instead provides a placeholder to standardize operations for devices not on a module.
   * No-argument call operator and EEPROM discovery are disabled, and return a software errorlevel. Other call operators behave normally.
   * NOTE: deviceGroupFactory is still EEPROM-only, so extend this class if you need device groups for non-EEPROM devices.
   */
  class NotAModule : public Module {
    public:
      NotAModule(const uint8_t& wire) : Module(wire, I2CIP_MUX_NUM_FAKE, I2CIP_EEPROM_ADDR) { }

      void handleCommand(JsonObject command, Print& out) override { } // NOP
      void handleConfig(JsonObject config, Print& out) override { } // NOP

      inline i2cip_fqa_t createFQA(const uint8_t& addr) const { return I2CIP::createFQA(this->getWireNum(), this->getModuleNum(), I2CIP_MUX_BUS_FAKE, addr); }
  };

  // extern NotAModule nomodule;
  // #ifdef CONTROLLER_HASWIRE1
  //   extern NotAModule nomodule1;
  // #endif

  void commandRouter(JsonObject command, Print& out);
  void rebuildTree(Print& out, bool update = false);
};

#include "I2CIP.tpp"

#endif