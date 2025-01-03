#ifndef I2CIP_TESTS_TEST_H_
#define I2CIP_TESTS_TEST_H_

#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>

#include <DebugJson.h>
#include "../src/debug.h"

#include <I2CIP.hpp>
#include <SHT45.h>
#include <HT16K33.h>
#include <PCA9685.h>
#include <JHD1313.h>

// TESTING PARAMETERS
#define WIRENUM 0x00
#define MODULE  0x00
#define I2CIP_TEST_BUFFERSIZE 100 // Need to limit this, or else crash; I think Unity takes up a lot of stack space

#define I2CIP_TEST_EEPROM_BYTE0  '[' // This should be the first character of ANY valid SPRT EEPROM
#define I2CIP_TEST_EEPROM_BYTE1 '{'
#define I2CIP_TEST_EEPROM_WORD  (uint16_t)(I2CIP_TEST_EEPROM_BYTE << 8 | I2CIP_TEST_EEPROM_BYTE2)

#define I2CIP_TEST_EEPROM_OVERWRITE 1 // Uncomment to enable EEPROM overwrite test

// #define EEPROM_JSON_CONTENTS_TEST I2CIP_EEPROM_DEFAULT
#define EEPROM_JSON_CONTENTS_TEST {"[{\"24LC32\":[80],\"SHT45\":[" STR(I2CIP_SHT45_ADDRESS) "]},{\"PCA9685\":[" STR(I2CIP_PCA9685_ADDRESS) "],\"JHD1313\":[" STR(I2CIP_JHD1313_ADDRESS) "]}]"}

#ifdef ESP32
  SET_LOOP_TASK_STACK_SIZE( 32*1024 ); // Thanks to: https://community.platformio.org/t/esp32-stack-configuration-reloaded/20994/8; https://github.com/espressif/arduino-esp32/pull/5173
#endif

#define I2CIP_ESP32RTOS_QUEUESIZE 32

using namespace I2CIP;

class ESP32Module;

class _Profiler {
  public:
    typedef i2cip_id_t (*i2cip_profiler_idget_t)(void); // I.e. Device::getID()

    virtual void setup(void *pvParameters) = 0;
    virtual void loop(void *pvParameters) = 0;
  protected:
    static i2cip_profiler_idget_t idGetters[HASHTABLE_SLOTS] = { nullptr };
    static factory_device_t factories[HASHTABLE_SLOTS] = { nullptr };

    DeviceGroup* deviceGroupFactory(const i2cip_id_t& _id) {
      if(_id == nullptr || _id[0] == '\0') return nullptr;
      for(uint8_t i = 0; i < HASHTABLE_SLOTS; i++) {
        if(idGetters[i] == nullptr) continue;
        i2cip_id_t id = idGetters[i]();
        if(id == nullptr || id[0] == '\0') continue;
        if(strcmp(_id, id) == 0) {
          factory_device_t factory = factories[i];
          if(factory == nullptr) continue; // return nullptr;
          return new DeviceGroup(id, factory, Module::_handle); // virtual with default
        }
      }
      return nullptr;
    }
};

template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> class _Profiler {
  public:
    _Profiler() : Profiler() { 
      for(uint8_t i = 0; i < HASHTABLE_SLOTS; i++) { 
        if(idGetters[i] == nullptr && factories[i] == nullptr) { 
          idGetters[i] = &C::getID;
          factories[i] = &C::factory;
        } else if(idGetters[i] == &C::getID) {
          if(factories[i] != &C::factory) factories[i] = &C::factory;
          break;
        } else if(factories[i] == &C::factory) {
          if(idGetters[i] != &C::getID) idGetters[i] = &C::getID;
          break;
        }
      }
    }
};

template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> class InputProfile {
  virtual i2cip_errorlevel_t handle(C* device, i2cip_args_io_t args = _i2cip_args_io_default) = 0;
  static i2cip_errorlevel_t _handle(C* device, i2cip_args_io_t args = _i2cip_args_io_default);
  virtual void callback(bool comp, const C::i2cip_input_type_t& val, const C::i2cip_input_type_t& ref) = 0;
  static void _callback(bool comp, const C::i2cip_input_type_t& val, const C::i2cip_input_type_t& ref);
};

class ESP32Module : public _Profiler, public JsonModule {
  private:
    Print& out;
    HT16K33 const* sevenseg;
    bool enable_sevenseg = true;
    QueueHandle_t queue_sht45;
    state_sht45_t state_sht45 = { NAN, NAN };
  protected:
    DeviceGroup* deviceGroupFactory(const i2cip_id_t& id) override {
      // DeviceGroup* dg = DeviceGroup::create<EEPROM>(id);
      // if(dg != nullptr) return dg;
      // dg = DeviceGroup::create<SHT45>(id, &(ESP32Module::handle<SHT45>));
      // if(dg != nullptr) return dg;
      // dg = DeviceGroup::create<HT16K33>(id);
      // if(dg != nullptr) return dg;
      // dg = DeviceGroup::create<PCA9685>(id); // Default handler
      // if(dg != nullptr) return dg;
      // dg = DeviceGroup::create<JHD1313>(id); // Special handler: SHT
      // return dg;
      return ((_Profiler*)this)->deviceGroupFactory(id);
    }
  public:
    ESP32Module(const uint8_t wirenum, const uint8_t modulenum, Print& out) : JsonModule(wirenum, modulenum), out(out), sevenseg(new HT16K33(WIRENUM, I2CIP_MUX_NUM_FAKE, I2CIP_MUX_BUS_FAKE, "SEVENSEG")) { 
      this->queue_sht45 = xQueueCreate(I2CIP_ESP32RTOS_QUEUESIZE, sizeof(state_sht45_t));
      I2CIP::modules[modulenum] = this; // Self-add
    }

    i2cip_errorlevel_t handle(Device* device, i2cip_args_io_t args = _i2cip_args_io_default) override;

    template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> i2cip_errorlevel_t handle(C* device, i2cip_args_io_t args = _i2cip_args_io_default);
    
    // Used for function pointer; routes to module instance handle() based on FQA
    static template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> i2cip_errorlevel_t _handle(C* device, i2cip_args_io_t args = _i2cip_args_io_default);

    static template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> i2cip_errorlevel_t handleAll(i2cip_args_io_t args = _i2cip_args_io_default);
    

    void setup(void *pvParameters) {
      out.print(F("[I2CIP | MODULE] "));
      out.print(this->getWireNum(), HEX);
      out.print(":");
      out.print(this->getModuleNum(), HEX);
      out.print(F(":.:. SETUP "));

      i2cip_errorlevel_t errlev = this->operator()();
      if(errlev != I2CIP_ERR_NONE) return; // Do not proceed

      // Add sevenseg via FQA
      errlev = this->operator()<HT16K33>(sevenseg->getFQA(), false);
      if(errlev != I2CIP_ERR_NONE) { this->enable_sevenseg = false; }
    };

    void loop(void *pvParameters) {
      out.print(F("[I2CIP | MODULE] "));
      out.print(this->getWireNum(), HEX);
      out.print(":");
      out.print(this->getModuleNum(), HEX);
      out.print(F(":.:. LOOP "));

      i2cip_errorlevel_t errlev = this->operator()<EEPROM>(this->eeprom, true, _i2cip_args_io_default, this->out);
      if(errlev != I2CIP_ERR_NONE) return; // Do not proceed

      
    }

    // InputInterface
    // template <class C, typename std::enable_if<std::is_base_of<InputGetter, C>::value, int>::type = 0, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> i2cip_errorlevel_t readAll(C::i2cip_input_type_t& dest, i2cip_args_io_t args = _i2cip_args_io_default);

    // operator+=<T>() re
    // static template <class C, typename std::enable_if<std::is_base_of<InputGetter, C>::value, int>::type = 0> bool aggregate(C::i2cip_input_type_t& dest, C::i2cip_input_type_t* values, uint8_t count);
};

i2cip_errorlevel_t ESP32Module::handle(Device* device, i2cip_args_io_t args = _i2cip_args_io_default) override {
  if(device == nullptr) return I2CIP_ERR_SOFT;
  const char* id = device->getID();
  if(id == nullptr) return I2CIP_ERR_SOFT;
  if(id == EEPROM::getID() || strcmp(id, EEPROM::getID()) == 0) { return handle<EEPROM>((EEPROM*)device, args); }
  if(id == SHT45::getID() || strcmp(id, SHT45::getID()) == 0) { return handle<SHT45>((SHT45*)device, args); }
  if(id == HT16K33::getID() || strcmp(id, HT16K33::getID()) == 0) { return handle<HT16K33>((HT16K33*)device, args); }
  if(id == PCA9685::getID() || strcmp(id, PCA9685::getID()) == 0) { return handle<PCA9685>((PCA9685*)device, args); }
  if(id == JHD1313::getID() || strcmp(id, JHD1313::getID()) == 0) { return handle<JHD1313>((JHD1313*)device, args); }
  return I2CIP_ERR_SOFT;
}

template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type> void ESP32Module::handleAll(i2cip_args_io_t args) {
  DeviceGroup* dg = (I2CIP::devicegroups[C::getID()]); // Handle all SHT45 addresses (default args; ping+update+cache -> queue)
  if(dg != nullptr) dg->operator()(args);

  // uint8_t count = 0; bool passed[I2CIP_DEVICES_PER_GROUP] = { false };
  // HashTableEntry<I2CIP::DeviceGroup&>* entry = I2CIP::devicegroups[C::getID()];
  // if(entry != nullptr && entry->value.numdevices > 0) {
  //   for(uint8_t x = 0; x < I2CIP_DEVICES_PER_GROUP; x++) {
  //     C* device = (C*)entry->value.devices[x];
  //     if(device == nullptr) continue;
      
  //     uint8_t m = I2CIP_FQA_SEG_MODULE(device->getFQA());
  //     passed[x] = (I2CIP::modules[m]->operator()<C>(device, true, args, DebugJsonBreakpoints) == I2CIP_ERR_NONE);
  //     if(err > errlev) errlev = err;
  //     if(errlev != I2CIP_ERR_NONE) continue;

  //     count += (passed[x] ? 1 : 0);
  //   }
    
  //   C::i2cip_input_type_t values[c]; bool a = false;
  //   for(uint8_t x = 0; x < I2CIP_DEVICES_PER_GROUP; x++) { if(passed[x]) { a = true; break; } }
  //   if(a) {
  //     uint8_t i = 0;
  //     for(uint8_t x = 0; x < I2CIP_DEVICES_PER_GROUP && i < c; x++) {
  //       if(passed[x]) {
  //         values[i] = (C::i2cip_input_type_t)entry->value.devices[x]->getCache();
  //         i++;
  //       }
  //     }
  //     C::i2cip_input_type_t temp;
  //     bool r = aggregate<C>(temp, values, i);
  //     if(r) dest = temp;
  //     else errlev = I2CIP_ERR_SOFT;
  //   }
  // }
}

// Static _handler: Routes to module instance handle()
template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type> i2cip_errorlevel_t ESP32Module::_handle(C* device, i2cip_args_io_t args) {
  if(device == nullptr) return I2CIP_ERR_SOFT;
  
  uint8_t m = I2CIP_FQA_SEG_MODULE(device->getFQA());
  return (I2CIP::modules[m]->handle<C>(device, args));
}

template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type> i2cip_errorlevel_t ESP32Module::handle(C* device, i2cip_args_io_t args) {
  if(device != nullptr) {
    i2cip_errorlevel_t errlev = this->operator()<C>(device, true, args, this->out);
    if(errlev != I2CIP_ERR_NONE) return errlev;
  } else {
    return I2CIP_ERR_SOFT;
  }
}

// Special Handler: SHT45
template <> i2cip_errorlevel_t ESP32Module::handle(SHT45* device, i2cip_args_io_t args) {
  if(device != nullptr) {
    i2cip_errorlevel_t errlev = this->operator()<SHT45>(device, true, _i2cip_args_io_default, this->out);
    if(errlev != I2CIP_ERR_NONE) return errlev;
    // Handle Cache: Queue TX
    state_sht45_t value = device->getCache();
    if(xQueueSend(this->queue_sht45, &value, 0) != pdTRUE) {
      return I2CIP_ERR_SOFT;
    }

    // DebugJson Telemetry
    const char* json = deviceCacheToString(device);
    if(json != nullptr) {
      JsonDocument data;
      JsonObject root = data.to<JsonObject>();
      telemetry(unsigned long timestamp, T value, String label, Print& out = DEBUG_SERIAL);

        // = deserializeJson(data, );
    }
    return I2CIP_ERR_NONE;
  } else {
    return I2CIP_ERR_SOFT;
  }
}

// Special Handler: PCA9685
template <> i2cip_errorlevel_t ESP32Module::handle(JHD1313* device, i2cip_args_io_t args) {
  if(device != nullptr) {
    i2cip_errorlevel_t errlev = this->operator()<JHD1313>(device, true, _i2cip_args_io_default, this->out);
    if(errlev != I2CIP_ERR_NONE) return errlev;
    // Handle Cache: Queue RX
    state_sht45_t value = { NAN, NAN };
    uint8_t count = uxQueueMessagesWaiting(this->queue_sht45);
    if(count > 0) {
      this->state_sht45.temperature = 0.0f;
      this->state_sht45.humidity = 0.0f;
      while(xQueueReceive(this->queue_sht45, &value, 0) == pdTRUE) {
        this->state_sht45.temperature += value.temperature;
        this->state_sht45.humidity += value.humidity;
      }
      result.temperature /= count;
      result.humidity /= count;
    }
    String msg = String("Time: ") + String(millis() / 1000.f, 3) + "s\n";
    if(temphum) { msg += "T:" + String(this->state_sht45.temperature, 1) + "C H:" + String(this->state_sht45.humidity, 1) + "%"; }
    i2cip_args_io_t largs = { .a = nullptr, .s = &msg, .b = nullptr };
  }
}

// template <> bool ESP32Module::aggregate<SHT45>(state_sht45_t& dest, state_sht45_t* values, uint8_t count) {
//   state_sht45_t result = { 0.0f, 0.0f };
//   for(uint8_t i = 0; i < count; i++) {
//     result.temperature += values[i].temperature;
//     result.humidity += values[i].humidity;
//   }
//   result.temperature /= count;
//   result.humidity /= count;
//   return result;
// }

// template <> bool ESP32Module::aggregate<EEPROM>(char*& dest, char** values, uint8_t count) {
//   // Deserialize, aggregate, and re-serialize

//   // Prep
//   JsonDocument doc;
//   JsonArray arr = doc.to<JsonArray>();

//   // Deserialize
//   for(uint8_t i = 0; i < count; i++) {
//     JsonDocument eeprom_json;
//     bool r = validateEEPROM(values[i], eeprom_json);
//     if(!r) return false;
//     else {
//       // Aggregate
//       JsonArray busses = eeprom_json.as<JsonArray>(); int busnum = -1;
//       for (JsonVariant bus : busses) {
//         busnum++;

//         JsonObject root = bus.as<JsonObject>();
//         for (JsonPair kv : root) {
//           // 2c. Array of I2C Addresses
//           if(!kv.value().is<JsonArray>() || kv.value().isNull()) { continue; }
//           const char* key = kv.key().c_str();

//           // Get DeviceGroup
//           DeviceGroup* dg = this->operator[](key);
//           if(dg == nullptr) { continue; }
//           if(kv.value().size() == 0) { continue; }

//           for (JsonVariant addr : kv.value().as<JsonArray>()) {
//             if(!addr.is<unsigned int>()) { continue; }
//             uint8_t address = addr.as<uint8_t>();
//             JsonObject o = arr[0].isNull() ? arr.createNestedObject(0) : arr[0].as<JsonObject>();
//             JsonArray a = o[key].isNull() ? o.createNestedArray(key) : o[key].as<JsonArray>();
//             a.add(address);
//             if(i >= numfqas) { break; }
//           }
//         }
//       }
//     }
//   }

//   // Serialize
//   return serializeJson(doc, dest) == 0;
// }

// template <class C, typename std::enable_if<std::is_base_of<InputGetter, C>::value, int>::type, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type> i2cip_errorlevel_t ESP32Module::readAll(C::i2cip_input_type_t& dest, i2cip_args_io_t args) {
  // i2cip_errorlevel_t errlev = I2CIP_ERR_NONE; uint8_t count = 0; bool passed[I2CIP_DEVICES_PER_GROUP] = { false };
  // HashTableEntry<I2CIP::DeviceGroup&>* entry = I2CIP::devicegroups[C::getID()];
  // if(entry != nullptr && entry->value.numdevices > 0) {
  //   for(uint8_t x = 0; x < I2CIP_DEVICES_PER_GROUP; x++) {
  //     C* device = (C*)entry->value.devices[x];
  //     if(device == nullptr) continue;
      
  //     passed[x] = (modules[MODULE]->operator()<C>(device, true, args, DebugJsonBreakpoints) == I2CIP_ERR_NONE);
  //     if(err > errlev) errlev = err;
  //     if(errlev != I2CIP_ERR_NONE) continue;

  //     count += (passed[x] ? 1 : 0);
  //   }
    
  //   C::i2cip_input_type_t values[c]; bool a = false;
  //   for(uint8_t x = 0; x < I2CIP_DEVICES_PER_GROUP; x++) { if(passed[x]) { a = true; break; } }
  //   if(a) {
  //     uint8_t i = 0;
  //     for(uint8_t x = 0; x < I2CIP_DEVICES_PER_GROUP && i < c; x++) {
  //       if(passed[x]) {
  //         values[i] = (C::i2cip_input_type_t)entry->value.devices[x]->getCache();
  //         i++;
  //       }
  //     }
  //     C::i2cip_input_type_t temp;
  //     bool r = aggregate<C>(temp, values, i);
  //     if(r) dest = temp;
  //     else errlev = I2CIP_ERR_SOFT;
  //   }
  // }
  // return errlev;
// }

#endif