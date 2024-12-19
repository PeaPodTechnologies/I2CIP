#ifndef UNIT_TEST
#define UNIT_TEST 1
#define IS_MAIN 1

#include <Arduino.h>

#include "../test/config.h"

// Uncomment to enable debug
#include <debug.h>

#include <I2CIP.h>

using namespace I2CIP;

#define DEBUG_SERIAL Serial

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
  DEBUG_SERIAL.begin(115200);

  // OPTIONAL: FIRST TIME INIT
  bool r = initializeModule(WIRENUM, MODULE);
  if (!r) { delete modules[MODULE]; modules[MODULE] = nullptr; crashout(); }

  // NOTE: module.eeprom == nullptr; and module["eeprom"] == nullptr
}

i2cip_errorlevel_t errlev;
uint8_t cycle = 0;
unsigned long last = 0;

void loop(void) {
  last = millis();
  switch(checkModule(WIRENUM, MODULE)) {
    case I2CIP_ERR_HARD:
      delete modules[MODULE];
      modules[MODULE] = nullptr;
      return;
    case I2CIP_ERR_SOFT:
      if (!initializeModule(WIRENUM, MODULE)) { delete modules[MODULE]; modules[MODULE] = nullptr; return; }
      break;
    default:
      errlev = updateModule(WIRENUM, MODULE);
      break;
  }

  // DEBUG PRINT: CYCLE COUNT, FPS, and ERRLEV
  unsigned long delta = millis() - last;
  DEBUG_SERIAL.print(F("[I2CIP | CYCLE "));
  DEBUG_SERIAL.print(cycle);
  DEBUG_SERIAL.print(F(" | "));
  DEBUG_SERIAL.print(1000.0 / delta, 0);
  DEBUG_SERIAL.print(F(" FPS | 0x"));
  DEBUG_SERIAL.print(errlev, HEX);
  DEBUG_SERIAL.println(F("]"));

  cycle++;

  delay(1000);
}


bool initializeModule(uint8_t wirenum, uint8_t modulenum) {
  DEBUG_SERIAL.print(F("[I2CIP] MODULE "));
  DEBUG_SERIAL.print(wirenum);
  DEBUG_SERIAL.print(":");
  DEBUG_SERIAL.print(modulenum);
  DEBUG_SERIAL.print(F(":.:. | INIT: "));

  if(modules[modulenum] != nullptr) {
    DEBUG_SERIAL.print(F("(DELETING) "));
    delete modules[modulenum];
  }

  // Initialize module
  unsigned long now = millis();
  modules[modulenum] = new Module(WIRENUM, MODULE);
  unsigned long delta = millis() - now;

  if(modules[modulenum] == nullptr) { 
    DEBUG_SERIAL.println(F("FAIL UNREACH"));
    return false;
  } else if((EEPROM*)(modules[modulenum]) == nullptr) {
    DEBUG_SERIAL.println(F("FAIL EEPROM"));
    delete modules[modulenum];
    modules[modulenum] = nullptr;
    return false;
  }

  DEBUG_SERIAL.print(delta / 1000.0, 3);
  DEBUG_SERIAL.println("s");

  return true;
}

i2cip_errorlevel_t checkModule(uint8_t wirenum, uint8_t modulenum) {
  DEBUG_SERIAL.print(F("[I2CIP] MODULE "));
  DEBUG_SERIAL.print(wirenum, HEX);
  DEBUG_SERIAL.print(":");
  DEBUG_SERIAL.print(modulenum, HEX);
  DEBUG_SERIAL.print(F(":.:. | CHECK: "));

  if(modules[modulenum] == nullptr) {
    DEBUG_SERIAL.println(F("FAIL ENOENT"));
    return I2CIP_ERR_SOFT; // ENOENT
  }

  unsigned long now = millis();
  i2cip_errorlevel_t errlev = modules[MODULE]->operator()();
  unsigned long delta = millis() - now;

  switch(errlev) {
    case I2CIP_ERR_HARD:
      DEBUG_SERIAL.print(F("FAIL EIO "));
      break;
    case I2CIP_ERR_SOFT:
      DEBUG_SERIAL.print(F("FAIL EINVAL "));
      break;
    default:
      DEBUG_SERIAL.print(F("PASS "));
      break;
  }
  DEBUG_SERIAL.print(delta / 1000.0, 3);
  DEBUG_SERIAL.println(F("s"));
  // I2CIP_ERR_BREAK(errlev);

  // // Continue - EEPROM check
  // i2cip_fqa_t eeprom = ((EEPROM*)modules[MODULE])->getFQA();

  // DEBUG_SERIAL.print(F("[I2CIP] EEPROM "));
  // DEBUG_SERIAL.print(wirenum);
  // DEBUG_SERIAL.print(":");
  // DEBUG_SERIAL.print(modulenum);
  // DEBUG_SERIAL.print(":");
  // DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(eeprom));
  // DEBUG_SERIAL.print(":");
  // DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(eeprom));
  // DEBUG_SERIAL.print(F(" - CHECK: "));

  // now = millis();
  // errlev = modules[MODULE]->operator()(eeprom);
  // delta = millis() - now;

  // switch(errlev) {
  //   case I2CIP_ERR_HARD:
  //     DEBUG_SERIAL.print(F("FAIL EIO "));
  //     break;
  //   case I2CIP_ERR_SOFT:
  //     DEBUG_SERIAL.print(F("FAIL EINVAL "));
  //     break;
  //   case I2CIP_ERR_NONE:
  //   default:
  //     DEBUG_SERIAL.print(F("PASS "));
  //     break;
  // }
  // DEBUG_SERIAL.print(delta / 1000.0, 3);
  // DEBUG_SERIAL.println(F("s"));
  return errlev;
}

i2cip_errorlevel_t updateModule(uint8_t wirenum, uint8_t modulenum) {
  DEBUG_SERIAL.print(F("[I2CIP] MODULE "));
  DEBUG_SERIAL.print(wirenum);
  DEBUG_SERIAL.print(":");
  DEBUG_SERIAL.print(modulenum);
  DEBUG_SERIAL.print(F(":.:. | UPDATE: "));

  if(modules[modulenum] == nullptr) {
    DEBUG_SERIAL.println(F("FAIL ENOENT"));
    return I2CIP_ERR_SOFT; // ENOENT
  }

  const EEPROM& eeprom = modules[modulenum]->operator const I2CIP::EEPROM &();

  DEBUG_SERIAL.print(F("EEPROM "));
  DEBUG_SERIAL.print(I2CIP_FQA_SEG_I2CBUS(eeprom.getFQA()), HEX);
  DEBUG_SERIAL.print(":");
  DEBUG_SERIAL.print(I2CIP_FQA_SEG_MODULE(eeprom.getFQA()), HEX);
  DEBUG_SERIAL.print(":");
  DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(eeprom.getFQA()), HEX);
  DEBUG_SERIAL.print(":");
  DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(eeprom.getFQA()), HEX);
  DEBUG_SERIAL.print(" ");

  unsigned long now = millis();
  i2cip_errorlevel_t errlev = modules[modulenum]->operator<EEPROM>()(eeprom.getFQA(), true);
  unsigned long delta = millis() - now;

  switch(errlev) {
    case I2CIP_ERR_HARD:
      DEBUG_SERIAL.print(F("FAIL EIO "));
      break;
    case I2CIP_ERR_SOFT:
      DEBUG_SERIAL.print(F("FAIL EINVAL "));
      break;
    default:
      DEBUG_SERIAL.print(F("PASS "));
      break;
  }
  DEBUG_SERIAL.print(delta / 1000.0, 3);
  DEBUG_SERIAL.print(F("s"));
  if(errlev != I2CIP_ERR_NONE) {
    DEBUG_SERIAL.println();
    return errlev;
  }

  // Bonus Points - Print EEPROM contents
  const char* cache = eeprom.getCache();
  if(cache == nullptr || cache[0] == '\0') {
    DEBUG_SERIAL.println(F(" EMPTY"));
    return errlev;
  }

  DEBUG_SERIAL.print(F(" \""));
  DEBUG_SERIAL.print(cache);
  DEBUG_SERIAL.println(F("\""));

  return errlev;
}

#endif