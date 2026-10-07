#ifndef UNIT_TEST
#define UNIT_TEST 1
#endif

#include <Arduino.h>
#include <unity.h>
#include "../config.h"

JsonDocument command = JsonDocument();
DebugJson::StringWriter outString = DebugJson::StringWriter();

void test_json_deserialize(void) {
  const char* commandString = I2CIP_TEST_COMMAND_STRING;

  DeserializationError error = deserializeJson(command, commandString);

  TEST_ASSERT_FALSE_MESSAGE((bool)error, (String("JSON command deserialization failed: ") + commandString).c_str());
}


void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  while(!Serial) { digitalWrite(LED_BUILTIN, HIGH); delay(100); digitalWrite(LED_BUILTIN, LOW); delay(100); }

  delay(2000);

  UNITY_BEGIN();

  delay(1000);

  RUN_TEST(test_json_deserialize);
}

/**
 * Module Detection
 * 
 * For each MUX address:
 * a. Ping MUX (if not found, fail errlev)
 * b. If not loaded, instantiate
 * c.
 */
void test_modules_load(void) {
  for(uint8_t m = 0; m < I2CIP_MUX_COUNT; m++) {
    if(I2CIP::MUX::pingMUX(WIRENUM, m)) {
      if(I2CIP::modules[m] == nullptr) {
        I2CIP::modules[m] = new TestModule(WIRENUM, m);
      }

      I2CIP::errlev[m] = I2CIP::modules[m]->operator()();
      // if(I2CIP::errlev[m] == I2CIP_ERR_NONE) {
      //   DebugJson::revision(m, Serial); // sends revision
      // }
    } else {
      I2CIP::errlev[m] = I2CIP_ERR_HARD;
    }
    String msg = "Module " + String(m) + ": " + (I2CIP::modules[m] == nullptr ? "Null" : "0x" + String(I2CIP::errlev[m], HEX));
    TEST_PASS_MESSAGE(msg.c_str());
  }
}

void test_modules_unload(void) {
  for(uint8_t m = 0; m < I2CIP_MUX_COUNT; m++) {
    if(I2CIP::modules[m] != nullptr && I2CIP::errlev[m] == I2CIP_ERR_HARD) {
      delete I2CIP::modules[m];
      I2CIP::modules[m] = nullptr;
    }
  }
}

void test_json_command(void) {
  I2CIP::commandRouter(command.as<JsonObject>(), outString);
  String str = outString.operator String();
  outString.flush();
  TEST_ASSERT_TRUE_MESSAGE(str.length() > 0, "JSON command output is empty");
  
  JsonDocument output = JsonDocument();
  DeserializationError error = deserializeJson(output, str);
  TEST_ASSERT_FALSE_MESSAGE((bool)error, ("JSON command output deserialization failed: " + str).c_str());

  TEST_ASSERT_TRUE_MESSAGE(output.is<JsonObject>(), "JSON command output is not an object");

  TEST_ASSERT_TRUE_MESSAGE(output["type"].is<const char*>(), "JSON command output does not contain 'type' key");
  TEST_ASSERT_EQUAL_STRING_MESSAGE("info", output["type"].as<const char*>(), "JSON command output 'type' is not 'info'");

  TEST_ASSERT_TRUE_MESSAGE(output["timestamp"].is<unsigned long>(), "JSON command output 'timestamp' is not an unsigned long");

  TEST_ASSERT_TRUE_MESSAGE(output["id"].is<const char*>(), "JSON command output 'id' is not a const char*");
  TEST_ASSERT_EQUAL_STRING_MESSAGE(I2CIP_TEST_COMMAND_OUTPUT_ID, output["id"].as<const char*>(), "JSON command output 'id' is not '" I2CIP_TEST_COMMAND_OUTPUT_ID "'");

  TEST_ASSERT_TRUE_MESSAGE(output["fqa"].is<uint16_t>(), "JSON command output 'fqa' is not a uint16_t");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(I2CIP_TEST_FQA, output["fqa"].as<uint16_t>(), "JSON command output 'fqa' is not '" STR(I2CIP_TEST_FQA) "'");

  TEST_ASSERT_TRUE_MESSAGE(output["errlev"].is<int>(), "JSON command output 'errlev' is not an int");
  TEST_ASSERT_EQUAL_INT_MESSAGE(I2CIP_ERR_NONE, output["errlev"].as<int>(), "JSON command output 'errlev' is not 0");

  TEST_PASS_MESSAGE(("JSON command output generated: " + str).c_str());
}

void loop(void) {

  RUN_TEST(test_modules_unload);

  delay(1000);

  RUN_TEST(test_modules_load);

  delay(1000);

  RUN_TEST(test_json_command);

  delay(1000);

  // DebugJson::update(Serial, I2CIP::commandRouter);
}