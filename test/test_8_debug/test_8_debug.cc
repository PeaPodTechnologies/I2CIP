#ifndef UNIT_TEST
#define UNIT_TEST 1
#endif

#define I2CIP_DEBUG_SERIAL Serial
#define DEBUG_DELAY() delayMicroseconds(1);

#include <Arduino.h>
#include <unity.h>
#include "../config.h"

// #define I2CIP_TEST_SEVENSEG_USE_SNAKE 1 // Uncomment to enable snake pattern for seven-segment display

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
    if(I2CIP::MUX::pingMUX(I2CIP_WIRENUM_PRIMARY, m)) {
      if(I2CIP::modules[m] == nullptr) {
        I2CIP::modules[m] = new TestModule(I2CIP_WIRENUM_PRIMARY, m);
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

void test_nomodule_init(void) {
  // TEST_ASSERT_FALSE_MESSAGE(I2CIP::modules[I2CIP_TEST_MODULE] == nullptr, "Module is not initialized");

  #ifdef I2CIP_TEST_USE_MCP23008LCD
  // errlev_lcd = I2CIP::modules[I2CIP_TEST_MODULE]->operator()<MCP23008>(fqa_lcd, false, _i2cip_args_io_default, DebugJsonOut);
  errlev_lcd = notamodule.operator()<MCP23008>(fqa_lcd, false, _i2cip_args_io_default, DebugJsonOut);
  TEST_ASSERT_EQUAL_INT_MESSAGE(I2CIP_ERR_NONE, errlev_lcd, "MCP23008 LCD initialization failed");
  #endif

  #ifdef I2CIP_TEST_USE_SEVENSEGMENT
  // errlev_sevenseg = I2CIP::modules[I2CIP_TEST_MODULE]->operator()<HT16K33>(fqa_sevenseg, false, _i2cip_args_io_default, DebugJsonOut);
  errlev_sevenseg = notamodule.operator()<HT16K33>(fqa_sevenseg, false, _i2cip_args_io_default, DebugJsonOut);
  TEST_ASSERT_EQUAL_INT_MESSAGE(I2CIP_ERR_NONE, errlev_sevenseg, "Seven-segment initialization failed");
  #endif

  #ifdef I2CIP_TEST_USE_SEESAWROTARY
  // errlev_rotary = I2CIP::modules[I2CIP_TEST_MODULE]->operator()<SEESAW>(fqa_rotary, false, _i2cip_args_io_default, DebugJsonOut);
  errlev_rotary = notamodule.operator()<Seesaw>(fqa_rotary, false, _i2cip_args_io_default, DebugJsonOut);
  TEST_ASSERT_EQUAL_INT_MESSAGE(I2CIP_ERR_NONE, errlev_rotary, "Rotary initialization failed");
  #endif

  TEST_PASS_MESSAGE("Nomodule initialization completed successfully");
}

#ifdef I2CIP_TEST_USE_MCP23008LCD
void test_nomodule_lcd(void) {
  // TEST_ASSERT_FALSE_MESSAGE(I2CIP::modules[I2CIP_TEST_MODULE] == nullptr, "Module is not initialized");
  
  // DeviceGroup* dg_mcp = I2CIP::modules[I2CIP_TEST_MODULE]->operator[]("MCP23008");
  DeviceGroup* dg_mcp = notamodule["MCP23008"];
  TEST_ASSERT_TRUE_MESSAGE(dg_mcp != nullptr, "MCP23008 device group is null");
  TEST_ASSERT_NOT_EQUAL_UINT_MESSAGE(0, dg_mcp->getNumDevices(), "MCP23008 device group has no devices");

  MCP23008* mcp = (MCP23008*)dg_mcp->getDevice(0);
  TEST_ASSERT_TRUE_MESSAGE(mcp != nullptr, "MCP23008 device is null");

  LCD lcd(mcp);
  String msg = "Hello, I2CIP!\n";
  double seconds = millis() / 1000.0;
  msg += String((float)seconds, 3);
  msg += "s\n";
  errlev_lcd = lcd.set(msg, LCD_ARGS_NONE);
  TEST_ASSERT_EQUAL_INT_MESSAGE(I2CIP_ERR_NONE, errlev_lcd, "LCD set command failed");
}
#endif

#ifdef I2CIP_TEST_USE_SEVENSEGMENT
int16_t count = 0;
void test_nomodule_sevenseg(void) {
  // TEST_ASSERT_FALSE_MESSAGE(I2CIP::modules[I2CIP_TEST_MODULE] == nullptr, "Module is not initialized");
  
  i2cip_ht16k33_data_t data_sevenseg = { .h = count };
  
  #ifdef I2CIP_TEST_SEVENSEG_USE_SNAKE
  i2cip_ht16k33_mode_s args_sevenseg = SEG_SNAKE;
  #else
  i2cip_ht16k33_mode_t args_sevenseg = SEG_INT;
  #endif

  i2cip_args_io_t args = {.g = false, .a = nullptr, .s = &data_sevenseg, .b = &args_sevenseg };
  // errlev_sevenseg = I2CIP::modules[I2CIP_TEST_MODULE]->operator()<HT16K33>(fqa_sevenseg, true, args, DebugJsonOut);
  errlev_sevenseg = notamodule.operator()<HT16K33>(fqa_sevenseg, true, args, DebugJsonOut);
  TEST_ASSERT_EQUAL_INT_MESSAGE(I2CIP_ERR_NONE, errlev_sevenseg, "Seven-segment set command failed");

  #ifndef I2CIP_TEST_USE_SEESAWROTARY
    count++;
  #endif
}
#endif

#ifdef I2CIP_TEST_USE_SEESAWROTARY
void test_nomodule_rotary(void) {
  // TEST_ASSERT_FALSE_MESSAGE(I2CIP::modules[I2CIP_TEST_MODULE] == nullptr, "Module is not initialized");

  // I2CIP::DeviceGroup* dg_rotary = modules[I2CIP_TEST_MODULE]->operator[]("SEESAW");
  I2CIP::DeviceGroup* dg_rotary = notamodule["SEESAW"];
  if(dg_rotary != nullptr && dg_rotary->getNumDevices() > 0) {
    RotaryEncoder* rotary = (RotaryEncoder*)(dg_rotary->getDevice(0)); // Use first device
    if(rotary != nullptr) {
      // i2cip_errorlevel_t errlev_rotary = modules[I2CIP_TEST_MODULE]->operator()<Seesaw>(rotary->getFQA(), true, _i2cip_args_io_default, DebugJsonBreakpoints);
      i2cip_errorlevel_t errlev_rotary = notamodule.operator()<Seesaw>(rotary->getFQA(), true, _i2cip_args_io_default, DebugJsonBreakpoints);
      if(errlev_rotary == I2CIP_ERR_NONE) {
        i2cip_rotaryencoder_t cache = rotary->getCache();

        DebugJson::telemetry(rotary->getLastRX(), cache.encoder, "encoder");
        DebugJson::telemetry(rotary->getLastRX(), cache.button, "button");
        
        #ifdef I2CIP_TEST_USE_SEVENSEGMENT
        // Overwrite 7Seg Args
        count = (int16_t)cache.encoder;
        #endif
      }
    }
  }
}
#endif

void loop(void) {

  RUN_TEST(test_modules_unload);

  delay(1000);

  RUN_TEST(test_modules_load);

  delay(1000);

  RUN_TEST(test_json_command);

  delay(1000);

  RUN_TEST(test_nomodule_init);
  delay(1000);

  #ifdef I2CIP_TEST_USE_MCP23008LCD
  RUN_TEST(test_nomodule_lcd);
  delay(1000);
  #endif

  #ifdef I2CIP_TEST_USE_SEESAWROTARY
  RUN_TEST(test_nomodule_rotary);
  delay(1000);
  #endif

  #ifdef I2CIP_TEST_USE_SEVENSEGMENT
  RUN_TEST(test_nomodule_sevenseg);
  delay(1000);
  #endif

  // DebugJson::update(Serial, I2CIP::commandRouter);
}