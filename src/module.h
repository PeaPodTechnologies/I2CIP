#ifndef I2CIP_MODULE_H_
#define I2CIP_MODULE_H_

// #include <guarantee.h>
#include <debug.h>
#include <device.h>
#include <interface.h>
#include <eeprom.h>

#include <bst.h>
#include <hashtable.h>

#include <type_traits>

// #define I2CIP_FQA_SUBNET_MATCH(fqa, wire, module) (bool)(I2CIP_FQA_SEG_I2CBUS(fqa) == wire && I2CIP_FQA_SEG_MODULE(fqa) == module)
#define I2CIP_FQA_SUBNET_MATCH(fqa, _fqa) (bool)((I2CIP_FQA_SEG_I2CBUS(fqa) == I2CIP_FQA_SEG_I2CBUS(_fqa)) && (I2CIP_FQA_SEG_MODULE(fqa) == I2CIP_FQA_SEG_MODULE(_fqa)))
#define I2CIP_FQA_MODULE_MATCH(fqa, wire, module) (bool)(I2CIP_FQA_SEG_I2CBUS(fqa) == (wire) && I2CIP_FQA_SEG_MODULE(fqa) == (module))
#define I2CIP_FQA_BUSADR_MATCH(fqa, bus, addr) (bool)(I2CIP_FQA_SEG_MUXBUS(fqa) == (bus) && I2CIP_FQA_SEG_DEVADR(fqa) == (addr))

#ifdef I2CIP_USE_GUARANTEES
#define I2CIP_GUARANTEE_MODULE 0x4D4F4455 // "MODU"
#endif

namespace I2CIP { class Module; }

#ifdef I2CIP_USE_GUARANTEES
I2CIP_GUARANTEE_DEFINE(Module, I2CIP_GUARANTEE_MODULE);
#endif

namespace I2CIP {

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

  typedef Device* (* factory_device_t)(i2cip_fqa_t fqa);

  class DeviceGroup {
    protected:
      friend class Module;

      bool add(Device& device);
      bool add(Device* device);
      bool addGroup(Device* devices[], uint8_t numdevices);
      void remove(Device* device);

      void destruct(void); // TODO: Private?

      // template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> static DeviceGroup* create(i2cip_id_t id);
    public:
      template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> static DeviceGroup* create(i2cip_id_t id);

      i2cip_id_t key;
      uint8_t numdevices = 0;
      Device* devices[I2CIP_DEVICES_PER_GROUP] = { nullptr };

      factory_device_t factory;

      DeviceGroup(const i2cip_id_t& key, factory_device_t factory = nullptr);

      bool contains(Device* device) const;
      bool contains(const i2cip_fqa_t& fqa) const;

      Device* operator[](const i2cip_fqa_t& fqa) const;

      // DeviceGroup& operator=(const DeviceGroup& rhs);

      Device* operator()(i2cip_fqa_t fqa);
  };

  class Module
    #ifdef I2CIP_USE_GUARANTEES
    : Guarantee<Module>
    #endif
    {
    #ifdef I2CIP_USE_GUARANTEES
    I2CIP_CLASS_USE_GUARANTEE(Module, I2CIP_GUARANTEE_MODULE);
    #endif
    private:
      const uint8_t wire; // I2C Wire Number (Index of `wires[]`)
      const uint8_t mux;  // MUX/Module Number (0x00 - 0x07, address range 0x70 - 0x77)

      bool isFQAinSubnet(const i2cip_fqa_t& fqa);

      // Tables/trees are allocated STATICALLY, their entries are dynamic
      BST<i2cip_fqa_t, Device*> devices_fqabst = BST<i2cip_fqa_t, Device*>();
      HashTable<DeviceGroup&> devices_idgroups = HashTable<DeviceGroup&>();

      HashTableEntry<DeviceGroup&>* addEmptyGroup(const char* id);

      bool eeprom_added = false;
      
    protected:
      EEPROM* const eeprom; // EEPROM device - to be added to `devices_fqabst` and `devices_idgroups` on construction
      
    public:
      Module(const uint8_t& wire, const uint8_t& module, const uint8_t& eeprom_addr = I2CIP_EEPROM_ADDR);
      Module(const i2cip_fqa_t& eeprom_fqa);
      
      ~Module();

      uint8_t getWireNum(void) const;
      uint8_t getModuleNum(void) const;

      /**
       * 1. Module Self-check
       * 1a. Check MUX - If we have lost the switch the entire subnet is down!
       * 1b. Rebuild EEPROM if necessary
       * 1c. Ping EEPROM
       * 1d. TODO: Ping Devices
      */
      i2cip_errorlevel_t operator()(void); // Module Self-check

      /**
       * 2. Device Discovery
       * 2a. Read EEPROM
       * 2b. Parse EEPROM
       *  Intended Implementation:
       *  2b i.   Read EEPROM (by Bus)
       *  2b ii.  Ping Devices (by ID)
       *  2b iii. Create (new) Devices and `add` to DeviceGroups (Made Available Internal API via `operator[]` functions)
      */

      /**
       * Discover devices on the module.
       * Side effect: Adds its own EEPROM to the proper DeviceGroup
       * Side effect: Recurses if EEPROM parse fails; attempts to overwrite with default contents and reparse
       * @param recurse {bool} - Whether to recursively parse EEPROM or not
       * @returns `false` IFF fail to add EEPROM | failed to ping EEPROM | failed to parse EEPROM; `true` otherwise
      */
      i2cip_errorlevel_t discoverEEPROM(bool recurse = true);
      virtual bool parseEEPROMContents(const char* contents);

    protected: // Controversial but necessary
      bool add(Device& device, bool overwrite = false);
      bool add(Device* device, bool overwrite = false);
      // virtual DeviceGroup* deviceGroupFactory(const i2cip_id_t& id) = 0;
      virtual DeviceGroup* deviceGroupFactory(const i2cip_id_t& id);

      /**
       * 3. Device Lookup: HashTable<DeviceGroup&> by ID, BST<Device*> by FQA
       *  Intended Implementation:
       *  3a i.   Check if FQA is in subnet
       *  3a ii.  Check if FQA is in DeviceGroups
       * 
       *  3b i.   Check if ID is in DeviceGroups
       *  3b ii.  Return DeviceGroup
       *  
      */
      DeviceGroup* operator[](i2cip_id_t id);
      Device* operator[](const i2cip_fqa_t& fqa) const;

      void remove(Device* device, bool del = true);


      // This function is like operator(Device) except it's cast to the specific Device type and prints FQA, Input cache, etc.
      #ifdef DEBUG_SERIAL
    public:
      template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> i2cip_errorlevel_t operator()(i2cip_fqa_t fqa, bool update = false, i2cip_args_io_t args = _i2cip_args_io_default, Print& out = DEBUG_SERIAL);
    // protected:
      template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> i2cip_errorlevel_t operator()(C& d, bool update = false, i2cip_args_io_t args = _i2cip_args_io_default, Print& out = DEBUG_SERIAL);
      template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> i2cip_errorlevel_t operator()(C* d, bool update = false, i2cip_args_io_t args = _i2cip_args_io_default, Print& out = DEBUG_SERIAL);
      #else
      // is there a "null" stream in Arduino? I don't think so. Would it be easy to implement? Probably.
    public:
      template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> i2cip_errorlevel_t operator()(i2cip_fqa_t fqa, bool update = false, i2cip_args_io_t args = _i2cip_args_io_default, Print& out = NullStream);
    // protected:
      template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> i2cip_errorlevel_t operator()(C& d, bool update = false, i2cip_args_io_t args = _i2cip_args_io_default, Print& out = NullStream); // Mostly for Module::eeprom
      template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> i2cip_errorlevel_t operator()(C* d, bool update = false, i2cip_args_io_t args = _i2cip_args_io_default, Print& out = NullStream);
      #endif
      // template <> i2cip_errorlevel_t I2CIP::Module::operator()(i2cip_fqa_t* ptr, bool update, i2cip_args_io_t args, Print& out) { return I2CIP_ERR_HARD; } // This is the new COCONUT.PNG
      // These are good for background searcheb  s, but unless you're debugging this library I wouldn't personally recommend using them
      // i2cip_errorlevel_t operator()(Device& d, bool update = false, i2cip_args_io_t args = _i2cip_args_io_default);
      // i2cip_errorlevel_t operator()(Device* d, bool update = false, i2cip_args_io_t args = _i2cip_args_io_default);
    
    public:
      inline operator const EEPROM&() const { return *this->eeprom; }
      
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

#include <module.tpp>

#endif