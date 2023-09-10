#include <Arduino.h>
#include <unity.h>

#include <mux.h>
#include "../config.h"

#define I2CIP_TEST_MUXINSTR 0b00010000 // Bus 4
#define I2CIP_TEST_MUXADDR  0x72       // Mux 2

using namespace I2CIP;

void test_mux_bus_to_instr(void) {
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_TEST_MUXINSTR, I2CIP_MUX_BUS_TO_INSTR(4), "MUX Bus Number to Instruction");
}

void test_mux_num_to_addr(void) {
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_TEST_MUXADDR, I2CIP_MODULE_TO_MUXADDR(2), "MUX Number to Address");
}

void test_mux_ping(void) {
  char msg[27];
  sprintf(msg, "MUX unreachable (%01X.%01X.X.XX)", I2CIP_FQA_SEG_I2CBUS(eeprom_fqa), I2CIP_FQA_SEG_MODULE(eeprom_fqa));
  TEST_ASSERT_TRUE_MESSAGE(I2CIP::MUX::pingMUX(eeprom_fqa), msg);
}

void test_mux_bus_set(void) {
  char msg[33];
  sprintf(msg, "Failed to set MUX bus (%01X.%01X.%01X.XX)", I2CIP_FQA_SEG_I2CBUS(eeprom_fqa), I2CIP_FQA_SEG_MODULE(eeprom_fqa), I2CIP_FQA_SEG_MUXBUS(eeprom_fqa));
  I2CIP::i2cip_errorlevel_t result = I2CIP::MUX::setBus(eeprom_fqa);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP::I2CIP_ERR_NONE, result, msg);
}

void test_mux_bus_reset(void) {
  char msg[35];
  sprintf(msg, "Failed to reset MUX bus (%01X.%01X.X.XX)", I2CIP_FQA_SEG_I2CBUS(eeprom_fqa), I2CIP_FQA_SEG_MODULE(eeprom_fqa));
  I2CIP::i2cip_errorlevel_t result = I2CIP::MUX::resetBus(eeprom_fqa);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP::I2CIP_ERR_NONE, result, msg);
}

void setup() {
  delay(2000);

  UNITY_BEGIN();

  RUN_TEST(test_mux_bus_to_instr);
  RUN_TEST(test_mux_num_to_addr);
  RUN_TEST(test_mux_ping);
  RUN_TEST(test_mux_bus_set);
  RUN_TEST(test_mux_bus_reset);

  UNITY_END();
}

void loop() {

}