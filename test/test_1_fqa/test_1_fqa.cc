#include <Arduino.h>
#include <unity.h>

#include <i2cip/fqa.h>
#include <I2CIP.h>
#include "../config.h"

#define I2CIP_TEST_FQA_WIRE 0b1010000000000000 // I2C Bus 5
#define I2CIP_TEST_FQA_MUX  0b0000110000000000 // MUX Number 3
#define I2CIP_TEST_FQA_BUS  0b0000001000000000 // Bus 4
#define I2CIP_TEST_FQA_ADDR 0b0000000001000001 // Device Address 0x41 (65)

void test_fqa_create(void) {
  i2cip_fqa_t fqa = I2CIP_TEST_FQA_WIRE
                  | I2CIP_TEST_FQA_MUX
                  | I2CIP_TEST_FQA_BUS
                  | I2CIP_TEST_FQA_ADDR;
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(fqa, I2CIP_FQA_CREATE(5, 3, 4, 65), "FQA Create matches Bit-OR'ed Expected Value");
}

void test_fqa_segments(void) {
  i2cip_fqa_t fqa = I2CIP::createFQA(0x00, 0x00, I2CIP_MUX_BUS_DEFAULT, 0x50);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0x00, I2CIP_FQA_SEG_I2CBUS(fqa), "FQA Seg: Bus Number");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0x00, I2CIP_FQA_SEG_MUXNUM(fqa), "FQA Seg: MUX Number");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(I2CIP_MUX_BUS_DEFAULT, I2CIP_FQA_SEG_MUXBUS(fqa), "FQA Seg: MUX Bus Number");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0x50, I2CIP_FQA_SEG_DEVADR(fqa), "FQA Seg: Device Address");
}

void setup() {
  delay(2000);

  UNITY_BEGIN();

  RUN_TEST(test_fqa_create);
  RUN_TEST(test_fqa_segments);

  UNITY_END();
}

void loop() {

}