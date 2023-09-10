#ifndef I2CIP_TESTS_TEST_H_
#define I2CIP_TESTS_TEST_H_

#include <I2CIP.h>

// TESTING PARAMETERS
#define WIRENUM 0x00
#define MODULE  0x00
#define I2CIP_TEST_BUFFERSIZE 100 // Need to limit this, or else crash; I think Unity takes up a lot of stack space

#define I2CIP_TEST_EEPROM_BYTE0  '[' // This should be the first character of ANY valid SPRT EEPROM
#define I2CIP_TEST_EEPROM_BYTE1 '{'
#define I2CIP_TEST_EEPROM_WORD  (uint16_t)(I2CIP_TEST_EEPROM_BYTE << 8 | I2CIP_TEST_EEPROM_BYTE2)

const char* eeprom_contents = "[{\"eeprom\":[80]}]";

// #define I2CIP_TEST_EEPROM_OVERWRITE 1

I2CIP::EEPROM eeprom = I2CIP::EEPROM(WIRENUM, MODULE);
const i2cip_fqa_t& eeprom_fqa = eeprom.getFQA();

I2CIP::Module mymodule = I2CIP::Module()

#endif