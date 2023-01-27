#ifndef UNIT_TEST

#include <main.h>

#include <string.h>

#include <Arduino.h>

#include <I2CIP.h>
#include <i2cip/mux.h>
#include <i2cip/eeprom.h>

using namespace I2CIP;

i2cip_fqa_t fqa = createFQA(WIRENUM, MUXNUM, I2CIP_MUX_BUS_DEFAULT, I2CIP_EEPROM_ADDR);

void setup(void) {
  Serial.begin(115200);
  
  if (Device::ping(fqa) > I2CIP_ERR_NONE) {
    Serial.println("EEPROM not found! Check wiring. Freezing...");
    while (1) delay(10);
  }

  Serial.println("EEPROM Found!");

  // DUMP
  Serial.println("Dumping EEPROM...");
  
  char buffer[I2CIP_EEPROM_SIZE] = { '\0' };
  uint16_t buflen = 0;
  I2CIP::EEPROM::readContents(fqa, (uint8_t*)buffer, buflen, 100, false);
  
  Serial.print("EEPROM Dump (");
  Serial.print(buflen);
  Serial.println(" bytes):");
  for(uint8_t i = 0; i < buflen; i++) {
    Serial.print(buffer[i]);
  }
  Serial.println();

  // CLEAR
  Serial.println("Clearing EEPROM...");
  if(I2CIP::EEPROM::clearContents(fqa) > I2CIP_ERR_NONE) {
    Serial.println("EEPROM clear failed! Check wiring. Freezing...");
    while (1) delay(10);
  }
  Serial.println("EEPROM Cleared!");

  buflen = 0;
  I2CIP::EEPROM::readContents(fqa, (uint8_t*)buffer, buflen, 100, false);
  
  Serial.print("EEPROM Dump (");
  Serial.print(buflen);
  Serial.println(" bytes):");
  for(uint8_t i = 0; i < buflen; i++) {
    Serial.print(buffer[i]);
  }
  Serial.println();

  // WRITE
  Serial.println("Writing new contents to EEPROM...");
  if(I2CIP::EEPROM::overwriteContents(fqa, eeprom_contents, false)) {
    Serial.println("EEPROM write failed! Check wiring. Freezing...");
    while (1) delay(10);
  }
  Serial.println("Contents written!");

  Serial.println("Verifying contents...");
  buflen = 0;
  I2CIP::EEPROM::readContents(fqa, (uint8_t*)buffer, buflen, 100, false);
  Serial.print("EEPROM Dump (");
  Serial.print(buflen);
  Serial.println(" bytes):");
  for(uint8_t i = 0; i < buflen; i++) {
    Serial.print(buffer[i]);
  }
  Serial.println();

  if(strcmp(buffer, eeprom_contents) != 0) {
    Serial.println("EEPROM verification failed!");
  } else {
    Serial.println("Contents verified!");
  }
}

void loop(void) {



}

#endif