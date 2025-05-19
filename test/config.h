#ifndef I2CIP_TESTS_TEST_H_
#define I2CIP_TESTS_TEST_H_

#include "../src/debug.h"
#include <I2CIP.hpp>
#include <SHT45.h>
#include <HT16K33.h>
#include <PCA9685.h>
#include <JHD1313.h>
#include <Seesaw.h>
#include <Nunchuck.h>

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

/** FOR MAIN **/

// #define MAIN_DEBUG_SERIAL Serial
#define MAIN_DEBUG_SERIAL DebugJsonOut
#define CYCLE_DELAY 20 // Max FPS 100Hz
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

#ifdef FSM_STATE_H_
// PeaPod Finite State Machine Flags
FSM::Variable cycle(FSM::Number(0, false, false), "cycle");
FSM::Flag watering(false);
// FSM::Flag lighting(false);
#endif

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