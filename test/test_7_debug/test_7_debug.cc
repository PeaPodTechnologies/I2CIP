#ifndef UNIT_TEST
#define UNIT_TEST 1
#define IS_MAIN 1

/** 
 * HEADER INCLUDES
 * - Arduino
 * - Unity (Testing Framework) 
 * - DebugJson (Serial Event Handling, Breakpoints, Telemetry)
 * - I2CIP (I2C Intra-network Protocols); Device Classes:
 *  - HT16K33 (7-Segment Display)
 *  - SHT45 (Temperature & Humidity Sensor)
 *  - JHD1313 (16x2 LCD w/ RGB Backlight)
 *  - Seesaw (Adafruit Multipurpose e.g. Rotary Encoder)
 **/

#include <Arduino.h>
#include <unity.h>

#include <DebugJson.h>

#include <debug.h>
#include <I2CIP.h>

#include <HT16K33.h>
#include <SHT45.h>
#include <JHD1313.h>
#include <Seesaw.h>

// using namespace I2CIP;

// DECLARATIONS

class DebugModule : public JsonModule {
  private:
  protected:
    DeviceGroup* deviceGroupFactory(const i2cip_id_t& id) override {
      DeviceGroup* dg = nullptr;
      dg = DeviceGroup::create<SHT45>(id);
      if(dg != nullptr) return dg;
      dg = DeviceGroup::create<JHD1313>(id);
      if(dg != nullptr) return dg;
      dg = DeviceGroup::create<RotaryEncoder>(id);
      if(dg != nullptr) return dg;
      dg = DeviceGroup::create<EEPROM>(id);
      return dg;
    }
  public:
    DebugModule(const uint8_t& wirenum, const uint8_t& modulenum) : JsonModule(wirenum, modulenum) { }

    // i2cip_errorlevel_t handleCommand(JsonObject command) {
    //   if(command.containsKey("fqa")) {
    //     // Device Command
        
    //   }
    //   return I2CIP_ERR_NONE; // NOP
    // }
};

// CONSTANTS

#define TEST_7_DEBUG_WIRENUM 0x00

// void test_initialize(void) {

// }

void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  while(!Serial) { digitalWrite(LED_BUILTIN, HIGH); delay(100); digitalWrite(LED_BUILTIN, LOW); delay(100); }

  delay(2000);

  UNITY_BEGIN();
}

/**
 * Module Detection
 * 
 * For each MUX address:
 * a. Ping MUX (if not found, fail errlev)
 * b. If not loaded, instantiate
 * c.
 */
void test_load_modules(void) {
  for(uint8_t m = 0; m < I2CIP_MUX_COUNT; m++) {
    if(I2CIP::MUX::pingMUX(TEST_7_DEBUG_WIRENUM, m)) {
      if(I2CIP::modules[m] == nullptr) {
        I2CIP::modules[m] = new DebugModule(TEST_7_DEBUG_WIRENUM, m);
      }

      I2CIP::errlev[m] = I2CIP::modules[m]->operator()();
      if(I2CIP::errlev[m] == I2CIP_ERR_NONE) {
        DebugJson::revision(m, Serial); // sends revision
      }
    } else {
      I2CIP::errlev[m] = I2CIP_ERR_HARD;
    }
    String msg = "Module " + String(m) + ": " + (I2CIP::modules[m] == nullptr ? "Null" : "0x" + String(I2CIP::errlev[m], HEX));
    TEST_IGNORE_MESSAGE(msg.c_str());
  }
}

void test_unload_modules(void) {
  for(uint8_t m = 0; m < I2CIP_MUX_COUNT; m++) {
    if(I2CIP::modules[m] != nullptr && I2CIP::errlev[m] == I2CIP_ERR_HARD) {
      delete I2CIP::modules[m];
      I2CIP::modules[m] = nullptr;
    }
  }
}

void loop(void) {

  RUN_TEST(test_unload_modules);
  RUN_TEST(test_load_modules);

  delay(1000);

  DebugJson::update(Serial, I2CIP::commandRouter);

}

#endif