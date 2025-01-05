#ifndef I2CIP_TESTS_TEST_H_
#define I2CIP_TESTS_TEST_H_

#include "../src/debug.h"
#include <I2CIP.hpp>
#include <SHT45.h>
#include <HT16K33.h>
#include <PCA9685.h>
#include <JHD1313.h>
#include <Seesaw.h>

// TESTING PARAMETERS
#define WIRENUM 0x00
#define MODULE  0x00
#define I2CIP_TEST_BUFFERSIZE 100 // Need to limit this, or else crash; I think Unity takes up a lot of stack space

#define I2CIP_TEST_EEPROM_BYTE0  '[' // This should be the first character of ANY valid SPRT EEPROM
#define I2CIP_TEST_EEPROM_BYTE1 '{'
#define I2CIP_TEST_EEPROM_WORD  (uint16_t)(I2CIP_TEST_EEPROM_BYTE << 8 | I2CIP_TEST_EEPROM_BYTE2)

#define I2CIP_TEST_EEPROM_OVERWRITE 1 // Uncomment to enable EEPROM overwrite test

// #define EEPROM_JSON_CONTENTS_TEST I2CIP_EEPROM_DEFAULT
#define EEPROM_JSON_CONTENTS_TEST {"[{\"24LC32\":[80],\"SHT45\":[" STR(I2CIP_SHT45_ADDRESS) "],\"SEESAW\":[" STR(I2CIP_SEESAW_ADDRESS) "]},{\"PCA9685\":[" STR(I2CIP_PCA9685_ADDRESS) "],\"JHD1313\":[" STR(I2CIP_JHD1313_ADDRESS) "]}]"}

#ifdef ESP32
  SET_LOOP_TASK_STACK_SIZE( 32*1024 ); // Thanks to: https://community.platformio.org/t/esp32-stack-configuration-reloaded/20994/8; https://github.com/espressif/arduino-esp32/pull/5173
#endif

using namespace I2CIP;

class TestModule : public JsonModule {
  private:
  protected:
    DeviceGroup* deviceGroupFactory(const i2cip_id_t& id) override {
      DeviceGroup* dg = DeviceGroup::create<EEPROM>(id);
      if(dg != nullptr) return dg;
      dg = DeviceGroup::create<SHT45>(id);
      if(dg != nullptr) return dg;
      dg = DeviceGroup::create<HT16K33>(id);
      if(dg != nullptr) return dg;
      dg = DeviceGroup::create<PCA9685>(id);
      if(dg != nullptr) return dg;
      dg = DeviceGroup::create<JHD1313>(id);
      if(dg != nullptr) return dg;
      dg = DeviceGroup::create<RotaryEncoder>(id);
      return dg;
    }
  public:
    TestModule(const uint8_t wirenum, const uint8_t modulenum) : JsonModule(wirenum, modulenum) { }
};

#endif