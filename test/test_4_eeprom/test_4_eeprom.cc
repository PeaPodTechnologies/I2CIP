#include <Arduino.h>
#include <unity.h>

#include <i2cip/eeprom.h>
#include <i2cip/mux.h>
#include <I2CIP.h>
#include "../config.h"

#define I2CIP_TEST_EEPROM_BYTE '[' // This should be the first character of ANY valid SPRT EEPROM

extern i2cip_fqa_t eeprom_fqa;
char buffer[I2CIP_TEST_BUFFERSIZE] = { '\0' };

// ALL MACROS

void test_eeprom_ping(void) {
  I2CIP::i2cip_errorlevel_t result = I2CIP::Device::ping(eeprom_fqa);
  TEST_ASSERT_EQUAL_UINT8(I2CIP::I2CIP_ERR_NONE, result);
}

void test_eeprom_read_byte(void) {
  uint8_t c = '\0';
  I2CIP::i2cip_errorlevel_t result = I2CIP::Routing::EEPROM::readByte(eeprom_fqa, 0, c);
  TEST_ASSERT_EQUAL_UINT8(I2CIP::I2CIP_ERR_NONE, result);
  TEST_ASSERT_EQUAL_UINT8(I2CIP_TEST_EEPROM_BYTE, c);
}

void test_eeprom_read_contents(void) {
  uint16_t size = 0;
  I2CIP::i2cip_errorlevel_t result = I2CIP::Routing::EEPROM::readContents(eeprom_fqa, (uint8_t*)buffer, size, I2CIP_TEST_BUFFERSIZE);
  TEST_ASSERT_EQUAL_UINT8(I2CIP::I2CIP_ERR_NONE, result);
  TEST_ASSERT_NOT_EQUAL(0, size);
}

void setup() {
  delay(2000);

  UNITY_BEGIN();

  RUN_TEST(test_eeprom_ping);
  RUN_TEST(test_eeprom_read_byte);
  // RUN_TEST(test_eeprom_write_byte);
  RUN_TEST(test_eeprom_read_contents);

  UNITY_END();
}

void loop() {

}