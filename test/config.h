#ifndef I2CIP_TESTS_TEST_H_
#define I2CIP_TESTS_TEST_H_

#include <Arduino.h>

#include <DebugJson.h>

#include <I2CIP.hpp>

#include <SHT45.h>
#include <K30.h>
#include <HT16K33.h>
#include <PCA9685.h>
#include <JHD1313.h>
#include <Seesaw.h>
#include <MCP23017.h>
#include <Nunchuck.h>

// TESTING PARAMETERS
#define WIRENUM 0x00
#define MODULE  0x00
#define I2CIP_TEST_BUFFERSIZE 256 // Need to limit this, or else crash; I think Unity takes up a lot of stack space

#define I2CIP_TEST_EEPROM_BYTE0  '[' // This should be the first character of ANY valid SPRT EEPROM
#define I2CIP_TEST_EEPROM_BYTE1 '{'
#define I2CIP_TEST_EEPROM_WORD  (uint16_t)(I2CIP_TEST_EEPROM_BYTE << 8 | I2CIP_TEST_EEPROM_BYTE2)

#define I2CIP_TEST_EEPROM_OVERWRITE 1 // Uncomment to enable EEPROM overwrite test

// #define EEPROM_JSON_CONTENTS_TEST I2CIP_EEPROM_DEFAULT
#define EEPROM_JSON_CONTENTS_TEST {"[{\"24LC32\":[80],\"SHT45\":[" STR(I2CIP_SHT45_ADDRESS) "],\"SEESAW\":[" STR(I2CIP_SEESAW_ADDRESS) "]},{\"PCA9685\":[" STR(I2CIP_PCA9685_ADDRESS) "],\"JHD1313\":[" STR(I2CIP_JHD1313_ADDRESS) "],\"K30\":[" STR(I2CIP_K30_ADDRESS) "]},{\"MCP23017\":[" STR(I2CIP_MCP23017_ADDRESS) "]}]"}

// #ifdef ESP32
//   SET_LOOP_TASK_STACK_SIZE( 32*1024 ); // Thanks to: https://community.platformio.org/t/esp32-stack-configuration-reloaded/20994/8; https://github.com/espressif/arduino-esp32/pull/5173
// #endif

using namespace I2CIP;

class TestModule : public JsonModule {
  private:
  protected:
    DeviceGroup* deviceGroupFactory(const i2cip_id_t& id) override {
      DeviceGroup* dg = DeviceGroup::create<EEPROM>(id);
      if(dg != nullptr) return dg;
      dg = DeviceGroup::create<SHT45>(id);
      if(dg != nullptr) return dg;
      dg = DeviceGroup::create<K30>(id);
      if(dg != nullptr) return dg;
      dg = DeviceGroup::create<HT16K33>(id);
      if(dg != nullptr) return dg;
      dg = DeviceGroup::create<PCA9685>(id);
      if(dg != nullptr) return dg;
      dg = DeviceGroup::create<JHD1313>(id);
      if(dg != nullptr) return dg;
      dg = DeviceGroup::create<RotaryEncoder>(id);
      if(dg != nullptr) return dg;
      dg = DeviceGroup::create<MCP23017>(id);
      if(dg != nullptr) return dg;
      dg = DeviceGroup::create<Nunchuck>(id);
      return dg;
    }
  public:
    TestModule(const uint8_t wirenum, const uint8_t modulenum) : JsonModule(wirenum, modulenum) { }

    void handleCommand(JsonObject command, Print& out) override { 
      JsonDocument doc;
      doc["timestamp"] = millis();
      
      i2cip_fqa_t fqa = command["fqa"].as<i2cip_fqa_t>();
      
      Device** dptr = I2CIP::devicetree[fqa];
      if(dptr != nullptr && *dptr != nullptr) {

        Device* d = *dptr;

        DeviceGroup* dg = this->operator[](d->getID());

        if(dg != nullptr && dg->handler != nullptr) {
          i2cip_args_io_t args = _i2cip_args_io_default;
    
          JsonVariant argsG = command["g"];
          JsonVariant argsA = command["a"];
          JsonVariant argsS = command["s"];
          JsonVariant argsB = command["b"];
          
          dg->handler(args, argsA, argsS, argsB);

          // DebugJson::StringWriter sw;
          // i2cip_errorlevel_t errlev = this->operator()(d, true, args, sw);
          unsigned long start = millis();
          String msg = String(d->getID()) + ' ' + fqaToString(fqa) + ' ';
          bool spacer = false;

          i2cip_errorlevel_t errlev = MUX::setBus(fqa);
          if(errlev == I2CIP_ERR_NONE) {
            if(d->getOutput() != nullptr && !argsS.isNull()) {
              errlev = d->set(args.s, args.b);

              // Print output cache
              msg += "OUTSET ";
              msg += d->getOutput()->valueToString();
              spacer = true;
            }
            if(errlev == I2CIP_ERR_NONE && d->getInput() != nullptr && !argsG.isNull()) {
              errlev = d->get(args.a);

              // Print input cache
              if(spacer) msg += "; ";
              else spacer = true;
              msg += "INPGET ";
              msg += d->getInput()->printCache();

              if(errlev == I2CIP_ERR_NONE) {
                DebugJson::telemetryJsonString(d->getInput()->getLastRX(), d->getInput()->cacheToString());
              }
            }
            if(errlev == I2CIP_ERR_NONE && argsG.isNull() && argsS.isNull()) {
              errlev = d->pingTimeout(false, true);
              msg = "PING";
            }
          }

          dg->cleanup(args);

          msg += " DELTA ";
          msg += String(millis() - start);
          msg += "ms";

          msg += errlev == I2CIP_ERR_NONE ? " OK" : (errlev == I2CIP_ERR_SOFT ? " EINVAL" : " EIO");


          // TODO: Print to out with sw, errlev, timestamp
          doc["type"] = "info";
          doc["id"] = d->getID();
          doc["fqa"] = d->getFQA();
          doc["errlev"] = errlev;
          // doc["msg"] = sw.operator String();
          doc["msg"] = msg;
        } else {
          doc["type"] = "error";
          doc["msg"] = "LIBRARY ENOENT";
          doc["errlev"] = I2CIP_ERR_SOFT;
        }
      } else {
        doc["type"] = "error";
        doc["msg"] = "DEVICE ENOENT";
        doc["errlev"] = I2CIP_ERR_SOFT;
      }

      DebugJson::jsonPrintln(doc, out);
    }

    void handleConfig(JsonObject config, Print& out) override { 
      #ifdef I2CIP_DEBUG_SERIAL
        I2CIP_DEBUG_SERIAL.println(F("TestModule: handleConfig() called, but not implemented."));
      #endif
    }

    
};

/** FOR MAIN **/

// #define MAIN_DEBUG_SERIAL Serial
#define MAIN_DEBUG_SERIAL DebugJsonOut
#define CYCLE_DELAY 1000 // Max FPS 100Hz
#define HEARTBEAT_DELAY 1000 // Max FPS 1Hz
#define EPSILON_TEMPERATURE 0.5f
#define EPSILON_HUMIDITY 2.0f // 0.11f
#define LCD_REFRESH_MAX 1000
#define RGB_REFRESH_MAX 1 // Near-instantaneous

// PeaPod Stuff
#define PEAPOD_PIN_TOGGLE 33 // Multipurpose; Interpreted input from rotary button (Debounced; Toggle)
#define PEAPOD_PIN_OUT 32 // Multipurpose; Direct input from rotary button
#define PEAPOD_PIN_PWM1 15 // Multipurpose; Interpreted output from rotary knob (0-360 -> 0-255)
#define PEAPOD_PIN_PWM2 14 // Multipurpose; Interpreted output from rotary knob (0-360 -> 0-255)

#define DURATION_WATERING   2000     // 10 seconds every...
#define PERIOD_WATERING     10000   //  ...30 minutes
#define DURATION_LIGHTING   (TWENTYFOURHRS_MILLIS*1/2) // 12:12 hrs
#define PIN_WATERING        PEAPOD_PIN_OUT

// GLOBAL OBJECTS

// bool temphum = false;
state_sht45_t temphum = {NAN, NAN};
int32_t rotary_zero1 = 0;
int32_t rotary_zero2 = 0;
unsigned long last_lcd = LCD_REFRESH_MAX; unsigned long last_rgb = RGB_REFRESH_MAX; // Fixes first-frame bug
bool do_lcd = true;
float nunchuck_sum = 0.0f;

// PeaPod Stuff
uint8_t peapod_pwm1 = 0; // Pin 15
uint8_t peapod_pwm2 = 0; // Pin 14
bool peapod_toggle = false; // Pin 33
bool peapod_out = false; // Pin 32
bool flag_debounce = false; // Used to debounce for toggles

HT16K33 *ht16k33 = nullptr;

bool pinModeSet[255] = { false };

template <unsigned char P> void controlPin(const bool& s) {
  if(!pinModeSet[P]) { pinMode(P, OUTPUT); }
  if(s) {
    digitalWrite(P, HIGH);
  } else {
    digitalWrite(P, LOW);
  }
}
template <unsigned char P> void controlPWM(const uint8_t& v) {
  if(!pinModeSet[P]) { pinMode(P, OUTPUT); }
  analogWrite(P, v);
}

#ifdef FSM_STATE_H_
template <unsigned char P> void controlPin(bool _, const bool& s) { controlPin<P>(s); }

template <unsigned char P> void controlPWM(bool _, const FSM::Number& v) { 
  if(v.isFloating) {
    analogWrite(P, min(255, max(0, (int)((double)v * 255.f))));
  } else {
    analogWrite(P, min(255, max(0, (int)(v))));
  }
  controlPWM<P>(v);
}
#endif

void crashout(void) {
  // if(m != nullptr) m->operator()<HT16K33>(ht16k33, true); // Display "FAIL" on seven segment
  while(true) { // Blinks
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}


#endif