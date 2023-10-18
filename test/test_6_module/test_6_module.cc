#include <Arduino.h>
#include <unity.h>

#include "../config.h"

#include <I2CIP.h>

using namespace I2CIP;

class BasicModule : public Module {
  public:
    BasicModule(const uint8_t& wire, const uint8_t& module) : Module(wire, module) { }
    
    bool parseEEPROMContents(const uint8_t* buffer, size_t buflen) override {
      #ifdef I2CIP_DEBUG_SERIAL
        DEBUG_DELAY();
        I2CIP_DEBUG_SERIAL.print(F("No Parsing Yet!\n"));
        DEBUG_DELAY();
      #endif
      return true;
    }
};

BasicModule* m;  // to be initialized in setup()

void setup(void) {
  Serial.begin(115200);

  #ifdef DEBUG_SERIAL
  DEBUG_SERIAL.println("\nInitializing BasicModule...");
  #endif

  // Initialize module
  m = new BasicModule(0, 0);

  // if((EEPROM*)(m) == nullptr) while(true);

  i2cip_fqa_t fqa = ((const EEPROM&)(*m)).getFQA();

  #ifdef DEBUG_SERIAL
    DEBUG_SERIAL.print(F("BasicModule Initialized! EEPROM FQA "));
    DEBUG_SERIAL.print(I2CIP_FQA_SEG_I2CBUS(fqa), HEX);
    DEBUG_SERIAL.print(':');
    DEBUG_SERIAL.print(I2CIP_FQA_SEG_MODULE(fqa), HEX);
    DEBUG_SERIAL.print(':');
    DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(fqa), HEX);
    DEBUG_SERIAL.print(':');
    DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
    DEBUG_SERIAL.print(" ID '");
    DEBUG_SERIAL.print(((const EEPROM&)(*m)).getID());
    DEBUG_SERIAL.print("' @0x");
    DEBUG_SERIAL.print((uint16_t)&((const EEPROM&)(*m)).getID()[0], HEX);  
    DEBUG_SERIAL.print('\n');
  #endif

  // Build module
  if(!Module::build(*m)) while(true) { // Blink
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }

  delay(2000);

  UNITY_BEGIN();
}

bool fail = false;

void test_module_self_check(void) {
  i2cip_errorlevel_t errlev = (*m)();
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_ERR_NONE, errlev, "Self-check failed! Check module wiring.");
  if(errlev > I2CIP_ERR_NONE) fail = true;
}

void test_module_eeprom_check(void) {
  i2cip_errorlevel_t errlev = (*m)(((const EEPROM&)(*m)).getFQA());
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_ERR_NONE, errlev, "EEPROM check failed! Check EEPROM wiring.");
  if(errlev > I2CIP_ERR_NONE) fail = true;
}

void test_module_eeprom_update(void) {
  i2cip_errorlevel_t errlev = (*m)(((const EEPROM&)(*m)).getFQA(), true);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(I2CIP_ERR_NONE, errlev, "EEPROM check failed! Check EEPROM wiring.");
  if(errlev > I2CIP_ERR_NONE) fail = true;
}

i2cip_errorlevel_t errlev;
uint8_t count = 1;
void loop(void) {

  RUN_TEST(test_module_self_check);

  if(fail) {
    UNITY_END();
    while(true);
  }

  delay(1000);

  RUN_TEST(test_module_eeprom_check);

  if(fail) {
    UNITY_END();
    while(true);
  }
  
  #if I2CIP_TEST_EEPROM_OVERWRITE == 1
  delay(1000);

  RUN_TEST(test_module_eeprom_update);

  if(fail) {
    UNITY_END();
    while(true);
  }
  #endif

  if(count > 3) {
    UNITY_END();
    while(true);
  }

  delay(1000);
  count++;
}
