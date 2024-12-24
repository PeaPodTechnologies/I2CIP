#ifndef UNIT_TEST
#define UNIT_TEST 1
#define IS_MAIN 1

#include <Arduino.h>

#include "../test/config.h"

// Uncomment to enable debug
#include <debug.h>

#include <I2CIP.h>

using namespace I2CIP;

// #define MAIN_DEBUG_SERIAL Serial
#define MAIN_DEBUG_SERIAL DebugJsonOut

// Module* m;  // to be initialized in setup()
Module* modules[I2CIP_MUX_COUNT] = { nullptr };
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

void setup(void) {
  Serial.begin(115200);

  // OPTIONAL: FIRST TIME INIT
  bool r = initializeModule(WIRENUM, MODULE);
  if (!r) { /*delete modules[MODULE];*/ modules[MODULE] = nullptr; crashout(); }

  // NOTE: module.eeprom == nullptr; and module["eeprom"] == nullptr
}

i2cip_errorlevel_t errlev;
uint8_t cycle = 0;
unsigned long last = 0;

void loop(void) {
  last = millis();
  switch(checkModule(WIRENUM, MODULE)) {
    case I2CIP_ERR_HARD:
      // delete modules[MODULE];
      modules[MODULE] = nullptr;
      return;
    case I2CIP_ERR_SOFT:
      if (!initializeModule(WIRENUM, MODULE)) { /*delete modules[MODULE];*/ modules[MODULE] = nullptr; return; }
      break;
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

  delay(1000);
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
  modules[modulenum] = new Module(WIRENUM, MODULE);
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
  i2cip_errorlevel_t errlev = modules[modulenum]->operator()<EEPROM>(eeprom.getFQA(), true, _i2cip_args_io_default, DebugJsonBreakpoints);
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

  return errlev;
}

#endif