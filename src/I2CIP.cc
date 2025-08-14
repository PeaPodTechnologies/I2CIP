#include "I2CIP.hpp"

#include <ArduinoJson.h>

using namespace I2CIP;

BST<i2cip_fqa_t, Device*> I2CIP::devicetree = BST<i2cip_fqa_t, Device*>();
// HashTable<DeviceGroup&> I2CIP::devicegroups = HashTable<DeviceGroup&>();
Module* I2CIP::modules[I2CIP_MUX_COUNT] = { nullptr };
i2cip_errorlevel_t I2CIP::errlev[I2CIP_MUX_COUNT] = { I2CIP_ERR_NONE };

bool JsonModule::parseEEPROMContents(const char* buffer) {
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(_F("-> Deserializing Module EEPROM to JSON: "));
    DEBUG_DELAY();
  #endif

  // 0. Buffer size
  size_t buflen = strlen(buffer);
  
  // 1. EEPROM -> JSON Deserialization
  // TODO: Buflen + 1 ?
  JsonDocument eeprom_json;
  // DeserializationError jsonerr = deserializeJson(this->eeprom_json, buffer, buflen);
  DeserializationError jsonerr = deserializeJson(eeprom_json, buffer, buflen);

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print("Code 0x");
    I2CIP_DEBUG_SERIAL.print(jsonerr.code(), 16);
    I2CIP_DEBUG_SERIAL.print("\n");
    DEBUG_DELAY();
  #endif

  // 2. Schema Validation and Loading
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(_F("-> Verifying JSON:\n"));
  #endif

  // 2a. Base Array of Busses
  // if(!this->eeprom_json.is<JsonArray>() || !this->eeprom_json[0].is<JsonObject>()) {
  if(!eeprom_json.is<JsonArray>() || !eeprom_json[0].is<JsonObject>()) {
    #ifdef I2CIP_DEBUG_SERIAL
      I2CIP_DEBUG_SERIAL.print(_F("Bad JSON: Invalid Structure!\n"));
      DEBUG_DELAY();
    #endif
    return false;
  }

  // JsonArray busses = this->eeprom_json.as<JsonArray>();
  JsonArray busses = eeprom_json.as<JsonArray>();

  int busnum = -1;
  for (JsonVariant bus : busses) {
    busnum++;

    #ifdef I2CIP_DEBUG_SERIAL
      I2CIP_DEBUG_SERIAL.print("[BUS ");
      I2CIP_DEBUG_SERIAL.print(busnum+1, HEX);
      I2CIP_DEBUG_SERIAL.print("]\n");
    #endif

    // 2b. Bus Root Object
    if(!bus.is<JsonObject>()) {
      #ifdef I2CIP_DEBUG_SERIAL
        I2CIP_DEBUG_SERIAL.print(_F("Bad JSON: Invalid Bus Structure!\n"));
        DEBUG_DELAY();
      #endif
      continue;
    }
    JsonObject root = bus.as<JsonObject>();
  
    for (JsonPair kv : root) {
      // 2c. Array of I2C Addresses
      if(!kv.value().is<JsonArray>() || kv.value().isNull()) {
        #ifdef I2CIP_DEBUG_SERIAL
          I2CIP_DEBUG_SERIAL.print(_F("Bad JSON: Invalid Entry Value!\n"));
          DEBUG_DELAY();
        #endif
        continue;
      }

      const char* key = kv.key().c_str();

      #ifdef I2CIP_DEBUG_SERIAL
        DEBUG_DELAY();
        I2CIP_DEBUG_SERIAL.print("[ ID '");
        I2CIP_DEBUG_SERIAL.print(key);
        I2CIP_DEBUG_SERIAL.print("']\n");
      #endif

      // Get DeviceGroup
      DeviceGroup* dg = this->operator[](key);
      if(dg == nullptr) {
        #ifdef I2CIP_DEBUG_SERIAL
          DEBUG_DELAY();
          I2CIP_DEBUG_SERIAL.print(_F("-> Group DNE! Check Libraries.\n"));
          DEBUG_DELAY();
        #endif
        break;
      }

      uint8_t numfqas = kv.value().size();
      if(numfqas == 0) {
        #ifdef I2CIP_DEBUG_SERIAL
          DEBUG_DELAY();
          I2CIP_DEBUG_SERIAL.print(_F("-> Empty! (Skipping)\n"));
          DEBUG_DELAY();
        #endif
        continue;
      }
      i2cip_fqa_t fqas[numfqas];

      #ifdef I2CIP_DEBUG_SERIAL
        DEBUG_DELAY();
        I2CIP_DEBUG_SERIAL.print("-> [");
        DEBUG_DELAY();
      #endif

      uint8_t i = 0;
      for (JsonVariant addr : kv.value().as<JsonArray>()) {
        if(!addr.is<unsigned int>()) { continue; }
        uint8_t address = addr.as<uint8_t>();
        #ifdef I2CIP_DEBUG_SERIAL
          DEBUG_DELAY();
          I2CIP_DEBUG_SERIAL.print(" 0x");
          I2CIP_DEBUG_SERIAL.print(address, HEX);
          DEBUG_DELAY();
        #endif
        fqas[i] = createFQA(this->getWireNum(), this->getModuleNum(), (uint8_t)busnum, address);
      }

      #ifdef I2CIP_DEBUG_SERIAL
        DEBUG_DELAY();
        I2CIP_DEBUG_SERIAL.print(" ]\n");
        DEBUG_DELAY();
      #endif

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
          continue;
        } else {
          #ifdef I2CIP_DEBUG_SERIAL
            DEBUG_DELAY();
            I2CIP_DEBUG_SERIAL.print(_F("-> Factory Success! (Adding)\n"));
            DEBUG_DELAY();
          #endif
          bool r = this->add(d);
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

void I2CIP::commandRouter(JsonObject command, Print& out) {
  // if(command.containsKey("rebuild") {
  if(command["rebuild"].is<bool>()) {
    // Rebuild device tree
    bool update = command["rebuild"].as<bool>();

    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.println(_F("-> Gathering Device Tree..."));
      DEBUG_DELAY();
    #endif

    I2CIP::rebuildTree(out, update);
  } else if(command["fqa"].is<int>()) {
    int i = command["fqa"].as<int>();
    if(i < 0) {
      return;
    }
    i2cip_fqa_t fqa = (i2cip_fqa_t)i;
    uint8_t m = fqa == I2CIP::sevenSegmentFQA ? 0 : I2CIP_FQA_SEG_MODULE(fqa);
    if(I2CIP::modules[m] != nullptr) {
      #ifdef I2CIP_DEBUG_SERIAL
        DEBUG_DELAY();
        I2CIP_DEBUG_SERIAL.print(_F("-> Routing Command for FQA "));
        I2CIP_DEBUG_SERIAL.print(fqaToString(fqa));
        I2CIP_DEBUG_SERIAL.print(_F(" to Module "));
        I2CIP_DEBUG_SERIAL.print(m, HEX);
        I2CIP_DEBUG_SERIAL.println(_F("..."));
        DEBUG_DELAY();
      #endif
      I2CIP::modules[m]->handleCommand(command, out);
    }
  }
}

void I2CIP::rebuildTree(Print& out, bool update) {
  JsonDocument tree;
  tree["type"] = "tree";
  tree["timestamp"] = millis();
  JsonArray arr = tree["data"].to<JsonArray>();
  for(uint8_t m = 0; m < I2CIP_MUX_COUNT; m++) {
    JsonObject obj = arr.add<JsonObject>();
    if(I2CIP::modules[m] != nullptr) {
      I2CIP::modules[m]->toJSON(obj, update);
    }
  }
  DebugJson::jsonPrintln(tree, out);
}