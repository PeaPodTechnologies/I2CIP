#include <Arduino.h>
#include <unity.h>

#include "../config.h"

using namespace I2CIP;

#define DEBUG_SERIAL Serial // Uncomment to enable debug

Module* m;  // to be initialized in setup()

void test_module_init(void) {
  #ifdef DEBUG_SERIAL
    DEBUG_SERIAL.println(F("==== [ Initializing Module ] ===="));
    DEBUG_DELAY();
  #endif

  m = new Module(0, 0);
  TEST_ASSERT_TRUE_MESSAGE(m != nullptr, "Module Initialization Fail");

  if(m == nullptr) while(true) { // Blink
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
  else {
    #ifdef DEBUG_SERIAL
      DEBUG_SERIAL.println(F("Barebones Module Initialized!"));
      DEBUG_DELAY();
    #endif
  }
}

void test_module_discovery(void) {
  #ifdef DEBUG_SERIAL
    DEBUG_SERIAL.println(F("==== [ Discovering Module ] ===="));
    DEBUG_DELAY();
  #endif

  bool r = m->discover();
  // if(!r) {
  //   while (true) { // Blink
  //     digitalWrite(LED_BUILTIN, HIGH);
  //     delay(100);
  //     digitalWrite(LED_BUILTIN, LOW);
  //     delay(100);
  //   }
  // }

  TEST_ASSERT_TRUE_MESSAGE(r, "Module Discovery Fail");
  if(r) {
    #ifdef DEBUG_SERIAL
      DEBUG_SERIAL.print(F("Module Discovered\n"));
      DEBUG_DELAY();
    #endif
  }
}

void setup(void) {
  Serial.begin(115200);

  delay(2000);

  UNITY_BEGIN();
  
  RUN_TEST(test_module_init);

  i2cip_fqa_t fqa = ((const EEPROM&)(*m));

  #ifdef DEBUG_SERIAL
    DEBUG_DELAY();
    DEBUG_SERIAL.print(F("EEPROM FQA "));
    DEBUG_SERIAL.print(I2CIP_FQA_SEG_I2CBUS(fqa), HEX);
    DEBUG_SERIAL.print(':');
    DEBUG_SERIAL.print(I2CIP_FQA_SEG_MODULE(fqa), HEX);
    DEBUG_SERIAL.print(':');
    DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(fqa), HEX);
    DEBUG_SERIAL.print(':');
    DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
    DEBUG_SERIAL.print(" ID '");
    DEBUG_SERIAL.print(((const EEPROM&)(*m)).getID());
    DEBUG_SERIAL.print("' @0x");
    DEBUG_SERIAL.print((uint16_t)&((const EEPROM&)(*m)).getID()[0], HEX);  
    DEBUG_SERIAL.print('\n');
  #endif

  delay(1000);

  // Build module
  RUN_TEST(test_module_discovery);

  delay(1000);
}

static bool fail = false;

void test_module_self_check(void) {
  i2cip_errorlevel_t errlev = (*m)();
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_ERR_NONE, errlev, "Self-check failed! Check module wiring.");
  if(errlev > I2CIP_ERR_NONE) fail = true;
}

void test_module_eeprom_check(void) {
  i2cip_errorlevel_t errlev = (*m)(((const EEPROM&)(*m)));
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_ERR_NONE, errlev, "EEPROM check failed! Check EEPROM wiring.");
  if(errlev > I2CIP_ERR_NONE) fail = true;
}

void test_module_eeprom_update(void) {
  i2cip_errorlevel_t errlev = (*m)(((const EEPROM&)(*m)), true);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_ERR_NONE, errlev, "EEPROM read/write failed! Check EEPROM wiring.");
  if(errlev > I2CIP_ERR_NONE) fail = true;
  
  uint8_t len = strlen_P(i2cip_eeprom_default);
  char str[len+1] = { '\0' };
  for (uint8_t k = 0; k < len; k++) {
    char c = pgm_read_byte_near(i2cip_eeprom_default + k);
    str[k] = c;
  }

  str[len] = '\0';

  const char* cache = ((const EEPROM&)(*m)).getCache();
  const char* value = ((const EEPROM&)(*m)).getValue();

  #ifdef DEBUG_SERIAL
    DEBUG_DELAY();
    DEBUG_SERIAL.print(F("EEPROM Cache Test: '"));
    DEBUG_SERIAL.print(cache);
    DEBUG_SERIAL.print(F("' @0x"));
    DEBUG_SERIAL.print((uint16_t)cache, HEX);
    DEBUG_SERIAL.print(F("\nEEPROM Value: "));
    DEBUG_SERIAL.print(value);
    DEBUG_SERIAL.print(F(" @0x"));
    DEBUG_SERIAL.print((uint16_t)value, HEX);
    DEBUG_SERIAL.print('\n');
    DEBUG_DELAY();
  #endif

  TEST_ASSERT_EQUAL_STRING_MESSAGE(str, cache, "GET Cache Mismatch");
  TEST_ASSERT_EQUAL_STRING_MESSAGE(str, value, "SET Value Mismatch");
}

i2cip_errorlevel_t errlev;
uint8_t count = 1;
void loop(void) {

  RUN_TEST(test_module_self_check);

  if(fail) goto stop;

  delay(1000);

  RUN_TEST(test_module_eeprom_check);

  if(fail) goto stop;
  
  #if I2CIP_TEST_EEPROM_OVERWRITE == 1
  delay(1000);

  RUN_TEST(test_module_eeprom_update);

  if(fail) goto stop;
  #endif

  if(count > 3) {
    stop: UNITY_END();
    while(true);
  }

  delay(1000);
  count++;
}
