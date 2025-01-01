#include <Arduino.h>
#include <unity.h>

#include "../config.h"
#include <debug.h>

#include <eeprom.h>

using namespace I2CIP;

EEPROM* eeprom = nullptr;
const i2cip_fqa_t& eeprom_fqa = I2CIP::createFQA(WIRENUM, MODULE, I2CIP_MUX_BUS_DEFAULT, I2CIP_EEPROM_ADDR);

// Explain FQA in detail:
/**
 * FQA (Fully Qualified Address) is a 16-bit unsigned integer that contains the following information:
 * - 3-bit I2C Bus Number
 * - 3-bit Module Number
 * - 4-bit MUX Bus Number
 * - 8-bit Device Address
*/

char buffer[I2CIP_TEST_BUFFERSIZE] = { '\0' };
size_t bufferlen = 0;

void test_device_oop(void) {
  // Initialize EEPROM - Done after Serial.begin for debug output to work
  eeprom = (EEPROM*)EEPROM::factory(eeprom_fqa);

  TEST_ASSERT_TRUE_MESSAGE(eeprom != nullptr, "EEPROM Object Instantiation");

  TEST_ASSERT_EQUAL_PTR_MESSAGE((InputGetter*)((InputInterface<char*, uint16_t>*)(eeprom)), eeprom->getInput(), "EEPROM Input Getter Self-Link (Makes High-Level GET Available VIA Device Class)");
  TEST_ASSERT_EQUAL_PTR_MESSAGE((OutputSetter*)((OutputInterface<const char*, uint16_t>*)(eeprom)), eeprom->getOutput(), "EEPROM Output Setter Self-Link (Makes High-Level SET Available VIA Device Class)");
}

void test_eeprom_ping(void) {
  char msg[30];
  sprintf(msg, "Device unreachable (%01X.%01X.%01X.%02X)", I2CIP_FQA_SEG_I2CBUS(eeprom_fqa), I2CIP_FQA_SEG_MODULE(eeprom_fqa), I2CIP_FQA_SEG_MUXBUS(eeprom_fqa), I2CIP_FQA_SEG_DEVADR(eeprom_fqa));
  i2cip_errorlevel_t result = eeprom->ping();
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_ERR_NONE, result, msg);
}

void test_eeprom_write_byte(void) {
  i2cip_errorlevel_t result = eeprom->writeRegister((uint16_t)0, I2CIP_TEST_EEPROM_BYTE0);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_ERR_NONE, result, "EEPROM Write Byte");
}

void test_eeprom_read_byte(void) {
  uint8_t c = '\0';
  i2cip_errorlevel_t result = eeprom->readRegisterByte((uint16_t)0, c);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_ERR_NONE, result, "EEPROM Read Byte");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_TEST_EEPROM_BYTE0, c, "EEPROM Read Byte (Match)");
}

void test_eeprom_write_word(void) {
  uint8_t buff[2] = {I2CIP_TEST_EEPROM_BYTE0, I2CIP_TEST_EEPROM_BYTE1};
  i2cip_errorlevel_t result = eeprom->writeRegister((uint16_t)0, buff, 2);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_ERR_NONE, result, "EEPROM Write Byte");
}

void test_eeprom_read_word(void) {
  uint16_t c = '\0';
  i2cip_errorlevel_t result = eeprom->readRegisterWord((uint16_t)0, c);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_ERR_NONE, result, "EEPROM Read Byte");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_TEST_EEPROM_BYTE0, (c & 0xFF), "EEPROM Read Byte (Match 1/2)");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_TEST_EEPROM_BYTE1, (c >> 8), "EEPROM Read Byte (Match 2/2)");
}

void test_device_io(void) {
  i2cip_errorlevel_t result = I2CIP_ERR_NONE;
  #ifdef I2CIP_TEST_EEPROM_OVERWRITE
    #ifdef EEPROM_JSON_CONTENTS_TEST
      const char* msg = EEPROM_JSON_CONTENTS_TEST;
      size_t len = strlen(msg);
      result = eeprom->getOutput()->set(&msg, &len);
    #else
      result = eeprom->getOutput()->set();
    #endif
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_ERR_NONE, result, "EEPROM Output Setter (Default Value, Args)");
  #endif
  result = ((Device*)eeprom)->get(nullptr);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_ERR_NONE, result, "EEPROM Input Getter (Default Args)");
  #ifdef I2CIP_TEST_EEPROM_OVERWRITE
    const char* cache = eeprom->getCache();
    #ifdef EEPROM_JSON_CONTENTS_TEST
      TEST_ASSERT_EQUAL_STRING_MESSAGE(msg, cache, "EEPROM Cache (Match)");
    #else
      TEST_ASSERT_EQUAL_STRING_MESSAGE(I2CIP_EEPROM_DEFAULT, cache, "EEPROM Cache (Match)");
    #endif
  #endif
}

void test_device_delete(void) {
  delete eeprom;
  TEST_ASSERT_TRUE_MESSAGE(true, "EEPROM Deletion Fail");
}

// TODO: Test Device IO Repeatability in loop()
// (i.e. multiple calls to get() and set() without intermediate calls to resetCache())

// TODO: Test Device Deletion (i.e. delete module; delete sht31;) stability

void setup() {
  Serial.begin(115200);

  delay(2000);

  UNITY_BEGIN();

  delay(1000);
  RUN_TEST(test_device_oop);
  delay(1000);
  RUN_TEST(test_eeprom_ping);
  delay(1000);
  RUN_TEST(test_eeprom_write_byte);
  delay(1000);
  RUN_TEST(test_eeprom_read_byte);
  delay(1000);
  RUN_TEST(test_eeprom_write_word);
  delay(1000);
  RUN_TEST(test_eeprom_read_word);
  delay(1000);
  RUN_TEST(test_device_io);
  delay(1000);

  RUN_TEST(test_device_delete);

  UNITY_END();
}

void loop() {

}