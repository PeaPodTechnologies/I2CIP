#ifndef UNIT_TEST

#ifndef I2CIP_MAIN_H_
#define I2CIP_MAIN_H_

#include <Arduino.h>

#include <I2CIP.h>
#include <i2cip/mux.h>
#include <i2cip/eeprom.h>

#define WIRENUM 0
#define MUXNUM 0

// Contents to write to EEPROM
const char* eeprom_contents = "[{\"eeprom\":[80]}]";

#endif

#endif