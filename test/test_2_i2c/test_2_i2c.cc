#include <Arduino.h>
#include <unity.h>

#include <i2cip/fqa.h>
#include <I2CIP.h>

// FQA to Wire array translation
void test_fqa_to_wire(void) {
  TEST_ASSERT_EQUAL_PTR_MESSAGE(&Wire, wires[0], "wires[0] points to &Wire");
}

void setup() {
  delay(2000);

  UNITY_BEGIN();

  RUN_TEST(test_fqa_to_wire);

  UNITY_END();
}

void loop() {

}