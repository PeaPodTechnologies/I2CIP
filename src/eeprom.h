#ifndef I2CIP_EEPROM_H_
#define I2CIP_EEPROM_H_

#include <fqa.h>
#include <device.h>
#include <interface.h>

#define I2CIP_EEPROM_SIZE     100   // EEPROM size in bytes
#define I2CIP_EEPROM_ADDR     80    // SPRT EEPROM address (0x50)
#define I2CIP_EEPROM_TIMEOUT  1000   // If we're going to crash on a module ping fail, we should wait a bit
#define I2CIP_GUARANTEE_EEPROM 2432

#ifdef I2CIP_USE_GUARANTEES
I2CIP_GUARANTEE_DEFINE(EEPROM, I2CIP_GUARANTEE_EEPROM);
#endif

#define I2CIP_EEPROM_ID       "24LC32"
#define STR_IMPL_(x) #x      //stringify argument
#define STR(x) STR_IMPL_(x)  //indirection to expand argument macros
#define I2CIP_EEPROM_DEFAULT  "[{\"" I2CIP_EEPROM_ID "\":[" STR(I2CIP_EEPROM_ADDR) "]}]"

// Future-Proofing ;)
// namespace ControlSystemsOS {
//   class Linker;
// }

namespace I2CIP {

  class Device;
  class Module;
  template <typename G, typename A, typename S, typename B> class IOInterface;

  const char i2cip_eeprom_default[] PROGMEM = {I2CIP_EEPROM_DEFAULT};
  const uint16_t i2cip_eeprom_capacity = I2CIP_EEPROM_SIZE;

  /**
   * 24LC32 EEPROM
   * 
   * This class is a wrapper for on- and off-module EEPROM as an I2CIP device and a high-level null-terminated UTF-8 C-string I/O peripheral.
   * NOTE: This object handles all C-string memory management, including allocation and deallocation. All pass-by-ref(!) pointer arguments are reassigned. Similar array-valued interfaces should follow the same pattern.
   * 
   * G - Getter type: char* (null-terminated; writable heap)
   * A - Getter argument type: uint16_t (max bytes to read)
   * S - Setter type: const char* (null-terminated; immutable)
   * B - Setter argument type: uint16_t (max bytes to write)
  */
  class EEPROM
    : public Device
    , public IOInterface<char*, uint16_t, const char*, uint16_t> 
    #ifdef I2CIP_USE_GUARANTEES
    , public Guarantee<EEPROM>
    #endif
    {
    I2CIP_DEVICE_CLASS_BUNDLE(EEPROM, I2CIP_EEPROM_ID);
    
    #ifdef I2CIP_USE_GUARANTEES
    I2CIP_CLASS_USE_GUARANTEE(EEPROM, I2CIP_GUARANTEE_EEPROM);
    #endif
      // friend Device* I2CIP::eepromFactory(i2cip_fqa_t fqa);
      // friend class ControlSystemsOS::Linker; // Future-Proofing ;)
    private:
      friend class Module;

      static bool _failsafe_set;
      static char _failsafe[]; // to be loaded from progmem
      static uint16_t _failsafe_b;

      char readBuffer[I2CIP_EEPROM_SIZE+1] = { '\0' };

    public:
      EEPROM(i2cip_fqa_t fqa, const i2cip_id_t& id);

      ~EEPROM();

      i2cip_errorlevel_t readContents(uint8_t* dest, size_t& num_read, size_t max_read = I2CIP_EEPROM_SIZE);

      i2cip_errorlevel_t writeByte(const uint16_t& bytenum, const uint8_t& value, bool setbus = true);

      i2cip_errorlevel_t clearContents(bool setbus = true, uint16_t numbytes = I2CIP_EEPROM_SIZE);

      i2cip_errorlevel_t overwriteContents(const char* contents, bool clear = true, bool setbus = true);

      i2cip_errorlevel_t overwriteContents(uint8_t* buffer, size_t len, bool clear = true, bool setbus = true);

      /**
       * Read a section from EEPROM.
       * @param dest Destination heap (pointer reassigned, not overwritten)
       * @param args Number of bytes to read
       **/
      i2cip_errorlevel_t get(char*& dest, const uint16_t& args) override;
    
      /**
       * Write to a section of EEPROM.
       * @param value Value to write (null-terminated)
       * @param args Number of bytes to write
       **/
      i2cip_errorlevel_t set(const char * const& value, const uint16_t& args) override;

      void clearCache(void) override;
      const uint16_t& getDefaultA(void) const override;
      void resetFailsafe(void) override;
      const uint16_t& getDefaultB(void) const override;

      const char* cacheToString(void) override { return readBuffer; } // Simple manual cache return
  };
}

#endif