#include "I2CIP.hpp"

#include <ArduinoJson.h>

using namespace I2CIP;

BST<i2cip_fqa_t, Device*> I2CIP::devicetree = BST<i2cip_fqa_t, Device*>();
HashTable<DeviceGroup&> I2CIP::devicegroups = HashTable<DeviceGroup&>();

bool JsonModule::parseEEPROMContents(const char* buffer) {  
  // 1. EEPROM -> JSON Deserialization
  // TODO: Buflen + 1 ?
  JsonDocument eeprom_json;
  DeserializationError jsonerr = deserializeJson(eeprom_json, buffer, strlen(buffer));

  if(jsonerr != DeserializationError::Ok) {
    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print("-> Deserialize EEPROM JSON Error (0x");
      I2CIP_DEBUG_SERIAL.print(jsonerr.code(), HEX);
      I2CIP_DEBUG_SERIAL.print("): ");
      I2CIP_DEBUG_SERIAL.println(jsonerr.c_str());
      DEBUG_DELAY();
    #endif
    return false;
  }
  bool r = validateEEPROM(eeprom_json);

  if(!r) {
    return false;
  }

  // JsonArray busses = this->eeprom_json.as<JsonArray>();
  JsonArray busses = eeprom_json.as<JsonArray>();

  int busnum = -1;
  for (JsonVariant bus : busses) {
    busnum++;

    JsonObject root = bus.as<JsonObject>();
    for (JsonPair kv : root) {
      // 2c. Array of I2C Addresses
      if(!kv.value().is<JsonArray>() || kv.value().isNull()) { continue; }
      const char* key = kv.key().c_str();

      // Get DeviceGroup
      DeviceGroup* dg = this->operator[](key);
      if(dg == nullptr) { continue; }

      uint8_t numfqas = kv.value().size();
      if(numfqas == 0) { continue; }
      i2cip_fqa_t fqas[numfqas];

      uint8_t i = 0;
      for (JsonVariant addr : kv.value().as<JsonArray>()) {
        if(!addr.is<unsigned int>()) { continue; }
        uint8_t address = addr.as<uint8_t>();
        fqas[i++] = createFQA(this->getWireNum(), this->getModuleNum(), (uint8_t)busnum, address);
        if(i >= numfqas) { break; }
      }

      for (i = 0; i < numfqas; i++) {
        #ifdef I2CIP_DEBUG_SERIAL
          DEBUG_DELAY();
          I2CIP_DEBUG_SERIAL.print(_F("-> Adding Device "));
          I2CIP_DEBUG_SERIAL.print(i+1);
          I2CIP_DEBUG_SERIAL.print(" / ");
          I2CIP_DEBUG_SERIAL.print(numfqas);
          I2CIP_DEBUG_SERIAL.print(_F(" (Factory @0x"));
          I2CIP_DEBUG_SERIAL.print((uintptr_t)dg->factory, HEX);
          I2CIP_DEBUG_SERIAL.print(")\n");
          DEBUG_DELAY();
        #endif

        // Invoke DeviceGroup Call Operator - Returns Matching, or Calls Factory and Adds
        i2cip_fqa_t fqa = fqas[i];
        if(fqa == this->eeprom->getFQA()) {
          #ifdef I2CIP_DEBUG_SERIAL
            DEBUG_DELAY();
            I2CIP_DEBUG_SERIAL.print(_F("-> Module EEPROM FQA Match! (Skipping)\n"));
            DEBUG_DELAY();
          #endif
          continue;
        }
        // Going to leave this commented, since we switched to global BST/HashTable
        // if(I2CIP_FQA_SEG_MUXBUS(fqa) == I2CIP_MUX_BUS_FAKE) {
        //   #ifdef I2CIP_DEBUG_SERIAL
        //     DEBUG_DELAY();
        //     I2CIP_DEBUG_SERIAL.print(_F("-> Invalid Fake Bus! (Skipping)\n"));
        //     DEBUG_DELAY();
        //   #endif
        //   continue;
        // }

        Device* d = (*dg)(fqa); 
        if(d == nullptr) { 
          #ifdef I2CIP_DEBUG_SERIAL
            DEBUG_DELAY();
            I2CIP_DEBUG_SERIAL.print(_F("-> Factory Failed! (Skipping)\n"));
            DEBUG_DELAY();
          #endif
          break;
        } else {
          #ifdef I2CIP_DEBUG_SERIAL
            DEBUG_DELAY();
            I2CIP_DEBUG_SERIAL.print(_F("-> Factory Success! (Adding)\n"));
            DEBUG_DELAY();
          #endif
          bool r = this->add(*d);
          if(!r) {
            #ifdef I2CIP_DEBUG_SERIAL
              DEBUG_DELAY();
              I2CIP_DEBUG_SERIAL.print(_F("-> Couldn't Add Device!\n"));
              DEBUG_DELAY();
            #endif
            return false;
          }
        }
      }

      #ifdef I2CIP_DEBUG_SERIAL
        DEBUG_DELAY();
        I2CIP_DEBUG_SERIAL.print(_F("-> Group Complete!\n"));
        DEBUG_DELAY();
      #endif
    }
  }
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(_F("-> EEPROM Parsed Successfully!\n"));
    DEBUG_DELAY();
  #endif
  return true;
}

bool JsonModule::validateEEPROM(JsonDocument& eeprom_json) {
  // DeserializationError jsonerr = deserializeJson(this->eeprom_json, buffer, buflen);
  unsigned long now = millis();

  // 2. Schema Validation and Loading
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(_F("-> Verifying JSON: "));
  #endif

  // 2a. Base Array of Busses
  // if(!this->eeprom_json.is<JsonArray>() || !this->eeprom_json[0].is<JsonObject>()) {
  if(!eeprom_json.is<JsonArray>()) {
    #ifdef I2CIP_DEBUG_SERIAL
      I2CIP_DEBUG_SERIAL.println(_F("Invalid Table Structure"));
      DEBUG_DELAY();
    #endif
    eeprom_json.clear();
    return false;
  }

  // JsonArray busses = this->eeprom_json.as<JsonArray>();
  JsonArray busses = eeprom_json.as<JsonArray>();

  int busnum = -1;
  for (JsonVariant bus : busses) {
    busnum++;

    if(busnum == I2CIP_MUX_BUS_FAKE) continue;

    #ifdef I2CIP_DEBUG_SERIAL
      I2CIP_DEBUG_SERIAL.print(_F("Bus "));
      I2CIP_DEBUG_SERIAL.print(busnum, DEC);
    #endif

    // 2b. Bus Root Object
    if(!bus.is<JsonObject>()) {
      #ifdef I2CIP_DEBUG_SERIAL
        I2CIP_DEBUG_SERIAL.print(_F(" Invalid Routing Structure; "));
        DEBUG_DELAY();
      #endif
      continue;
    }
    JsonObject root = bus.as<JsonObject>();
  
    for (JsonPair kv : root) {
      // 2c. Array of I2C Addresses
      if(!kv.value().is<JsonArray>() || kv.value().isNull()) {
        #ifdef I2CIP_DEBUG_SERIAL
          I2CIP_DEBUG_SERIAL.print(_F(" Invalid Routing Table; "));
          DEBUG_DELAY();
        #endif
        continue;
      }

      const char* key = kv.key().c_str();

      #ifdef I2CIP_DEBUG_SERIAL
        DEBUG_DELAY();
        I2CIP_DEBUG_SERIAL.print(" { \"");
        I2CIP_DEBUG_SERIAL.print(key);
        I2CIP_DEBUG_SERIAL.print("\" : ");
      #endif

      // Get DeviceGroup
      DeviceGroup* dg = this->operator[](key);
      if(dg == nullptr) {
        #ifdef I2CIP_DEBUG_SERIAL
          I2CIP_DEBUG_SERIAL.print(_F("LIB ENOENT,"));
          DEBUG_DELAY();
        #endif
        // Remove this JsonPair
        kv.value().clear();
        continue;
      }

      uint8_t numfqas = kv.value().size();
      if(numfqas == 0) {
        #ifdef I2CIP_DEBUG_SERIAL
          I2CIP_DEBUG_SERIAL.print(_F("[ NONE ],"));
          DEBUG_DELAY();
        #endif
        continue;
      }

      #ifdef I2CIP_DEBUG_SERIAL
        DEBUG_DELAY();
        I2CIP_DEBUG_SERIAL.print(" (");
        I2CIP_DEBUG_SERIAL.print(numfqas);
        I2CIP_DEBUG_SERIAL.print(") [");
        DEBUG_DELAY();
      #endif

      for (JsonVariant addr : kv.value().as<JsonArray>()) {
        if(!addr.is<unsigned int>()) { continue; }
        uint8_t address = addr.as<uint8_t>();
        if(address > 0b1111111) {
          #ifdef I2CIP_DEBUG_SERIAL
            I2CIP_DEBUG_SERIAL.print(" 0x");
            I2CIP_DEBUG_SERIAL.print(address, HEX);
          #endif
          // Remove this JsonVariant
          addr.clear();
          continue;
        }
        #ifdef I2CIP_DEBUG_SERIAL
          I2CIP_DEBUG_SERIAL.print(" 0x");
          I2CIP_DEBUG_SERIAL.print(address, HEX);
        #endif
      }

      #ifdef I2CIP_DEBUG_SERIAL
        I2CIP_DEBUG_SERIAL.print(" ]");
        DEBUG_DELAY();
      #endif
    }
    #ifdef I2CIP_DEBUG_SERIAL
      I2CIP_DEBUG_SERIAL.print(" PASS, ");
      DEBUG_DELAY();
    #endif
  }
  unsigned long delta = millis() - now;
  #ifdef I2CIP_DEBUG_SERIAL
    I2CIP_DEBUG_SERIAL.print(delta / 1000.0, 3);
    I2CIP_DEBUG_SERIAL.println("s");
    DEBUG_DELAY();
  #endif
  return true;
}