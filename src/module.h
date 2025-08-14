#ifndef I2CIP_MODULE_H_
#define I2CIP_MODULE_H_

#include <type_traits>

#include <Arduino.h>
#include <Wire.h>

#include "device.h"
#include "interface.h"
#include "eeprom.h"

#include "bst.h"
#include "hashtable.h"

#include "debug.h"

#define I2CIP_FQA_SUBNET_MATCH(fqa, _fqa) (bool)((I2CIP_FQA_SEG_I2CBUS(fqa) == I2CIP_FQA_SEG_I2CBUS(_fqa)) && (I2CIP_FQA_SEG_MODULE(fqa) == I2CIP_FQA_SEG_MODULE(_fqa)))
#define I2CIP_FQA_MODULE_MATCH(fqa, wire, module) (bool)(I2CIP_FQA_SEG_I2CBUS(fqa) == (wire) && I2CIP_FQA_SEG_MODULE(fqa) == (module))
#define I2CIP_FQA_BUSADR_MATCH(fqa, bus, addr) (bool)(I2CIP_FQA_SEG_MUXBUS(fqa) == (bus) && I2CIP_FQA_SEG_DEVADR(fqa) == (addr))

// 0. Forward Declarations and Global Variables
namespace I2CIP { 
  class Module; class DeviceGroup;

  extern BST<i2cip_fqa_t, Device*> devicetree;
  extern Module* modules[I2CIP_MUX_COUNT];
  extern i2cip_errorlevel_t errlev[I2CIP_MUX_COUNT];
};

// 1. NullStream Utility Class
class _NullStream : public Stream {
  // Does nothing. Everything goes nowhere. Bare minimum implementation.
  public:
    int available(void) { return 0; }
    int read(void) { return -1; }
    int peek(void) { return '\0'; } // EOS always
    void flush(void) { }
    size_t write(uint8_t c) { return 1; } // NOP always
};
extern _NullStream NullStream;

namespace I2CIP {

  typedef Device* (* factory_device_t)(i2cip_fqa_t fqa);
  typedef void (* jsonhandler_device_t)(i2cip_args_io_t& argsDest, JsonVariant argsA, JsonVariant argsS, JsonVariant argsB);
  typedef void (* cleanup_device_t)(i2cip_args_io_t& args);

  /** 
   * 2. DeviceGroup Class
   * 
   * This class manages an array of Device* with the same ID.
   * 
   **/
  class DeviceGroup {
    private:
      DeviceGroup(const i2cip_id_t& key, factory_device_t factory, jsonhandler_device_t handler, cleanup_device_t cleanup);
    protected:
      friend class Module; // Allow Module to add/remove devices and create DeviceGroups
      
      // 2A. Device Array Management

      bool add(Device* device);
      bool addGroup(Device* devices[], uint8_t numdevices);
      void remove(Device* device);
      void unready(void);

      
      i2cip_id_t key; // ID of the DeviceGroup
      uint8_t numdevices = 0; // Number of devices in the group
      Device* devices[I2CIP_DEVICES_PER_GROUP] = { nullptr }; // Array of devices in the group
      
    public:
      
      const factory_device_t factory; // Factory function to create devices
      const jsonhandler_device_t handler; // Argument factory from JSON parsing
      const cleanup_device_t cleanup; // Deletes args
      ~DeviceGroup();

      /**
       * 2B. Attempt to create a DeviceGroup with the given ID. Attempts to match the given ID with the ID of the Device class template parameter.
       * @param id The ID to match against the Device class template parameter
       * @tparam C The Device class template parameter
       * @return `nullptr` if the ID does not match; otherwise, a pointer to the new DeviceGroup.
       */
      template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> static DeviceGroup* create(i2cip_id_t id);
      
      // 2C. Device Lookup

      uint8_t getNumDevices(void) const { return this->numdevices; }
      Device* getDevice(uint8_t index) const { return this->devices[index]; }
      bool contains(Device* device) const;

      /**
       * Search for a device by FQA.
       * @param fqa FQA of the device
       * @return Pointer to the device if found, nullptr otherwise
       */
      Device* operator[](const i2cip_fqa_t& fqa) const;

      // DeviceGroup& operator=(const DeviceGroup& rhs);

      /**
       * Find by FQA, or; Instantiate a device with the given FQA using the factory function and insert it into the DeviceGroup.
       * @param fqa The FQA of the device to instantiate
       * @return A pointer to the instantiated device
       */
      Device* operator()(i2cip_fqa_t fqa);
  };

  /**
   * 3. Module Class
   * 
   * This class represents a physical collection of devices attached to the busses of a multiplexer, including an EEPROM (mandatory).
   * 
   **/
  class Module {
    private:
      const uint8_t wire; // I2C Wire Number (Index of `wires[]`)
      const uint8_t mux;  // MUX/Module Number (0x00 - 0x07, address range 0x70 - 0x77)
      
      HashTable<DeviceGroup> devicegroups = HashTable<DeviceGroup>(); // HashTable of DeviceGroup* by ID

      bool eeprom_added = false; // Has this module's EEPROM been added to the DeviceGroup HashTable?

      /**
       * 3A. Check if the given FQA is a part of this module's subnetwork.
       * @note If the given FQA is on a "fake" MUX or bus, this will return `true` - this enables any module to 'operate' on a MUX-NOP'd device.
       * @param fqa FQA of the device to check
       * @return `true` if the given FQA WIRE and MUX bits match those of this module's EEPROM, `false` otherwise
       */
      bool isFQAinSubnet(const i2cip_fqa_t& fqa);

      /**
       * 3B. Add an empty DeviceGroup to the HashTable. Uses `deviceGroupFactory` to create a new DeviceGroup matching the given ID.
       * @param id The ID of the DeviceGroup to attempt to create
       * @return Pointer to the new DeviceGroup if successful, `nullptr` otherwise
       */
      DeviceGroup* addEmptyGroup(const char* id);

    protected:
      EEPROM* const eeprom; // This module's EEPROM device
      
      /**
       * 3C. Factory function to create a DeviceGroup with the given ID.
       * As-implemented, handles only EEPROM DeviceGroup creation. Should be overridden and use `DeviceGroup::create<C>(id)`.
       * @param id The ID of the DeviceGroup to create
       * @return Pointer to the new DeviceGroup if successful, `nullptr` otherwise
       */
      virtual DeviceGroup* deviceGroupFactory(const i2cip_id_t& id) { return DeviceGroup::create<EEPROM>(id); }

      
      // 3D. Network Management
      
      /**
       * Add a device to the network.
       * i. Search HashTable for DeviceGroup; If not found attempt to create using `addEmptyGroup`; Skip if the device is already in the DeviceGroup.
       * ii. Overwrite the BST with the device's FQA and pointer.
       * @param device Pointer to the device to add
       * @param overwrite Whether to overwrite the device in the BST if it already exists (Default: `true`)
       * @return `true` if the device was added successfully, `false` otherwise (e.g. if the device is already in the DeviceGroup, or; the DeviceGroup could not be created)
       */
      bool add(Device* device, bool overwrite = true);
      
      void remove(Device* device, bool del = true);
      void remove(const i2cip_fqa_t& fqa, bool del = true);
      void unready(void); // ready = false for all devices in subnet
      
    public:
      Module(const uint8_t& wire, const uint8_t& module, const uint8_t& eeprom_addr = I2CIP_EEPROM_ADDR);
      Module(const i2cip_fqa_t& eeprom_fqa);
      
      ~Module();

      uint8_t getWireNum(void) const { return this->wire; }
      uint8_t getModuleNum(void) const { return this->mux; }
      inline operator const EEPROM&() const { return *this->eeprom; }

      String toString(void) const { return this->devicegroups.toString(); }
      void toJSON(JsonObject obj, bool pingFilter = false) const;

      /**
       * DeviceGroup Lookup
       * @note If not found, create and add using `addEmptyGroup()`.
       * @param id ID of the DeviceGroup to look up
       * @return Pointer to the DeviceGroup if found, or created; `nullptr` otherwise
      */
      DeviceGroup* operator[](i2cip_id_t id);

      // 3E. Network Operations

      /**
       * Module Self-Check
       * i.   Check MUX - If we have lost the switch the entire subnet is down!
       * ii.  Rebuild EEPROM if necessary
       * iii. Ping EEPROM
       * @returns `I2CIP_ERR_HARD` if MUX ping fails, or; EEPROM ping fails; `I2CIP_ERR_SOFT` if EEPROM parse fails; `I2CIP_ERR_NONE` otherwise
      */
      i2cip_errorlevel_t operator()(void); // Module Self-check

      /**
       * EEPROM Discovery
       * i.   Add this module's EEPROM (sets Module::eeprom_added)
       * ii.  Recurse if EEPROM parse fails; attempts to overwrite with default contents and reparse
       * @param recurse Whether to overwrite and parse EEPROM on fail
       * @returns `false` IFF fail to add EEPROM || failed to ping EEPROM || failed to parse EEPROM; `true` otherwise
      */
      i2cip_errorlevel_t discoverEEPROM(bool recurse = false);

      /**
       * Do something with the contents of this module's EEPROM; e.g. add devices.
       * @note For example implementation, see `I2CIP::JsonModule::parseEEPROMContents()`
       */
      virtual bool parseEEPROMContents(const char* contents) { return true; }


    #ifdef DEBUG_SERIAL
    public:
      /**
       * FQA Handler
       * i. Check Subnet Match
       * ii. Search BST; Not Found: Find/Create DeviceGroup, Find or Factory Device
       * iii. Call Device Handler
       * @param fqa FQA of the device to handle
       * @param update Whether to update the device (set output, get input) or just ping it
       * @param args Arguments for input/output operations
       * @param out Print stream to output to (default: `DEBUG_SERIAL`)
       * @return Error level of the operation
       */
      template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> i2cip_errorlevel_t operator()(i2cip_fqa_t fqa, bool update = false, i2cip_args_io_t args = _i2cip_args_io_default, Print& out = DEBUG_SERIAL);

      /**
       * DeviceGroup Handler
       * i. Find/Create DeviceGroup
       * ii. Foreach Device: Check Subnet Match, Call Device Handler
       * @param id ID of the DeviceGroup to handle
       * @param update Whether to update the devices (set output, get input) or just ping them
       * @param args Arguments for input/output operations
       * @param out Print stream to output to (default: `DEBUG_SERIAL`)
       * @return Error level of the operation
       */
      template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> i2cip_errorlevel_t operator()(i2cip_id_t id, bool update = false, i2cip_args_io_t args = _i2cip_args_io_default, Print& out = DEBUG_SERIAL);
    protected:
      /**
       * Device Handler
       * IF UPDATE :
       * i. Set MUX Bus
       * ii. If Device has Output, Device->Set
       * iii. If Device has Input, Device->Get
       * IF NOT UPDATE : Just Ping
       * @note Prints to `out` in the format: `I2C[{wire}]:{module}:{bus}:0x{addr} '{id}' {"PASS"/"EINVAL"/"EIO"} {time}s INPGET {cache} OUTSET {value}`
       * @param d Pointer to the device to handle
       * @param update Whether to update the device (set output, get input) or just ping it
       * @param args Arguments for input/output operations
       * @param out Print stream to output to (default: `DEBUG_SERIAL`)
       * @return Error level of the operation
       */
      i2cip_errorlevel_t operator()(Device* d, bool update = false, i2cip_args_io_t args = _i2cip_args_io_default, Print& out = DEBUG_SERIAL);
    #else
      // is there a "null" stream in Arduino? I don't think so. Would it be easy to implement? Probably.
    public:
      template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> i2cip_errorlevel_t operator()(i2cip_fqa_t fqa, bool update = false, i2cip_args_io_t args = _i2cip_args_io_default, Print& out = NullStream);
      template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> i2cip_errorlevel_t operator()(i2cip_id_t id, bool update = false, i2cip_args_io_t args = _i2cip_args_io_default, Print& out = NullStream);
    protected:
      i2cip_errorlevel_t operator()(Device* d, bool update = false, i2cip_args_io_t args = _i2cip_args_io_default, Print& out = NullStream);
    #endif
    
    public:
      
      // 3F. Command and Configuration Handlers

      /**
       * Handle commands from DebugJson.
       */
      virtual void handleCommand(JsonObject command, Print& out) = 0; // Default: Do nothing

      /**
       * Handle configuration from DebugJson.
       */
      virtual void handleConfig(JsonObject config, Print& out) = 0; // Default: Do nothing
      
      // Output helpers for your various Devices

      // template <class C> static const char* cacheToString(C* that) {
      //   return that == nullptr ? nullptr : that->cacheToString();
      // }
      #ifdef I2CIP_INPUTS_USE_TOSTRING
      #ifdef DEBUG_SERIAL
      template <class C, typename std::enable_if<std::is_base_of<InputGetter, C>::value, int>::type = 0> static void printDevice(C* that, Print& out = DEBUG_SERIAL);
      #else
      template <class C
      // , typename std::enable_if<!std::is_same<C, unsigned short>::value, int>::type = 0
      , typename std::enable_if<std::is_base_of<InputGetter, C>::value, int>::type = 0
      > static void printDevice(C* that, Print& out);
      #endif
      // template <> void printCache(i2cip_fqa_t* that, Print& out) { return; }

      template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> static String deviceCacheToString(C* that);
      // #ifdef DEBUG_SERIAL
      // template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> static void toString(C* that, Print& out = DEBUG_SERIAL, bool printCache = true);
      // #else
      // template <class C
      //   // , typename std::enable_if<!std::is_same<C, unsigned short>::value, int>::type = 0
      //   , typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0
      // > static void toString(C* that, Print& out, bool printCache = true);
      // #endif
      #endif
      // template <> void toString(i2cip_fqa_t* that, Print& out) { return; }
  };
}

#include "module.tpp"

#endif