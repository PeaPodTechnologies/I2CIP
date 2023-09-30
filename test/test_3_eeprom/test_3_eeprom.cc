#include <Arduino.h>
#include <unity.h>

#include <eeprom.h>
#include "../config.h"

// ALL MACROS

char buffer[I2CIP_TEST_BUFFERSIZE] = { '\0' };
size_t bufferlen = 0;

void test_eeprom_ping(void) {
  char msg[30];
  sprintf(msg, "Device unreachable (%01X.%01X.%01X.%02X)", I2CIP_FQA_SEG_I2CBUS(eeprom_fqa), I2CIP_FQA_SEG_MODULE(eeprom_fqa), I2CIP_FQA_SEG_MUXBUS(eeprom_fqa), I2CIP_FQA_SEG_DEVADR(eeprom_fqa));
  I2CIP::i2cip_errorlevel_t result = eeprom.ping();
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP::I2CIP_ERR_NONE, result, msg);
}

void test_eeprom_write_byte(void) {
  I2CIP::i2cip_errorlevel_t result = eeprom.writeRegister((uint16_t)0, I2CIP_TEST_EEPROM_BYTE0);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP::I2CIP_ERR_NONE, result, "EEPROM Write Byte");
}

void test_eeprom_read_byte(void) {
  uint8_t c = '\0';
  I2CIP::i2cip_errorlevel_t result = eeprom.readRegisterByte((uint16_t)0, c);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP::I2CIP_ERR_NONE, result, "EEPROM Read Byte");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_TEST_EEPROM_BYTE0, c, "EEPROM Read Byte (Match)");
}

void test_eeprom_write_word(void) {
  uint8_t buff[2] = {I2CIP_TEST_EEPROM_BYTE0, I2CIP_TEST_EEPROM_BYTE1};
  I2CIP::i2cip_errorlevel_t result = eeprom.writeRegister((uint16_t)0, buff, 2);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP::I2CIP_ERR_NONE, result, "EEPROM Write Byte");
}

void test_eeprom_read_word(void) {
  uint16_t c = '\0';
  I2CIP::i2cip_errorlevel_t result = eeprom.readRegisterWord((uint16_t)0, c);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP::I2CIP_ERR_NONE, result, "EEPROM Read Byte");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_TEST_EEPROM_BYTE0, (c & 0xFF), "EEPROM Read Byte (Match 1/2)");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_TEST_EEPROM_BYTE1, (c >> 8), "EEPROM Read Byte (Match 2/2)");
}

void test_eeprom_overwrite_contents(void) {
  I2CIP::i2cip_errorlevel_t result = eeprom.overwriteContents(eeprom_contents);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP::I2CIP_ERR_NONE, result, "EEPROM Overwrite Contents");
}

void test_eeprom_read_contents(void) {
  I2CIP::i2cip_errorlevel_t result = eeprom.readContents((uint8_t*)buffer, bufferlen);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP::I2CIP_ERR_NONE, result, "EEPROM Read Contents");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(0, bufferlen, "EEPROM Read Contents (Empty)");
  
  #ifdef I2CIP_TEST_EEPROM_OVERWRITE
    TEST_ASSERT_EQUAL_STRING_MESSAGE(eeprom_contents, (char*)buffer, "EEPROM Read Contents (Match)");
  #endif
}

void setup() {
  delay(2000);

  UNITY_BEGIN();

  RUN_TEST(test_eeprom_ping);
  RUN_TEST(test_eeprom_write_byte);
  RUN_TEST(test_eeprom_read_byte);
  RUN_TEST(test_eeprom_write_word);
  RUN_TEST(test_eeprom_read_word);
  
  #ifdef I2CIP_TEST_EEPROM_OVERWRITE
    RUN_TEST(test_eeprom_overwrite_contents);
  #endif
  
  RUN_TEST(test_eeprom_read_contents);

  UNITY_END();
}

void loop() {

}