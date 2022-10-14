#include <Arduino.h>
#include <unity.h>

#include <i2cip/mux.h>
#include <I2CIP.h>
#include "../config.h"

#define I2CIP_TEST_MUXINSTR 0b00010000 // Bus 4
#define I2CIP_TEST_MUXADDR  0x72       // Mux 2

// ALL MACROS

void test_mux_bus_to_instr(void) {
  TEST_ASSERT_EQUAL_UINT8(I2CIP_TEST_MUXINSTR, I2CIP_MUX_BUS_TO_INSTR(4));
}

void test_mux_num_to_addr(void) {
  TEST_ASSERT_EQUAL_UINT8(I2CIP_TEST_MUXADDR, I2CIP_MUX_NUM_TO_ADDR(2));
}

void test_mux_ping(void) {
  TEST_ASSERT_TRUE(I2CIP::MUX::pingMUX(eeprom_fqa));
}

void test_mux_bus_set(void) {
  I2CIP::i2cip_errorlevel_t result = I2CIP::MUX::setBus(eeprom_fqa);
  TEST_ASSERT_EQUAL_UINT8(I2CIP::I2CIP_ERR_NONE, result);
}

void test_mux_bus_reset(void) {
  I2CIP::i2cip_errorlevel_t result = I2CIP::MUX::resetBus(eeprom_fqa);
  TEST_ASSERT_EQUAL_UINT8(I2CIP::I2CIP_ERR_NONE, result);
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