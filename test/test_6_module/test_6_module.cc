#include <Arduino.h>
#include <unity.h>

#include "../config.h"

#include <debug.h>
#include <I2CIP.h>

#include <HT16K33.hpp>
#include <SHT45.h>

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
}

state_sht45_t cache = { 0.0f, 0.0f };
i2cip_ht16k33_data_t data = { .f = NAN }; // = { 0xCACA }; // 0xCAFE; // 0xBEEF; // 0xDEAD; // 0x1337; // LEET
i2cip_ht16k33_mode_t mode = SEG_1F; // SEG_HEX16;
i2cip_args_io_t args = { .a = nullptr, .s = &data.f, .b = &mode };
HT16K33 *ht16k33 = nullptr;
SHT45 *sht45 = nullptr;

void test_ht16k33_oop(void) {
  ht16k33 = new HT16K33(WIRENUM, I2CIP_MUX_NUM_FAKE, I2CIP_MUX_BUS_FAKE, "SEVENSEG"); // Fakeout constructor
  TEST_ASSERT_TRUE_MESSAGE(ht16k33 != nullptr, "HT16K33 Fakeout Constructor");
}

void test_sht45_oop(void) {
  sht45 = new SHT45(createFQA(WIRENUM, MODULE, 0, I2CIP_SHT45_ADDRESS), "SHT45"); // Fakeout constructor
  TEST_ASSERT_TRUE_MESSAGE(sht45 != nullptr, "SHT45 Default Constructor");
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

  delay(1000);

  RUN_TEST(test_sht45_oop);

  delay(1000);

  RUN_TEST(test_ht16k33_oop);

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
  
  uint8_t len = strlen_P(i2cip_eeprom_default);
  char str[len+1] = { '\0' };
  for (uint8_t k = 0; k < len; k++) {
    char c = pgm_read_byte_near(i2cip_eeprom_default + k);
    str[k] = c;
  }

  str[len] = '\0';

  const char* cache = (m->operator const I2CIP::EEPROM &()).getCache();
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

  TEST_ASSERT_EQUAL_STRING_MESSAGE(str, cache, "GET Cache Mismatch");
  // TEST_ASSERT_EQUAL_STRING_MESSAGE(str, value, "SET Value Mismatch");
}

void test_ht16k33_ping(void) {
  if(ht16k33 == nullptr) {
    TEST_IGNORE_MESSAGE("HT16K33 Null");
    return;
  }
  char msg[30];
  sprintf(msg, "%s EIO", fqaToString(ht16k33->getFQA()));
  i2cip_errorlevel_t result = ht16k33->ping();
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_ERR_NONE, result, msg);
  if(result > I2CIP_ERR_NONE) end = true;
}

void test_ht16k33_write_string(void) {
  if(ht16k33 == nullptr) {
    TEST_IGNORE_MESSAGE("HT16K33 Null");
    return;
  }
  // i2cip_errorlevel_t result = module->operator()<HT16K33>(WIRENUM, 0x07, 0x07, true, args, Serial);
  i2cip_errorlevel_t result = m->operator()<HT16K33>(ht16k33, true, args);
  // i2cip_errorlevel_t result = m->operator()<HT16K33>(ht16k33, true);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_ERR_NONE, result, "HT16K33 Overwrite");
  // if(result > I2CIP_ERR_NONE) end = true;
}

void test_sht45_ping(void) {
  if(sht45 == nullptr) {
    TEST_IGNORE_MESSAGE("SHT45 Null");
    return;
  }
  char msg[30];
  sprintf(msg, "%s EIO", fqaToString(sht45->getFQA()));
  i2cip_errorlevel_t result = sht45->ping();
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_ERR_NONE, result, msg);
  if(result > I2CIP_ERR_NONE) end = true;
}

void test_sht45_read(void) {
  if(sht45 == nullptr) {
    TEST_IGNORE_MESSAGE("SHT45 Null");
    return;
  }
  i2cip_errorlevel_t result = m->operator()<SHT45>(sht45, true);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_ERR_NONE, result, "SHT45 Read");
  if(result > I2CIP_ERR_NONE) { end = true; return; }
  cache = sht45->getCache();
  data.f = cache.temperature;
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

  if (!end) {RUN_TEST(test_ht16k33_ping);

  delay(1000);}

  if (!end) {RUN_TEST(test_ht16k33_write_string);

  delay(1000);}

  if (!end) {RUN_TEST(test_sht45_ping);

  delay(1000);}

  if (!end) {RUN_TEST(test_sht45_read);

  delay(1000);}

  if(end || count == 0) {
    delay(1000);

    UNITY_END();

    delay(2000);

    end = true;
  }

  delay(1000);
  count--;
}
