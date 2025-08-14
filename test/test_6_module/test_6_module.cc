#include <Arduino.h>
#include <unity.h>

#include "../config.h"

#include <debug.h>
#include <I2CIP.h>

using namespace I2CIP;

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

  i2cip_errorlevel_t errlev = m->discoverEEPROM();
  // if(!r) {
  //   while (true) { // Blink
  //     digitalWrite(LED_BUILTIN, HIGH);
  //     delay(100);
  //     digitalWrite(LED_BUILTIN, LOW);
  //     delay(100);
  //   }
  // }

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_ERR_NONE, errlev, "Module Discovery Fail");

  DeviceGroup* eeprom_group = m->operator[](EEPROM::getID());
  TEST_ASSERT_TRUE_MESSAGE(eeprom_group != nullptr, "Module EEPROM Group Not Found");
  EEPROM* eeprom = (EEPROM*)eeprom_group->operator[](m->operator const I2CIP::EEPROM &().getFQA());
  TEST_ASSERT_TRUE_MESSAGE(eeprom != nullptr, "Module EEPROM Not Found");
  
  i2cip_fqa_t fqa = createFQA(m->getWireNum(), m->getModuleNum(), 0, I2CIP_EEPROM_ADDR);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(fqa, eeprom->getFQA(), "Module EEPROM FQA Mismatch");

  Device** d = I2CIP::devicetree.operator[](fqa);
  TEST_ASSERT_TRUE_MESSAGE(d != nullptr, "Module EEPROM Not Found in Device Tree");
  TEST_ASSERT_EQUAL_PTR_MESSAGE(*d, eeprom, "Module EEPROM Not Found in Device Tree");
}

void setup(void) {
  Serial.begin(115200);

  delay(2000);

  UNITY_BEGIN();
  // @0x3FFBDB7C
  // @0x3FFB2188
  RUN_TEST(test_module_init);

  delay(1000);

  RUN_TEST(test_module_discovery);

  // i2cip_fqa_t fqa = (m->operator const I2CIP::EEPROM &());

  // #ifdef DEBUG_SERIAL
  //   DEBUG_DELAY();
  //   DEBUG_SERIAL.print(F("EEPROM FQA "));
  //   DEBUG_SERIAL.print(I2CIP_FQA_SEG_I2CBUS(fqa), HEX);
  //   DEBUG_SERIAL.print(':');
  //   DEBUG_SERIAL.print(I2CIP_FQA_SEG_MODULE(fqa), HEX);
  //   DEBUG_SERIAL.print(':');
  //   DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(fqa), HEX);
  //   DEBUG_SERIAL.print(':');
  //   DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
  //   DEBUG_SERIAL.print(" ID '");
  //   DEBUG_SERIAL.print((m->operator const I2CIP::EEPROM &()).getID());
  //   DEBUG_SERIAL.print("' @0x");
  //   DEBUG_SERIAL.print((uint16_t)&(m->operator const I2CIP::EEPROM &()).getID()[0], HEX);  
  //   DEBUG_SERIAL.print('\n');
  // #endif

  delay(1000);
}

static bool end = false;

void test_module_self_check(void) {
  i2cip_errorlevel_t errlev = m->operator()();
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_ERR_NONE, errlev, "Self-check failed! Check module wiring.");
  if(errlev > I2CIP_ERR_NONE) end = true;
}

void test_module_eeprom_check(void) {
  i2cip_errorlevel_t errlev = m->operator()<EEPROM>(m->operator const I2CIP::EEPROM &());
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_ERR_NONE, errlev, "EEPROM check failed! Check EEPROM wiring.");
  if(errlev > I2CIP_ERR_NONE) end = true;
}

void test_module_eeprom_update(void) {
  // i2cip_errorlevel_t errlev = (*m)((m->operator const I2CIP::EEPROM &()), true);
  i2cip_errorlevel_t errlev = m->operator()<EEPROM>(m->operator const I2CIP::EEPROM &(), true);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_ERR_NONE, errlev, "EEPROM read/write failed! Check EEPROM wiring.");
  if(errlev > I2CIP_ERR_NONE) end = true;
  
  // uint8_t len = strlen_P(i2cip_eeprom_default);
  // char str[len+1] = { '\0' };
  // for (uint8_t k = 0; k < len; k++) {
  //   char c = pgm_read_byte_near(i2cip_eeprom_default + k);
  //   str[k] = c;
  // }

  // str[len] = '\0';

  // const char* cache = (m->operator const I2CIP::EEPROM &()).getCache();
  // const char* value = (m->operator const I2CIP::EEPROM &()).getValue();

  // #ifdef DEBUG_SERIAL
  //   DEBUG_DELAY();
  //   DEBUG_SERIAL.print(F("EEPROM Cache Test: '"));
  //   DEBUG_SERIAL.print(cache);
  //   DEBUG_SERIAL.print(F("' @0x"));
  //   DEBUG_SERIAL.print((uint16_t)cache, HEX);
  //   DEBUG_SERIAL.print(F("\nEEPROM Value: "));
  //   DEBUG_SERIAL.print(value);
  //   DEBUG_SERIAL.print(F(" @0x"));
  //   DEBUG_SERIAL.print((uint16_t)value, HEX);
  //   DEBUG_SERIAL.print('\n');
  //   DEBUG_DELAY();
  // #endif

  // TEST_ASSERT_EQUAL_STRING_MESSAGE(str, cache, "GET Cache Mismatch");
  // TEST_ASSERT_EQUAL_STRING_MESSAGE(str, value, "SET Value Mismatch");
}

void test_module_delete(void) {
  delete(m);

  TEST_PASS();
}

i2cip_errorlevel_t errlev;
uint8_t count = 2;
void loop(void) {
  if (!end) {RUN_TEST(test_module_self_check);

  delay(1000);}

  if (!end) {RUN_TEST(test_module_eeprom_check);
  
  delay(1000);}

  if (!end) {RUN_TEST(test_module_eeprom_update); // NOTE: Does not test EEPROM overwrite - see test_3_eeprom::test_eeprom_overwrite_contents

  delay(1000);}

  if(end || count == 0) {
    delay(1000);
    
    RUN_TEST(test_module_delete);

    delay(1000);

    UNITY_END();

    delay(2000);

    end = true;
  }

  delay(1000);
  count--;
}
