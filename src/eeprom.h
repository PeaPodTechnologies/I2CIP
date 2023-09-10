#ifndef I2CIP_EEPROM_H_
#define I2CIP_EEPROM_H_

#include <fqa.h>
#include <device.h>

#define I2CIP_EEPROM_SIZE     100   // EEPROM size in bytes
#define I2CIP_EEPROM_ADDR     0x50  // SPRT EEPROM address
#define I2CIP_EEPROM_TIMEOUT  100    // How long to wait for a write to complete (ms)

extern const char* id_eeprom;

namespace I2CIP {

  class Device;
  template <typename G, typename A, typename S, typename B> class IOInterface;

  typedef struct { uint16_t pos; size_t len; } args_eeprom_t;

  class EEPROM : public Device, public IOInterface<uint8_t*, args_eeprom_t, uint8_t*, args_eeprom_t> {
  public:
    EEPROM(const i2cip_fqa_t& fqa);
    EEPROM(const uint8_t& wire, const uint8_t& module, const uint8_t& addr = I2CIP_EEPROM_ADDR);

    i2cip_errorlevel_t readContents(uint8_t* dest, size_t& num_read, size_t max_read = I2CIP_EEPROM_SIZE);

    i2cip_errorlevel_t writeByte(const uint16_t& bytenum, const uint8_t& value, bool setbus = true);

    i2cip_errorlevel_t clearContents(bool setbus = true, uint16_t numbytes = I2CIP_EEPROM_SIZE);

    i2cip_errorlevel_t overwriteContents(const char* contents, bool clear = true, bool setbus = true);

    i2cip_errorlevel_t overwriteContents(uint8_t* buffer, size_t len, bool clear = true, bool setbus = true);

    /**
     * Read a section from EEPROM.
     * @param dest Byte
     * @param args Address to read from
     **/
    i2cip_errorlevel_t get(uint8_t*& dest, const args_eeprom_t& args) override;
  
    /**
     * Write to a section of EEPROM.
     * @param value New byte
     * @param args Address to write to
     **/
    i2cip_errorlevel_t set(uint8_t* const& value, const args_eeprom_t& args) override;
  };
}

#endif