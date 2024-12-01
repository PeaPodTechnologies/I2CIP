#include <Arduino.h>
#include <unity.h>

#include <mux.h>
#include "../config.h"

#define I2CIP_TEST_MUXINSTR 0b00010000 // Bus 4
#define I2CIP_TEST_MUXADDR  0x72       // Mux 2

const i2cip_fqa_t& eeprom_fqa = I2CIP::createFQA(WIRENUM, MODULE, I2CIP_MUX_BUS_DEFAULT, I2CIP_EEPROM_ADDR);

using namespace I2CIP;

void test_mux_bus_to_instr(void) {
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_TEST_MUXINSTR, I2CIP_MUX_BUS_TO_INSTR(4), "MUX Bus Number to Instruction");
}

void test_mux_num_to_addr(void) {
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_TEST_MUXADDR, I2CIP_MODULE_TO_MUXADDR(2), "MUX Number to Address");
}

void test_mux_ping(void) {
  char msg[50];
  unsigned long now = millis();
  bool r = I2CIP::MUX::pingMUX(eeprom_fqa);
  unsigned long delta = r ? millis() - now : 0;

  sprintf(msg, "MUX %01X:%01X:.:. - PING: FAIL", I2CIP_FQA_SEG_I2CBUS(eeprom_fqa), I2CIP_FQA_SEG_MODULE(eeprom_fqa));
  TEST_ASSERT_TRUE_MESSAGE(r, msg);

  if(r){
    sprintf(msg, "MUX %01X:%01X:.:. - PING: %.3fs", I2CIP_FQA_SEG_I2CBUS(eeprom_fqa), I2CIP_FQA_SEG_MODULE(eeprom_fqa), (delta / 1000.0));
    TEST_PASS_MESSAGE(msg);
  }
}

void test_mux_bus_set(void) {
  char msg[50];
  sprintf(msg, "MUX %01X:%01X:.:. - SET BUS %01X: FAIL", I2CIP_FQA_SEG_I2CBUS(eeprom_fqa), I2CIP_FQA_SEG_MODULE(eeprom_fqa), I2CIP_FQA_SEG_MUXBUS(eeprom_fqa));

  unsigned long now = millis();
  I2CIP::i2cip_errorlevel_t result = I2CIP::MUX::setBus(eeprom_fqa);
  unsigned long delta = millis() - now;
  
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP::I2CIP_ERR_NONE, result, msg);

  if(result == I2CIP::I2CIP_ERR_NONE) {
    sprintf(msg, "MUX %01X:%01X:.:. - SET BUS %01X: %.3fs", I2CIP_FQA_SEG_I2CBUS(eeprom_fqa), I2CIP_FQA_SEG_MODULE(eeprom_fqa), I2CIP_FQA_SEG_MUXBUS(eeprom_fqa), (delta / 1000.0));
    TEST_PASS_MESSAGE(msg);
  }
}

void test_mux_bus_reset(void) {
  char msg[50];
  sprintf(msg, "MUX %01X:%01X:.:. - RESET BUS: FAIL", I2CIP_FQA_SEG_I2CBUS(eeprom_fqa), I2CIP_FQA_SEG_MODULE(eeprom_fqa));

  unsigned long now = millis();
  I2CIP::i2cip_errorlevel_t result = I2CIP::MUX::resetBus(eeprom_fqa);
  unsigned long delta = millis() - now;
  
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP::I2CIP_ERR_NONE, result, msg);

  if(result == I2CIP::I2CIP_ERR_NONE) {
    sprintf(msg, "MUX %01X:%01X:.:. - RESET BUS: %.3fs", I2CIP_FQA_SEG_I2CBUS(eeprom_fqa), I2CIP_FQA_SEG_MODULE(eeprom_fqa), (delta / 1000.0));
    TEST_PASS_MESSAGE(msg);
  }
}

void setup() {
  delay(2000);

  UNITY_BEGIN();

  RUN_TEST(test_mux_bus_to_instr);

  delay(1000);

  RUN_TEST(test_mux_num_to_addr);

  delay(1000);

  RUN_TEST(test_mux_ping);

  delay(1000);

  RUN_TEST(test_mux_bus_set);

  delay(1000);

  RUN_TEST(test_mux_bus_reset);

  UNITY_END();
}

void loop() {

}