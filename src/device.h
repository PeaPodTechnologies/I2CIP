#ifndef I2CIP_DEVICE_H_
#define I2CIP_DEVICE_H_

#include <Arduino.h>
#include <Wire.h>

#include "fqa.h"
#include "mux.h"

#define I2CIP_DEVICE_TIMEOUT 10

/**
 * Shifts and masks a number's bits.
 * @param data  Data to shift/mask
 * @param lsb   LSB position
 * @param bits  Number of bits to keep
 **/
#define READ_BITS(data, lsb, bits) (((data) >> (lsb)) & ((1 << (bits)) - 1))

/**
 * Overwrites some bits in existing data.
 * @param existing  Existing data
 * @param data      New data
 * @param lsb       Position to insert (LSB)
 * @param bits      Number of bits to overwrite
 **/
#define OVERWRITE_BITS(existing, data, lsb, bits) (((existing) & ~(((1 << (bits)) - 1) << (lsb))) | (((data) & ((1 << (bits)) - 1)) << (lsb)))

namespace I2CIP { class Device; }

#define I2CIP_DEVICES_PER_GROUP ((size_t)8)
#define I2CIP_ID_SIZE ((size_t)10)
#define I2CIP_INPUT_CACHEBUFFER_SIZE 32
#define I2CIP_INPUT_PRINTBUFFER_SIZE 64

#define I2CIP_DEVICE_USE_FACTORY(CLASS, ...) \
  public:\
    static Device* factory(i2cip_fqa_t fqa, const i2cip_id_t& id) { return (Device*)(new CLASS(fqa, id)); }

#define VALUE_IFNOT_TEST(...) __VA_ARGS__
#define VALUE_IFNOT_TEST0(...) __VA_ARGS__
#define VALUE_IFNOT_TEST1(...)
#define VALUE_IFNOT(COND, ...) VALUE_IFNOT_TEST ## COND ( __VA_ARGS__ )
#define HANDLE_CLASS_ID_VARGS(CLASS, ...) __VA_OPT__(__VA_ARGS__)VALUE_IFNOT(__VA_OPT__(1), #CLASS)

// #define I2CIP_DEVICES_USE_PROGMEM_STATIC_IDS true // uncomment to disable static ID buffers and functions

// PROGMEM IDs are flat-out necessary for embedded systems because of poor memory management and volatility - ON AVR, NOT ON ESP32
#ifdef I2CIP_DEVICES_USE_PROGMEM_STATIC_IDS
#define I2CIP_DEVICE_USE_STATIC_ID() \
  private:\
    static char _id[];\
    static bool _id_set;\
    static const char _id_progmem[] PROGMEM;\
  protected:\
    const char* getStaticID() override { return _id_set ? _id : loadProgmemToStaticID(_id_progmem); }\
    void setStaticID(const char* id) override {\
      strncpy(_id, id, I2CIP_ID_SIZE);\
      _id_set = true;\
    }\
    static const char* getID() { return _id_set ? _id : loadProgmemToStaticID(_id_progmem); }

#define I2CIP_DEVICE_INIT_STATIC_ID(CLASS, ...) \
  char CLASS::_id[I2CIP_ID_SIZE] = { '\0' };\
  bool CLASS::_id_set = false;\
  const char CLASS::_id_progmem[] PROGMEM = {HANDLE_CLASS_ID_VARGS(CLASS __VA_OPT__(,) __VA_ARGS__)};\

// #define I2CIP_DEVICES_INIT_PROGMEM_ID(CLASS, ID) \
//   const char PROGMEM CLASS::_id_progmem[] = {#ID};

// #define I2CIP_DEVICES_INIT_PROGMEM_ID(CLASS) \
//   const char PROGMEM CLASS::_id_progmem[] = {#CLASS};

#define I2CIP_DEVICE_USE_SFACTORY(CLASS, ...) \
  public:\
    CLASS(i2cip_fqa_t fqa) : CLASS(fqa, getStaticID()) { }\
    static Device* factory(i2cip_fqa_t fqa) { return (Device*)(new CLASS(fqa)); }

#else

#define I2CIP_DEVICE_USE_STATIC_ID(...) \
  protected:\
    const char* getStaticID() override;\
  public:\
    static const char* getID();\
    // void setStaticID(const char* id) override { }

#define I2CIP_DEVICE_INIT_STATIC_ID(CLASS, ...) \
  const char* CLASS::getID() { return HANDLE_CLASS_ID_VARGS(CLASS __VA_OPT__(,) __VA_ARGS__); }\
  const char* CLASS::getStaticID() { return CLASS::getID(); }

#define I2CIP_DEVICE_USE_SFACTORY(CLASS, ...) \
  public:\
    CLASS(i2cip_fqa_t fqa) : CLASS(fqa, HANDLE_CLASS_ID_VARGS(CLASS __VA_OPT__(,) __VA_ARGS__)) { }\
    static Device* factory(i2cip_fqa_t fqa) { return (Device*)(new CLASS(fqa)); }

#endif

// #define I2CIP_DEVICE_INIT_STATIC_ID(CLASS) I2CIP_DEVICE_INIT_STATIC_ID(CLASS, CLASS)

// Bundle and Present the API
// #define I2CIP_DEVICE_CLASS_BUNDLE(CLASS) \
//   I2CIP_DEVICE_USE_STATIC_ID(CLASS);\
//   I2CIP_DEVICE_USE_FACTORY(CLASS);\
//   I2CIP_DEVICE_USE_SFACTORY(CLASS, CLASS);

#define I2CIP_DEVICE_CLASS_BUNDLE(CLASS, ...) \
  I2CIP_DEVICE_USE_STATIC_ID();\
  I2CIP_DEVICE_USE_FACTORY(CLASS  __VA_OPT__(,) __VA_ARGS__);\
  I2CIP_DEVICE_USE_SFACTORY(CLASS  __VA_OPT__(,) __VA_ARGS__);

// ARGS is implied to be JSON-friendly
#define I2CIP_INPUTS_USE_TOSTRING true // uncomment to disable input cache toString/print macros
#define I2CIP_INPUT_USE_TOSTRING(TYPE, ARGS)\
private:\
  char cache_buffer[I2CIP_INPUT_CACHEBUFFER_SIZE];\
public:\
  const char* cacheToString(void) override {\
    memset(this->cache_buffer, 0, I2CIP_INPUT_CACHEBUFFER_SIZE);\
    TYPE value = this->getCache();\
    snprintf(this->cache_buffer, I2CIP_INPUT_CACHEBUFFER_SIZE, ARGS, value);\
    return this->cache_buffer;\
  }

// ARGS is whatever you want; overrides default (printCache = cacheToString)
#define I2CIP_INPUT_ADD_PRINTCACHE(TYPE, ARGS)\
private:\
    char print_buffer[I2CIP_INPUT_PRINTBUFFER_SIZE];\
public:\
  const char* printCache(void) override {\
    memset(this->print_buffer, 0, I2CIP_INPUT_PRINTBUFFER_SIZE);\
    TYPE value = this->getCache();\
    snprintf(this->print_buffer, I2CIP_INPUT_PRINTBUFFER_SIZE, ARGS, value);\
    return this->print_buffer;\
  }

#define I2CIP_OUTPUTS_USE_TOSTRING true // uncomment to disable input cache toString/print macros
#define I2CIP_OUTPUT_USE_TOSTRING(TYPE, ARGS, ...)\
private:\
  char value_buffer[I2CIP_INPUT_CACHEBUFFER_SIZE];\
public:\
  const char* valueToString(void) override {\
    memset(this->value_buffer, 0, I2CIP_INPUT_CACHEBUFFER_SIZE);\
    TYPE value = this->getValue();\
    snprintf(this->value_buffer, I2CIP_INPUT_CACHEBUFFER_SIZE, ARGS, __VA_OPT__(__VA_ARGS__)VALUE_IFNOT(__VA_OPT__(1), value));\
    return this->value_buffer;\
  }

#define I2CIP_OUTPUTS_USE_FAILSAFE true // uncomment to disable ouput set-value failsafe defaulting
#ifdef I2CIP_OUTPUTS_USE_FAILSAFE
#define I2CIP_OUTPUT_USE_FAILSAFE(TYPE, TYPEB, ...)\
private:\
  static TYPE _default_s;\
  static TYPEB _default_b;\
public:\
  void resetFailsafe(void) override {\
    this->setValue(_default_s);\
  }\
  __VA_OPT__(__VA_ARGS__)VALUE_IFNOT(__VA_OPT__(1), const TYPEB)& getDefaultB(void) const override { return _default_b; }

#define I2CIP_OUTPUT_INIT_FAILSAFE(CLASS, TYPE, ARGS, TYPEB, ARGSB) \
  TYPE CLASS::_default_s = ARGS;\
  TYPEB CLASS::_default_b = ARGSB;

#endif

#define I2CIP_INPUTS_USE_RESET true // uncomment to disable input set-value reset defaulting
#ifdef I2CIP_INPUTS_USE_RESET
#define I2CIP_INPUT_USE_RESET(TYPE, TYPEA, ...)\
  private:\
    static TYPE _default_g;\
    static TYPEA _default_a;\
  public:\
    void clearCache(void) override { this->setCache(_default_g); }\
    __VA_OPT__(__VA_ARGS__)VALUE_IFNOT(__VA_OPT__(1), const TYPEA)& getDefaultA(void) const override { return _default_a; }

#define I2CIP_INPUT_INIT_RESET(CLASS, TYPE, ARGS, TYPEA, ARGSA) \
  TYPE CLASS::_default_g = ARGS;\
  TYPEA CLASS::_default_a = ARGSA;
#endif


typedef enum { PIN_OFF = LOW, PIN_ON = HIGH, PIN_UNDEF } i2cip_state_pin_t;

namespace I2CIP {

  extern struct i2cip_args_io_s {
    const void* a;
    const void* s;
    const void* b;
  } _i2cip_args_io_default;

  typedef struct i2cip_args_io_s i2cip_args_io_t;

  class Module;
  template <typename G, typename A> class InputInterface;
  template <typename S, typename B> class OutputInterface;

  // typedef enum { I2CIP_ITYPE_NULL = 0b00, I2CIP_ITYPE_INPUT = 0b01, I2CIP_ITYPE_OUTPUT = 0b10, I2CIP_ITYPE_IO = 0b11 } i2cip_itype_t;

  // Barebones template-less abstract classes expose voidptr hooks for the device to be used as an input or output

  class InputGetter {
    protected:
      static const char failptr_get = '\a';
      unsigned long lastrx = 0; // Set by InputInterface
    public:
      virtual ~InputGetter() = 0;
      // virtual i2cip_errorlevel_t get(const void* args = nullptr) { return I2CIP_ERR_HARD; } // Unimplemented; delete this device
      virtual i2cip_errorlevel_t get(const void* args = nullptr) = 0; // Unimplemented; delete this device
      i2cip_errorlevel_t failGet(void) { 
        #ifdef I2CIP_DEBUG_SERIAL
          I2CIP_DEBUG_SERIAL.println(F("INPUT DEFAULT"));
        #endif
        return this->get(&failptr_get);
      }

      unsigned long getLastRX(void) const { return this->lastrx; }

      #ifdef I2CIP_INPUTS_USE_TOSTRING
        virtual const char* cacheToString(void) = 0; // To be implemented by the child class (i.e. for debugging, sensors)
        virtual const char* printCache(void) { return this->cacheToString(); } // Default to cacheToString
      #endif
  };

  class OutputSetter {
    protected:
      static const char failptr_set = '\a';
      unsigned long lasttx = 0;
    public:
      virtual ~OutputSetter() = 0;
      virtual i2cip_errorlevel_t set(const void* value, const void* args = nullptr) = 0; // Unimplemented; delete this device
      // virtual i2cip_errorlevel_t set(const void* value = nullptr, const void* args = nullptr) { return I2CIP_ERR_HARD; } // Unimplemented; delete this device
      i2cip_errorlevel_t reset(const void* args = nullptr) { 
        #ifdef I2CIP_DEBUG_SERIAL
          I2CIP_DEBUG_SERIAL.println(F("OUTPUT RESET"));
        #endif
        return this->set(&failptr_set, args); }
      i2cip_errorlevel_t failSet(const void* value = nullptr) { 
        #ifdef I2CIP_DEBUG_SERIAL
          I2CIP_DEBUG_SERIAL.println(F("OUTPUT FAILSAFE"));
        #endif
        return this->set(value, &failptr_set); }
      
      unsigned long getLastTX(void) const { return this->lasttx; }
      
      #ifdef I2CIP_OUTPUTS_USE_TOSTRING
        virtual const char* valueToString(void) = 0; // To be implemented by the child class (i.e. for debugging, sensors)
      #endif
  };

  typedef i2cip_errorlevel_t (*i2cip_device_begin_t)(const i2cip_fqa_t& fqa, bool setbus);

  class Device {
    private:
      bool _begin(bool setbus);
      // TODO: Rejig member protection
    protected:
      const i2cip_fqa_t fqa;
      i2cip_id_t id;
      const uint16_t timeout;

      bool ready = false; // set false in constructor iff begin != nullptr
      virtual i2cip_errorlevel_t begin(bool setbus) { return I2CIP_ERR_NONE; }

      // // Set by public API, deleted on deconstruction
      InputGetter* input = nullptr;
      OutputSetter* output = nullptr;

      void setInput(InputGetter* input);
      void setOutput(OutputSetter* output);
      template <typename G, typename A> friend class InputInterface;
      template <typename S, typename B> friend class OutputInterface;

      Device(i2cip_fqa_t fqa, i2cip_id_t id, unsigned int timeout = I2CIP_DEVICE_TIMEOUT);
      // Device(i2cip_fqa_t fqa) : Device(fqa, getStaticID()) { }
      // Device(i2cip_fqa_t fqa, const char id_progmem[] PROGMEM, char* staticBuffer); // Replaced by ADD_STATIC_ID(PROGMEM_ID)

      /**
       * Attempt to communicate with a device. Always sets the bus.
       * | MUX ADDR (7) | MUX CONFIG (8) | ACK? | DEV ADDR (7) | ACK? | { resetbus? : MUX ADDR (7) | MUX RESET (8) | ACK? | }
       * @param fqa FQA of the device
       * @param resetbus Should the bus be reset? (Default: `true`)
       * @return Hardware failure: Device lost; no ACK (check MUX). Software failure: Failed to switch MUX bus.
       */
      static i2cip_errorlevel_t ping(const i2cip_fqa_t& fqa, bool resetbus = true, bool setbus = true);

      /**
       * Attempt to communicate with a device repeatedly until timeout. Always sets the bus.
       * | MUX ADDR (7) | MUX CONFIG (8) | ACK? | DEV ADDR (7) | ACK? | { until ACK or timeout: DEV ADDR (7) | ACK? | } { failed? : | MUX ADDR (7) | ACK? | }{ resetbus? : | MUX ADDR (7) | MUX RESET (8) | ACK? |
       * @param fqa FQA of the device
       * @param setbus Should the bus be set? (Default: `true`, set false if checking EEPROM write!)
       * @param resetbus Should the bus be reset? (Default: `true`, set false if checking EEPROM write!)
       * @param timeout Attempt duration (ms)
       * @return Hardware failure: Device unreachable, module check. Software failure: Failed to switch MUX bus
       */
      static i2cip_errorlevel_t pingTimeout(const i2cip_fqa_t& fqa, bool setbus = true, bool resetbus = true, unsigned int timeout = I2CIP_DEVICE_TIMEOUT);

      /**
       * Write one byte to a device.
       * | { setbus? : MUX ADDR (7) | MUX CONFIG (8) | ACK? | } DEV ADDR (7) | DATA BYTE (8) | ACK? | { setbus? : MUX ADDR (7) | MUX RESET (8) | ACK? | }
       * @param fqa FQA of the device
       * @param value Byte to be written
       * @param setbus Should the MUX be set and reset? (Default: `true`)
       * @return Hardware failure: Device and/or module lost. Software failure: Failed to write and/or failed to switch MUX bus
       */
      static i2cip_errorlevel_t writeByte(const i2cip_fqa_t& fqa, const uint8_t& value, bool setbus = true, bool resetbus = false);

      /**
       * Write data to a device.
       * | { setbus? : MUX ADDR (7) | MUX CONFIG (8) | ACK? | } DEV ADDR (7) | DATA BYTE (8 * len) | ACK? | { setbus? : MUX ADDR (7) | MUX RESET (8) | ACK? | }
       * @param fqa FQA of the device
       * @param buffer Bytes to be sent
       * @param len Number of bytes (Default: `1`)
       * @param setbus Should the MUX be set and reset? (Default: `true`)
       * @return Hardware failure: Device and/or module lost. Software failure: Failed to write and/or failed to switch MUX bus
       */
      static i2cip_errorlevel_t write(const i2cip_fqa_t& fqa, const uint8_t* buffer, size_t len = 1, bool setbus = true, bool resetbus = false);

      /**
       * Write one byte to a device's register. Effectively adds ONE prefix byte.
       * | { setbus? : MUX ADDR (7) | MUX CONFIG (8) | ACK? | } DEV ADDR (7) | REG ADDR (16) | DATA BYTE (8) | ACK? | { setbus? : MUX ADDR (7) | MUX RESET  (8) | ACK? | }
       * @param fqa FQA of the device
       * @param reg Register address
       * @param value Byte to be written
       * @param setbus Should the MUX be reset? (Default: `true`)
       * @return Hardware failure: Device and/or module lost. Software failure: Failed to write and/or failed to switch MUX bus
       */
      static i2cip_errorlevel_t writeRegister(const i2cip_fqa_t& fqa, const uint8_t& reg, const uint8_t& value, bool setbus = true, bool resetbus = false);

      /**
       * Write one byte to a device's register (16-bit register address). Effectively adds TWO prefix bytes.
       * | { setbus? : MUX ADDR (7) | MUX CONFIG (8) | ACK? | } DEV ADDR (7) | REG ADDR (16) | DATA BYTE (8) | ACK? | { setbus? : | MUX ADDR (7) | MUX RESET  (8) | ACK? | }
       * @param fqa FQA of the device
       * @param reg Register address
       * @param value Byte to be written
       * @param setbus Should the MUX be reset? (Default: `true`)
       * @return Hardware failure: Device and/or module lost. Software failure: Failed to write and/or failed to switch MUX bus
       */
      static i2cip_errorlevel_t writeRegister(const i2cip_fqa_t& fqa, const uint16_t& reg, const uint8_t& value, bool setbus = true, bool resetbus = false);

      static i2cip_errorlevel_t writeRegister(const i2cip_fqa_t& fqa, const uint8_t& reg, uint8_t* buffer, size_t len = 1, bool setbus = true, bool resetbus = false);

      static i2cip_errorlevel_t writeRegister(const i2cip_fqa_t& fqa, const uint16_t& reg, uint8_t* buffer, size_t len = 1, bool setbus = true, bool resetbus = false);

      /**
       * Request and read in data from a device.
       * | MUX ADDR (7) | MUX CONFIG (8) | ACK? | DEV ADDR (7) | ACK? | DEV ADDR (7) | READ BYTES (8*len) |
       * resetbus? : | MUX ADDR (7) | MUX RESET (8) | ACK? |
       * @param fqa FQA of the device
       * @param dest Bytes to read into
       * @param len Number of bytes to read (Default: `1`)
       * @param setbus Should the MUX be reset? (Default: `true`)
       */
      static i2cip_errorlevel_t read(const i2cip_fqa_t& fqa, uint8_t* dest, size_t& len, bool nullterminate = false, bool resetbus = true, bool setbus = true);

      /**
       * Read one byte of data from the device.
       * | MUX ADDR (7) | MUX CONFIG (8) | ACK? | DEV ADDR (7) | ACK? | DEV ADDR (7) | READ BYTES (8) |
       * resetbus? : | MUX ADDR (7) | MUX RESET (8) | ACK? |
       * @param fqa FQA of the device
       * @param dest Byte to read into
       * @param setbus Should the MUX be reset? (Default: `true`)
       */
      static i2cip_errorlevel_t readByte(const i2cip_fqa_t& fqa, uint8_t& dest, bool resetbus = true, bool setbus = true);

      /**
       * Read one byte of data from the device.
       * | MUX ADDR (7) | MUX CONFIG (8) | ACK? | DEV ADDR (7) | ACK? | DEV ADDR (7) | READ BYTES (16) |
       * resetbus? : | MUX ADDR (7) | MUX RESET (8) | ACK? |
       * @param fqa FQA of the device
       * @param dest Word to read into
       * @param setbus Should the MUX be reset? (Default: `true`)
       */
      static i2cip_errorlevel_t readWord(const i2cip_fqa_t& fqa, uint16_t& dest, bool resetbus = true, bool setbus = true);

      // Loads the RX buffer (wire.read())
      static i2cip_errorlevel_t requestFromRegister(const i2cip_fqa_t& fqa, size_t& len, const uint8_t& reg, bool sendStop = true);
      static i2cip_errorlevel_t requestFromRegister(const i2cip_fqa_t& fqa, size_t& len, const uint16_t& reg, bool sendStop = true);

      static i2cip_errorlevel_t readRegister(const i2cip_fqa_t& fqa, const uint8_t& reg, uint8_t* dest, size_t& len, bool nullterminate = false, bool resetbus = true, bool setbus = true);

      static i2cip_errorlevel_t readRegister(const i2cip_fqa_t& fqa, const uint16_t& reg, uint8_t* dest, size_t& len, bool nullterminate = false, bool resetbus = true, bool setbus = true);
      

      /**
       * Read one byte of data from the device. Effectively adds a prefix byte.
       * | MUX ADDR (7) | MUX CONFIG (8) | ACK? | DEV ADDR (7) | ACK? | DEV ADDR (7) | REG ADDR (8) | READ BYTES (8*len) | { resetbus? : | MUX ADDR (7) | MUX RESET (8) | ACK? | }
       * @param fqa FQA of the device
       * @param dest Bytes to read into
       * @param setbus Should the MUX be reset? (Default: `true`)
       */
      static i2cip_errorlevel_t readRegisterByte(const i2cip_fqa_t& fqa, const uint8_t& reg, uint8_t& dest, bool resetbus = true, bool setbus = true);

      /**
       * Read one byte of data from the device. Effectively adds TWO prefix bytes.
       * | MUX ADDR (7) | MUX CONFIG (8) | ACK? | DEV ADDR (7) | ACK? | DEV ADDR (7) | REG ADDR (16) | READ BYTES (8*len) | { resetbus? : | MUX ADDR (7) | MUX RESET (8) | ACK? | }
       * @param fqa FQA of the device
       * @param dest Bytes to read into
       * @param setbus Should the MUX be reset? (Default: `true`)
       **/
      static i2cip_errorlevel_t readRegisterByte(const i2cip_fqa_t& fqa, const uint16_t& reg, uint8_t& dest, bool resetbus = true, bool setbus = true);

      /**
       * Read two bytes of data from the device. Effectively adds one prefix bytes.
       * | MUX ADDR (7) | MUX CONFIG (8) | ACK? | DEV ADDR (7) | ACK? | DEV ADDR (7) | REG ADDR (16) | READ BYTES (8*len) | { resetbus? : | MUX ADDR (7) | MUX RESET (8) | ACK? | }
       * @param fqa FQA of the device
       * @param dest Bytes to read into
       * @param setbus Should the MUX be reset? (Default: `true`)
       **/
      static i2cip_errorlevel_t readRegisterWord(const i2cip_fqa_t& fqa, const uint8_t& reg, uint16_t& dest, bool resetbus = true, bool setbus = true);

      /**
       * Read two bytes of data from the device. Effectively adds TWO prefix bytes.
       * | MUX ADDR (7) | MUX CONFIG (8) | ACK? | DEV ADDR (7) | ACK? | DEV ADDR (7) | REG ADDR (16) | READ BYTES (8*len) | { resetbus? : | MUX ADDR (7) | MUX RESET (8) | ACK? | }
       * @param fqa FQA of the device
       * @param dest Bytes to read into
       * @param setbus Should the MUX be reset? (Default: `true`)
       **/
      static i2cip_errorlevel_t readRegisterWord(const i2cip_fqa_t& fqa, const uint16_t& reg, uint16_t& dest, bool resetbus = true, bool setbus = true);

    public:
      virtual ~Device() = 0;

      void removeInput(void);
      void removeOutput(void);

      InputGetter* getInput(void) const;
      OutputSetter* getOutput(void) const;

      i2cip_errorlevel_t get(const void* args);
      i2cip_errorlevel_t set(const void* value, const void* args);

      const i2cip_fqa_t& getFQA(void) const;
      const i2cip_id_t& getID(void) const;
      // i2cip_id_t getID(void) const;

      #ifdef I2CIP_DEVICES_USE_PROGMEM_STATIC_IDS
    protected:
      const char* loadProgmemToStaticID(const char load[] PROGMEM) {
        char _id[I2CIP_ID_SIZE] = { '\0' };
        uint8_t len = strlen_P(load);
        for (uint8_t k = 0; k < len; k++) {
          char c = pgm_read_byte_near(load + k);
          _id[k] = c;
        }
        _id[len] = '\0';
        setStaticID(_id);
        return getStaticID();
      }
      virtual void setStaticID(const char* id) = 0;
      #endif
    public:
      void unready(void) { this->ready = false; }
      
      virtual const char* getStaticID() = 0; // Pretty much just a formality to make sure you implement the macro, which has the WAY MORE useful static function variant getID()

      i2cip_errorlevel_t ping(bool resetbus = true, bool setbus = true);
      i2cip_errorlevel_t pingTimeout(bool setbus = true, bool resetbus = true);
      i2cip_errorlevel_t writeByte(const uint8_t& value, bool setbus = true, bool resetbus = false);
      i2cip_errorlevel_t write(const uint8_t* buffer, size_t len = 1, bool setbus = true, bool resetbus = false);
      i2cip_errorlevel_t writeRegister(const uint8_t& reg, const uint8_t& value, bool setbus = true, bool resetbus = false);
      i2cip_errorlevel_t writeRegister(const uint16_t& reg, const uint8_t& value, bool setbus = true, bool resetbus = false);
      i2cip_errorlevel_t writeRegister(const uint8_t& reg, uint8_t* buffer, size_t len = 1, bool setbus = true, bool resetbus = false);
      i2cip_errorlevel_t writeRegister(const uint16_t& reg, uint8_t* buffer, size_t len = 1, bool setbus = true, bool resetbus = false);
      i2cip_errorlevel_t read(uint8_t* dest, size_t& len, bool nullterminate = false, bool resetbus = true, bool setbus = true);
      i2cip_errorlevel_t readByte(uint8_t& dest, bool resetbus = true, bool setbus = true);
      i2cip_errorlevel_t readWord(uint16_t& dest, bool resetbus = true, bool setbus = true);
      i2cip_errorlevel_t readRegister(const uint8_t& reg, uint8_t* dest, size_t& len, bool nullterminate = false, bool resetbus = true, bool setbus = true);
      i2cip_errorlevel_t readRegister(const uint16_t& reg, uint8_t* dest, size_t& len, bool nullterminate = false, bool resetbus = true, bool setbus = true);
      i2cip_errorlevel_t readRegisterByte(const uint8_t& reg, uint8_t& dest, bool resetbus = true, bool setbus = true);
      i2cip_errorlevel_t readRegisterByte(const uint16_t& reg, uint8_t& dest, bool resetbus = true, bool setbus = true);
      i2cip_errorlevel_t readRegisterWord(const uint8_t& reg, uint16_t& dest, bool resetbus = true, bool setbus = true);
      i2cip_errorlevel_t readRegisterWord(const uint16_t& reg, uint16_t& dest, bool resetbus = true, bool setbus = true);

      inline operator i2cip_fqa_t() const { return this->fqa; }
  };
};

#endif