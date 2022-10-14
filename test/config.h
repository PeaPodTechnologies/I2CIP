#ifndef I2CIP_TESTS_TEST_H_
#define I2CIP_TESTS_TEST_H_

#include <i2cip/eeprom.h>
#include <i2cip/mux.h>
#include <I2CIP.h>

// TESTING PARAMETERS
#define WIRENUM 0x00
#define MUXNUM  0x00
#define I2CIP_TEST_BUFFERSIZE 100 // Need to limit this, or else crash; I think Unity takes up a lot of stack space

static i2cip_fqa_t eeprom_fqa = I2CIP::createFQA(WIRENUM, MUXNUM, I2CIP_MUX_BUS_DEFAULT, I2CIP_EEPROM_ADDR);

#endif