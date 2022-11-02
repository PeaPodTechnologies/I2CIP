#ifndef UNIT_TEST

#include <string.h>

#include <Arduino.h>

#include <I2CIP.h>
#include <i2cip/mux.h>
#include <i2cip/eeprom.h>

using namespace I2CIP;

#define WIRENUM 0
#define MUXNUM 0
const char* eeprom_contents = "[{\"eeprom\":[80]}]";

i2cip_fqa_t fqa = createFQA(WIRENUM, MUXNUM, I2CIP_MUX_BUS_DEFAULT, I2CIP_EEPROM_ADDR);

void setup(void) {
  Serial.begin(115200);
  
  if (Device::ping(fqa) > I2CIP_ERR_NONE) {
    Serial.println("EEPROM not found! Check wiring. Freezing...");
    while (1) delay(10);
  }

  Serial.println("EEPROM Found!");

  // CLEAR
  Serial.println("Clearing EEPROM...");
  if(I2CIP::EEPROM::clearContents(fqa) > I2CIP_ERR_NONE) {
    Serial.println("EEPROM clear failed! Check wiring. Freezing...");
    while (1) delay(10);
  }
  Serial.println("EEPROM Cleared!");

  // WRITE
  Serial.println("Writing new contents to EEPROM...");
  if(I2CIP::EEPROM::overwriteContents(fqa, eeprom_contents, false)) {
    Serial.println("EEPROM write failed! Check wiring. Freezing...");
    while (1) delay(10);
  }
  Serial.println("Contents written!");

  Serial.println("Verifying contents...");
  char buffer[I2CIP_EEPROM_SIZE];
  uint16_t buflen = 0;
  I2CIP::EEPROM::readContents(fqa, (uint8_t*)buffer, buflen);
  for(int i = 0; i < buflen; i++) {
    Serial.print(buffer[i]);
  }
  Serial.println();
  if(strcmp(buffer, eeprom_contents) != 0) {
    Serial.println("EEPROM verification failed! Check wiring. Freezing...");
    while (1) delay(10);
  }
  Serial.println("Contents verified!");
}

void loop(void) {

}

#endif