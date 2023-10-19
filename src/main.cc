#ifndef UNIT_TEST
#include <Arduino.h>

// Uncomment to enable debug
#define DEBUG_SERIAL Serial
#include <debug.h>

#include <I2CIP.h>

using namespace I2CIP;

Module* m;  // to be initialized in setup()
char idbuffer[10];

void setup(void) {
  DEBUG_SERIAL.begin(115200);

  DEBUG_SERIAL.println("\nInitializing Module...");

  // Initialize module
  m = new Module(0, 0);

  // if((EEPROM*)(m) == nullptr) while(true);

  i2cip_fqa_t fqa = ((const EEPROM&)(*m)).getFQA();
  DEBUG_SERIAL.print("BasicModule Initialized! EEPROM FQA ");
  DEBUG_SERIAL.print(I2CIP_FQA_SEG_I2CBUS(fqa), HEX);
  DEBUG_SERIAL.print(":");
  DEBUG_SERIAL.print(I2CIP_FQA_SEG_MODULE(fqa), HEX);
  DEBUG_SERIAL.print(":");
  DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(fqa), HEX);
  DEBUG_SERIAL.print(":");
  DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
  DEBUG_SERIAL.print(" ID '");
  DEBUG_SERIAL.print(((const EEPROM&)(*m)).getID());
  DEBUG_SERIAL.print(F("' @0x"));
  DEBUG_SERIAL.print((uint16_t)&((const EEPROM&)(*m)).getID()[0], HEX);  
  DEBUG_SERIAL.print('\n');

  // Build module
  if(!:discover(*m)) while(true) { // Blink
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}
i2cip_errorlevel_t errlev;

void loop(void) {
  errlev = (*m)();
  DEBUG_SERIAL.print(F("ERRORLEVEL: "));
  DEBUG_SERIAL.print(errlev);
  DEBUG_SERIAL.print('\n');
  delay(1000);

  errlev = (*m)(((const EEPROM&)(*m)).getFQA());
  DEBUG_SERIAL.print(F("ERRORLEVEL: "));
  DEBUG_SERIAL.print(errlev);
  DEBUG_SERIAL.print('\n');
  delay(1000);
}

#endif