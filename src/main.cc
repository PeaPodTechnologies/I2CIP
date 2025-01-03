#ifndef UNIT_TEST
#define UNIT_TEST 1
#define IS_MAIN 1

#include <Arduino.h>

#include <DebugJson.h>

#include "../test/config.h"

// #define MAIN_DEBUG_SERIAL Serial
#define MAIN_DEBUG_SERIAL DebugJsonOut
#define CYCLE_DELAY 500

char idbuffer[10];

void crashout(void) {
  while(true) { // Blink
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}

bool initializeModule(uint8_t wirenum, uint8_t modulenum);
i2cip_errorlevel_t checkModule(uint8_t wirenum, uint8_t modulenum);
i2cip_errorlevel_t updateModule(uint8_t wirenum, uint8_t modulenum);

HT16K33 *ht16k33 = nullptr;

void setup(void) {
  Serial.begin(115200);

  // OPTIONAL: FIRST TIME INIT
  bool r = initializeModule(WIRENUM, MODULE);
  if (!r) { /*delete modules[MODULE];*/ modules[MODULE] = nullptr; crashout(); }

  // NOTE: module.eeprom == nullptr; and module["eeprom"] == nullptr
  ht16k33 = new HT16K33(WIRENUM, I2CIP_MUX_NUM_FAKE, I2CIP_MUX_BUS_FAKE, "SEVENSEG");

  i2cip_errorlevel_t errlev = modules[MODULE]->operator()<HT16K33>(ht16k33, false, _i2cip_args_io_default, DebugJsonBreakpoints);
  if(errlev != I2CIP_ERR_NONE) crashout();
}

i2cip_errorlevel_t errlev;
uint8_t cycle = 0;
unsigned long last = 0;

// bool temphum = false;

void loop(void) {
  last = millis();
  switch(checkModule(WIRENUM, MODULE)) {
    case I2CIP_ERR_HARD:
      // delete modules[MODULE];
      modules[MODULE] = nullptr;
      return;
    case I2CIP_ERR_SOFT:
      if (!initializeModule(WIRENUM, MODULE)) { /*delete modules[MODULE];*/ modules[MODULE] = nullptr; }
      return;
    default:
      errlev = updateModule(WIRENUM, MODULE);
      break;
  }

  // DEBUG PRINT: CYCLE COUNT, FPS, and ERRLEV
  unsigned long delta = millis() - last;
  MAIN_DEBUG_SERIAL.print(F("[I2CIP | CYCLE "));
  MAIN_DEBUG_SERIAL.print(cycle);
  MAIN_DEBUG_SERIAL.print(F(" | "));
  MAIN_DEBUG_SERIAL.print(1000.0 / delta, 0);
  MAIN_DEBUG_SERIAL.print(F(" FPS | 0x"));
  MAIN_DEBUG_SERIAL.print(errlev, HEX);
  MAIN_DEBUG_SERIAL.println(F("]"));

  cycle++;

  delay(CYCLE_DELAY);
}


bool initializeModule(uint8_t wirenum, uint8_t modulenum) {
  MAIN_DEBUG_SERIAL.print(F("[I2CIP] MODULE "));
  MAIN_DEBUG_SERIAL.print(wirenum);
  MAIN_DEBUG_SERIAL.print(":");
  MAIN_DEBUG_SERIAL.print(modulenum);
  MAIN_DEBUG_SERIAL.print(F(":.:. | INIT: "));

  if(modules[modulenum] != nullptr) {
    MAIN_DEBUG_SERIAL.print(F("(DELETED) "));
  //   delete modules[modulenum];
  }

  // Initialize module
  unsigned long now = millis();
  modules[modulenum] = new ESP32Module(WIRENUM, MODULE);
  unsigned long delta = millis() - now;

  if(modules[modulenum] == nullptr) { 
    MAIN_DEBUG_SERIAL.println(F("FAIL UNREACH"));
    return false;
  } else if((EEPROM*)(modules[modulenum]) == nullptr) {
    MAIN_DEBUG_SERIAL.println(F("FAIL EEPROM"));
    // delete modules[modulenum];
    modules[modulenum] = nullptr;
    return false;
  }

  MAIN_DEBUG_SERIAL.print(delta / 1000.0, 3);
  MAIN_DEBUG_SERIAL.println("s");

  return true;
}

i2cip_errorlevel_t checkModule(uint8_t wirenum, uint8_t modulenum) {
  MAIN_DEBUG_SERIAL.print(F("[I2CIP] MODULE "));
  MAIN_DEBUG_SERIAL.print(wirenum, HEX);
  MAIN_DEBUG_SERIAL.print(":");
  MAIN_DEBUG_SERIAL.print(modulenum, HEX);
  MAIN_DEBUG_SERIAL.print(F(":.:. | CHECK: "));

  if(modules[modulenum] == nullptr) {
    MAIN_DEBUG_SERIAL.println(F("FAIL ENOENT"));
    return I2CIP_ERR_SOFT; // ENOENT
  }

  unsigned long now = millis();
  i2cip_errorlevel_t errlev = modules[MODULE]->operator()();
  unsigned long delta = millis() - now;

  switch(errlev) {
    case I2CIP_ERR_HARD:
      MAIN_DEBUG_SERIAL.print(F("FAIL EIO "));
      break;
    case I2CIP_ERR_SOFT:
      MAIN_DEBUG_SERIAL.print(F("FAIL EINVAL "));
      break;
    default:
      MAIN_DEBUG_SERIAL.print(F("PASS "));
      break;
  }
  MAIN_DEBUG_SERIAL.print(delta / 1000.0, 3);
  MAIN_DEBUG_SERIAL.println(F("s"));
  // I2CIP_ERR_BREAK(errlev);

  // // Continue - EEPROM check
  // i2cip_fqa_t eeprom = ((EEPROM*)modules[MODULE])->getFQA();

  // MAIN_DEBUG_SERIAL.print(F("[I2CIP] EEPROM "));
  // MAIN_DEBUG_SERIAL.print(wirenum);
  // MAIN_DEBUG_SERIAL.print(":");
  // MAIN_DEBUG_SERIAL.print(modulenum);
  // MAIN_DEBUG_SERIAL.print(":");
  // MAIN_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(eeprom));
  // MAIN_DEBUG_SERIAL.print(":");
  // MAIN_DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(eeprom));
  // MAIN_DEBUG_SERIAL.print(F(" - CHECK: "));

  // now = millis();
  // errlev = modules[MODULE]->operator()(eeprom);
  // delta = millis() - now;

  // switch(errlev) {
  //   case I2CIP_ERR_HARD:
  //     MAIN_DEBUG_SERIAL.print(F("FAIL EIO "));
  //     break;
  //   case I2CIP_ERR_SOFT:
  //     MAIN_DEBUG_SERIAL.print(F("FAIL EINVAL "));
  //     break;
  //   case I2CIP_ERR_NONE:
  //   default:
  //     MAIN_DEBUG_SERIAL.print(F("PASS "));
  //     break;
  // }
  // MAIN_DEBUG_SERIAL.print(delta / 1000.0, 3);
  // MAIN_DEBUG_SERIAL.println(F("s"));
  return errlev;
}

i2cip_errorlevel_t updateModule(uint8_t wirenum, uint8_t modulenum) {
  MAIN_DEBUG_SERIAL.print(F("[I2CIP] MODULE "));
  MAIN_DEBUG_SERIAL.print(wirenum);
  MAIN_DEBUG_SERIAL.print(":");
  MAIN_DEBUG_SERIAL.print(modulenum);
  MAIN_DEBUG_SERIAL.print(F(":.:. | UPDATE: "));

  if(modules[modulenum] == nullptr) {
    MAIN_DEBUG_SERIAL.println(F("FAIL ENOENT"));
    return I2CIP_ERR_SOFT; // ENOENT
  }

  const EEPROM& eeprom = modules[modulenum]->operator const I2CIP::EEPROM &();

  MAIN_DEBUG_SERIAL.print(F("EEPROM "));
  MAIN_DEBUG_SERIAL.print(I2CIP_FQA_SEG_I2CBUS(eeprom.getFQA()), HEX);
  MAIN_DEBUG_SERIAL.print(":");
  MAIN_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MODULE(eeprom.getFQA()), HEX);
  MAIN_DEBUG_SERIAL.print(":");
  MAIN_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(eeprom.getFQA()), HEX);
  MAIN_DEBUG_SERIAL.print(":");
  MAIN_DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(eeprom.getFQA()), HEX);
  MAIN_DEBUG_SERIAL.print(" ");

  unsigned long now = millis();
  i2cip_errorlevel_t errlev = modules[modulenum]->operator()<EEPROM>(eeprom, true, _i2cip_args_io_default, DebugJsonBreakpoints);
  unsigned long delta = millis() - now;

  switch(errlev) {
    case I2CIP_ERR_HARD:
      MAIN_DEBUG_SERIAL.print(F("FAIL EIO "));
      break;
    case I2CIP_ERR_SOFT:
      MAIN_DEBUG_SERIAL.print(F("FAIL EINVAL "));
      break;
    default:
      MAIN_DEBUG_SERIAL.print(F("PASS "));
      break;
  }
  MAIN_DEBUG_SERIAL.print(delta / 1000.0, 3);
  MAIN_DEBUG_SERIAL.print(F("s"));
  if(errlev != I2CIP_ERR_NONE) {
    MAIN_DEBUG_SERIAL.println();
    return errlev;
  }

  // Bonus Points - Print EEPROM contents
  const char* cache = eeprom.getCache();
  if(cache == nullptr || cache[0] == '\0') {
    MAIN_DEBUG_SERIAL.println(F(" EMPTY"));
    return errlev;
  }

  MAIN_DEBUG_SERIAL.print(F(" \""));
  MAIN_DEBUG_SERIAL.print(cache);
  MAIN_DEBUG_SERIAL.println(F("\""));

  if(errlev == I2CIP_ERR_NONE) {
    HashTableEntry<I2CIP::DeviceGroup&>* entry = I2CIP::devicegroups[SHT45::getID()];

    uint8_t c = 0; state_sht45_t state = {0.0f, 0.0f};
    if(entry != nullptr && entry->value.numdevices > 0) {
      for(uint8_t x = 0; x < entry->value.numdevices; x++) {
        if(entry->value.devices[x] == nullptr) continue;
        
        errlev = modules[MODULE]->operator()<SHT45>((SHT45*)entry->value.devices[x], true, _i2cip_args_io_default, DebugJsonBreakpoints);
        if(errlev != I2CIP_ERR_NONE) {break;}
        c++;

        state.temperature += ((SHT45*)(entry->value.devices[x]))->getCache().temperature;
        state.humidity += ((SHT45*)(entry->value.devices[x]))->getCache().humidity;
      }
      state.temperature /= c; state.humidity /= c;

      if(errlev == I2CIP_ERR_NONE) {
        // SEVENSEG
        i2cip_ht16k33_mode_t mode = SEG_1F;
        // i2cip_ht16k33_data_t data = { .f = temphum ? state.temperature : state.humidity };
        i2cip_ht16k33_data_t data = { .f = state.temperature };
        i2cip_args_io_t hargs = { .a = nullptr, .s = &data.f, .b = &mode };
        
        // errlev = 
          modules[MODULE]->operator()<HT16K33>(ht16k33, true, hargs, DebugJsonBreakpoints);
        // temphum = !temphum;

        // PWM
        uint16_t pwm12 = (uint16_t)(0xFFF * max(0.f, state.temperature) / 100.f); // Celsius to PWM
        i2cip_pca9685_chsel_t ch = PCA9685_CH0;
        i2cip_args_io_t pargs = { .a = nullptr, .s = &pwm12, .b = &ch };

        entry = I2CIP::devicegroups[PCA9685::getID()];
        if(entry != nullptr && entry->value.numdevices > 0) {
          for(uint8_t x = 0; x < entry->value.numdevices; x++) {
            if(entry->value.devices[x] == nullptr) continue;
            modules[MODULE]->operator()<PCA9685>((PCA9685*)entry->value.devices[x], true, pargs, DebugJsonBreakpoints);
          }
        }

        // LCD
        String msg = String("Time: ") + String(last / 1000.f, 3) + "s\nT:" + String(state.temperature, 1) + "C PWM:" + String(pwm12, HEX) + "H";
        i2cip_args_io_t largs = { .a = nullptr, .s = &msg, .b = nullptr };
        entry = I2CIP::devicegroups[JHD1313::getID()];
        if(entry != nullptr && entry->value.numdevices > 0) {
          for(uint8_t x = 0; x < entry->value.numdevices; x++) {
            if(entry->value.devices[x] == nullptr) continue;
            modules[MODULE]->operator()<JHD1313>((JHD1313*)entry->value.devices[x], true, largs, DebugJsonBreakpoints);
          }
        }
      } else {
        // SEVENSEG
        i2cip_ht16k33_mode_t mode = SEG_1F;
        // i2cip_ht16k33_data_t data = { .f = temphum ? state.temperature : state.humidity };
        i2cip_ht16k33_data_t data = { .f = NAN };
        i2cip_args_io_t hargs = { .a = nullptr, .s = &data.f, .b = &mode };
        
        // errlev = 
          modules[MODULE]->operator()<HT16K33>(ht16k33, true, hargs, DebugJsonBreakpoints);
      }
    }
  }

  return errlev;
}

#endif