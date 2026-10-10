#ifndef UNIT_TEST
#define UNIT_TEST 1
#endif

#define I2CIP_DEBUG_SERIAL Serial
#define DEBUG_DELAY() delayMicroseconds(1);

#include <Arduino.h>
#include <unity.h>
#include "../config.h"

#define I2CIP_TEST_SEVENSEG_USE_SNAKE 1 // Uncomment to use snake mode for seven-segment display

void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  while(!Serial) { digitalWrite(LED_BUILTIN, HIGH); delay(100); digitalWrite(LED_BUILTIN, LOW); delay(100); }

  delay(2000);

  UNITY_BEGIN();

  delay(1000);
}

void test_nomodule_init(void) {
  I2CIP::modules[0] = new TestModule(WIRENUM, I2CIP_MUX_NUM_FAKE);

  #ifdef I2CIP_TEST_USE_SEVENSEGMENT
  errlev_sevenseg = I2CIP::modules[0]->operator()<HT16K33>(fqa_sevenseg, false, _i2cip_args_io_default, DebugJsonOut);
  TEST_ASSERT_EQUAL_INT_MESSAGE(I2CIP_ERR_NONE, errlev_sevenseg, "Seven-segment initialization failed");
  #endif

  TEST_PASS_MESSAGE("Nomodule initialization completed successfully");
}

#ifdef I2CIP_TEST_USE_SEVENSEGMENT
unsigned count = 0;
void test_nomodule_sevenseg(void) {
  i2cip_ht16k33_data_t data_sevenseg = { .h = count };
  #ifdef I2CIP_TEST_SEVENSEG_USE_SNAKE
  i2cip_ht16k33_mode_t args_sevenseg = SEG_SNAKE;
  #else
  i2cip_ht16k33_mode_t args_sevenseg = SEG_UINT;
  #endif
  i2cip_args_io_t args = {.g = false, .a = nullptr, .s = &data_sevenseg, .b = &args_sevenseg };
  errlev_sevenseg = I2CIP::modules[0]->operator()<HT16K33>(fqa_sevenseg, true, args, DebugJsonOut);
  TEST_ASSERT_EQUAL_INT_MESSAGE(I2CIP_ERR_NONE, errlev_sevenseg, "Seven-segment set command failed");
  count++;
}
#endif

void loop(void) {

  RUN_TEST(test_nomodule_init);
  delay(1000);

  #ifdef I2CIP_TEST_USE_SEVENSEGMENT
  RUN_TEST(test_nomodule_sevenseg);
  delay(1000);
  #endif

  // DebugJson::update(Serial, I2CIP::commandRouter);
}