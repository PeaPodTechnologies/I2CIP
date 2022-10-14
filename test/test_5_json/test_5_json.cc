#include <Arduino.h>
#include <ArduinoJson.h>
#include <unity.h>

#include <i2cip/routingtable.h>
#include <I2CIP.h>
#include "../config.h"

// GLOBALS
char buffer[I2CIP_TEST_BUFFERSIZE] = { '\0' };
StaticJsonDocument<I2CIP_TEST_BUFFERSIZE> eeprom_json;  // EEPROM JSON doc
uint8_t totaldevices = 0; // Number of devices in EEPROM
i2cip_fqa_t* devices; // Array of EEPROM device FQAs
uint8_t d = 0;  // Loop variable

/**
 * Read and deserialize the EEPROM.
 **/
void test_sprt_json_deser(void) {
  // READ EEPROM
  uint16_t size = 0;
  I2CIP::i2cip_errorlevel_t result = I2CIP::Routing::EEPROM::readContents(eeprom_fqa, (uint8_t*)buffer, size, I2CIP_TEST_BUFFERSIZE);
  TEST_ASSERT_EQUAL_UINT8(I2CIP::I2CIP_ERR_NONE, result);
  TEST_ASSERT_NOT_EQUAL(0, size);

  // DESERIALIZE
  DeserializationError jsonerr = deserializeJson(eeprom_json, buffer);

  TEST_ASSERT_TRUE(jsonerr.code() == DeserializationError::Code::Ok);
  TEST_ASSERT_NOT_EQUAL(0, eeprom_json.memoryUsage());
  TEST_ASSERT_FALSE(eeprom_json.overflowed());
}

/**
 * Parse the JSON, count devices and busses.
 **/
void test_sprt_json_valid(void) {
  JsonArray arr = eeprom_json.as<JsonArray>();

  uint8_t buscount = 0;
  for (JsonObject bus : arr) {
    // Count reachable devices in each device group
    uint8_t devicecount = 0;
    for (JsonPair device : bus) {
      buscount++;

      // Device addresses
      JsonArray addresses = device.value().as<JsonArray>();

      for (JsonVariant address : addresses) {
        devicecount++;
      }
    }

    // Add the number of devices on this bus to the tally
    totaldevices += devicecount;
  }

  TEST_ASSERT_NOT_EQUAL(0, buscount);

  TEST_ASSERT_NOT_EQUAL(0, totaldevices);
}

/**
 * Attempt to reach a device.
 **/
void test_device_reachable(void) {
  Serial.println(eeprom_fqa, HEX);
  Serial.println(devices[d], HEX);
  TEST_ASSERT_EQUAL_UINT8(I2CIP::I2CIP_ERR_NONE, I2CIP::Device::ping(devices[d]));
}

void setup() {
  delay(2000);

  UNITY_BEGIN();

  RUN_TEST(test_sprt_json_deser);

  RUN_TEST(test_sprt_json_valid);

  devices = new i2cip_fqa_t[totaldevices];

  JsonArray arr = eeprom_json.as<JsonArray>();

  uint8_t busnum = 0, i = 0;
  for (JsonObject bus : arr) {
    for (JsonPair device : bus) {

      // Device addresses
      JsonArray addresses = device.value().as<JsonArray>();

      for (JsonVariant address : addresses) {
        Serial.println(address.as<uint8_t>(), HEX);
        devices[i] = I2CIP::createFQA(WIRENUM, MUXNUM, busnum, address.as<uint8_t>());
        i++;
      }

      busnum++;
    }
  }
}

void loop() {
  if(d >= totaldevices) {
    UNITY_END();
    delete devices;
  }
  RUN_TEST(test_device_reachable);
  d++;
}