#ifndef I2CIP_H_
#define I2CIP_H_

// HEADERS

#include <Arduino.h>
#include <Wire.h>

#include <i2cip/fqa.h>
#include <i2cip/mux.h>
#include <i2cip/eeprom.h>

// ---------------
// Settings
// ---------------

// #define DEBUG_SERIAL 1      // Uncomment for serial debug messages
#define I2CIP_MAXBUFFER 32  // I2C buffer size
#define I2CIP_NUM_WIRES 1   // Number of I2C wires - TODO: autodetect and populate `wires[]` based on hardware spec macros

extern TwoWire Wire;

static TwoWire* const wires[I2CIP_NUM_WIRES] = { &Wire };

#define I2CIP_FQA_TO_WIRE(fqa) (wires[I2CIP_FQA_SEG_I2CBUS(fqa)])

#define I2CIP_ERR_BREAK(errlev) if((errlev) > I2CIP::I2CIP_ERR_NONE) { return (errlev); }

// -----------------------
// I2C Intranet Protocols
// -----------------------

namespace I2CIP {
  /**
   * Errorlevels for I2CIP communication.
   */
  typedef enum {
    I2CIP_ERR_NONE = 0, // No error
    I2CIP_ERR_SOFT = 1, // Communications error, device still reachable
    I2CIP_ERR_HARD = 2, // Device unreachable
  } i2cip_errorlevel_t;

  /**
   * Create an FQA from segments. Validates.
   * @param wire I2C bus number
   * @param mux MUX number
   * @param bus MUX bus number
   * @param addr Device address
   * @return A valid FQA, or 0 if any segment has an invalid value.
   */
  i2cip_fqa_t createFQA(const uint8_t& wire, const uint8_t& mux, const uint8_t& bus, const uint8_t& addr);

  /**
   * Initialize an I2C interface (if it has not already been initialized)
   * @param fqa
   */
  void beginWire(const i2cip_fqa_t fqa);

  namespace MUX {
    /**
     * Pings the MUX. Begins the Wire.
     * | MUX ADDR (7) | ACK? |
     * @param fqa FQA of a device this MUX is in front of.
     * @return ACK?
     */
    bool pingMUX(const i2cip_fqa_t& fqa);

    /**
     * Sets the MUX bus.
     * | MUX ADDR (7) | MUX CONFIG (8) | ACK? |
     * @param fqa FQA of a device that is on the target Subnet.
     * @return Hardware failure: No ACK; Module lost. Software failure: Failed to write to MUX.
     */
    i2cip_errorlevel_t setBus(const i2cip_fqa_t& fqa);

    /**
     * Reset the MUX to the "inactive" bus.
     * | MUX ADDR (7) | MUX RESET (8) | ACK? |
     * @param fqa FQA of a device that this MUX is in front of
     * @return Hardware failure: No ACK; Module lost. Software failure: Failed to write to MUX.
     */
    i2cip_errorlevel_t resetBus(const i2cip_fqa_t& fqa);
  };

  class Device {
    private:
      const i2cip_fqa_t& fqa;
    public:
      Device(const i2cip_fqa_t& fqa);
      /**
       * Attempt to communicate with a device. Always sets the bus.
       * | MUX ADDR (7) | MUX CONFIG (8) | ACK? | DEV ADDR (7) | ACK? | { resetbus? : MUX ADDR (7) | MUX RESET (8) | ACK? | }
       * @param fqa FQA of the device
       * @param resetbus Should the bus be reset? (Default: `true`)
       * @return Hardware failure: Device lost; no ACK (check MUX). Software failure: Failed to switch MUX bus.
       */
      static i2cip_errorlevel_t ping(const i2cip_fqa_t& fqa, bool resetbus = true);

    /**
     * Attempt to communicate with a device repeatedly until timeout. Always sets the bus.
     * | MUX ADDR (7) | MUX CONFIG (8) | ACK? | DEV ADDR (7) | ACK? | { until ACK or timeout: DEV ADDR (7) | ACK? | } { failed? : | MUX ADDR (7) | ACK? | }{ resetbus? : | MUX ADDR (7) | MUX RESET (8) | ACK? |
     * @param fqa FQA of the device
     * @param setbus Should the bus be set? (Default: `true`, set false if checking EEPROM write!)
     * @param resetbus Should the bus be reset? (Default: `true`, set false if checking EEPROM write!)
     * @param timeout Attempt duration (ms)
     * @return Hardware failure: Device unreachable, module check. Software failure: Failed to switch MUX bus
     */
    static i2cip_errorlevel_t pingTimeout(const i2cip_fqa_t& fqa, bool setbus = true, bool resetbus = true, unsigned int timeout = 100);

    /**
     * Write one byte to a device.
     * | { setbus? : MUX ADDR (7) | MUX CONFIG (8) | ACK? | } DEV ADDR (7) | DATA BYTE (8) | ACK? | { setbus? : MUX ADDR (7) | MUX RESET (8) | ACK? | }
     * @param fqa FQA of the device
     * @param value Byte to be written
     * @param setbus Should the MUX be set and reset? (Default: `true`)
     * @return Hardware failure: Device and/or module lost. Software failure: Failed to write and/or failed to switch MUX bus
     */
    static i2cip_errorlevel_t writeByte(const i2cip_fqa_t& fqa, const uint8_t& value, bool setbus = true);

    /**
     * Write data to a device.
     * | { setbus? : MUX ADDR (7) | MUX CONFIG (8) | ACK? | } DEV ADDR (7) | DATA BYTE (8 * len) | ACK? | { setbus? : MUX ADDR (7) | MUX RESET (8) | ACK? | }
     * @param fqa FQA of the device
     * @param buffer Bytes to be sent
     * @param len Number of bytes (Default: `1`)
     * @param setbus Should the MUX be set and reset? (Default: `true`)
     * @return Hardware failure: Device and/or module lost. Software failure: Failed to write and/or failed to switch MUX bus
     */
    static i2cip_errorlevel_t write(const i2cip_fqa_t& fqa, const uint8_t* buffer, size_t len = 1, bool setbus = true);

    /**
     * Write one byte to a device's register. Effectively adds ONE prefix byte.
     * | { setbus? : MUX ADDR (7) | MUX CONFIG (8) | ACK? | } DEV ADDR (7) | REG ADDR (16) | DATA BYTE (8) | ACK? | { setbus? : MUX ADDR (7) | MUX RESET  (8) | ACK? | }
     * @param fqa FQA of the device
     * @param reg Register address
     * @param value Byte to be written
     * @param setbus Should the MUX be reset? (Default: `true`)
     * @return Hardware failure: Device and/or module lost. Software failure: Failed to write and/or failed to switch MUX bus
     */
    static i2cip_errorlevel_t writeRegister(const i2cip_fqa_t& fqa, const uint8_t& reg, const uint8_t& value, bool setbus = true);

    /**
     * Write one byte to a device's register (16-bit register address). Effectively adds TWO prefix bytes.
     * | { setbus? : MUX ADDR (7) | MUX CONFIG (8) | ACK? | } DEV ADDR (7) | REG ADDR (16) | DATA BYTE (8) | ACK? | { setbus? : | MUX ADDR (7) | MUX RESET  (8) | ACK? | }
     * @param fqa FQA of the device
     * @param reg Register address
     * @param value Byte to be written
     * @param setbus Should the MUX be reset? (Default: `true`)
     * @return Hardware failure: Device and/or module lost. Software failure: Failed to write and/or failed to switch MUX bus
     */
    static i2cip_errorlevel_t writeRegister(const i2cip_fqa_t& fqa, const uint16_t& reg, const uint8_t& value, bool setbus = true);

    static i2cip_errorlevel_t writeRegister(const i2cip_fqa_t& fqa, const uint8_t& reg, uint8_t* buffer, size_t len = 1, bool setbus = true);

    static i2cip_errorlevel_t writeRegister(const i2cip_fqa_t& fqa, const uint16_t& reg, uint8_t* buffer, size_t len = 1, bool setbus = true);

    /**
     * Request and read in data from a device.
     * | MUX ADDR (7) | MUX CONFIG (8) | ACK? | DEV ADDR (7) | ACK? | DEV ADDR (7) | READ BYTES (8*len) |
     * resetbus? : | MUX ADDR (7) | MUX RESET (8) | ACK? |
     * @param fqa FQA of the device
     * @param dest Bytes to read into
     * @param len Number of bytes to read (Default: `1`)
     * @param setbus Should the MUX be reset? (Default: `true`)
     */
    static i2cip_errorlevel_t read(const i2cip_fqa_t& fqa, uint8_t* dest, size_t& len, bool nullterminate = true, bool resetbus = true);

    /**
     * Read one byte of data from the device.
     * | MUX ADDR (7) | MUX CONFIG (8) | ACK? | DEV ADDR (7) | ACK? | DEV ADDR (7) | READ BYTES (8) |
     * resetbus? : | MUX ADDR (7) | MUX RESET (8) | ACK? |
     * @param fqa FQA of the device
     * @param dest Byte to read into
     * @param setbus Should the MUX be reset? (Default: `true`)
     */
    static i2cip_errorlevel_t readByte(const i2cip_fqa_t& fqa, uint8_t& dest, bool resetbus = true);

    /**
     * Read one byte of data from the device.
     * | MUX ADDR (7) | MUX CONFIG (8) | ACK? | DEV ADDR (7) | ACK? | DEV ADDR (7) | READ BYTES (16) |
     * resetbus? : | MUX ADDR (7) | MUX RESET (8) | ACK? |
     * @param fqa FQA of the device
     * @param dest Word to read into
     * @param setbus Should the MUX be reset? (Default: `true`)
     */
    static i2cip_errorlevel_t readWord(const i2cip_fqa_t& fqa, uint16_t& dest, bool resetbus = true);

    static i2cip_errorlevel_t readRegister(const i2cip_fqa_t& fqa, const uint8_t& reg, uint8_t* dest, size_t& len, bool nullterminate = true, bool resetbus = true);

    static i2cip_errorlevel_t readRegister(const i2cip_fqa_t& fqa, const uint16_t& reg, uint8_t* dest, size_t& len, bool nullterminate = true, bool resetbus = true);

    /**
     * Read one byte of data from the device. Effectively adds a prefix byte.
     * | MUX ADDR (7) | MUX CONFIG (8) | ACK? | DEV ADDR (7) | ACK? | DEV ADDR (7) | REG ADDR (8) | READ BYTES (8*len) | { resetbus? : | MUX ADDR (7) | MUX RESET (8) | ACK? | }
     * @param fqa FQA of the device
     * @param dest Bytes to read into
     * @param setbus Should the MUX be reset? (Default: `true`)
     */
    static i2cip_errorlevel_t readRegisterByte(const i2cip_fqa_t& fqa, const uint8_t& reg, uint8_t& dest, bool resetbus = true);

    /**
     * Read one byte of data from the device. Effectively adds TWO prefix bytes.
     * | MUX ADDR (7) | MUX CONFIG (8) | ACK? | DEV ADDR (7) | ACK? | DEV ADDR (7) | REG ADDR (16) | READ BYTES (8*len) | { resetbus? : | MUX ADDR (7) | MUX RESET (8) | ACK? | }
     * @param fqa FQA of the device
     * @param dest Bytes to read into
     * @param setbus Should the MUX be reset? (Default: `true`)
     **/
    static i2cip_errorlevel_t readRegisterByte(const i2cip_fqa_t& fqa, const uint16_t& reg, uint8_t& dest, bool resetbus = true);

    /**
     * Read two bytes of data from the device. Effectively adds one prefix bytes.
     * | MUX ADDR (7) | MUX CONFIG (8) | ACK? | DEV ADDR (7) | ACK? | DEV ADDR (7) | REG ADDR (16) | READ BYTES (8*len) | { resetbus? : | MUX ADDR (7) | MUX RESET (8) | ACK? | }
     * @param fqa FQA of the device
     * @param dest Bytes to read into
     * @param setbus Should the MUX be reset? (Default: `true`)
     **/
    static i2cip_errorlevel_t readRegisterWord(const i2cip_fqa_t& fqa, const uint8_t& reg, uint16_t& dest, bool resetbus = true);

    /**
     * Read two bytes of data from the device. Effectively adds TWO prefix bytes.
     * | MUX ADDR (7) | MUX CONFIG (8) | ACK? | DEV ADDR (7) | ACK? | DEV ADDR (7) | REG ADDR (16) | READ BYTES (8*len) | { resetbus? : | MUX ADDR (7) | MUX RESET (8) | ACK? | }
     * @param fqa FQA of the device
     * @param dest Bytes to read into
     * @param setbus Should the MUX be reset? (Default: `true`)
     **/
    static i2cip_errorlevel_t readRegisterWord(const i2cip_fqa_t& fqa, const uint16_t& reg, uint16_t& dest, bool resetbus = true);
  };

  class EEPROM : Device {
  private:
  public:
    EEPROM(const i2cip_fqa_t& fqa);

    static i2cip_errorlevel_t readContents(const i2cip_fqa_t& fqa, uint8_t* dest, size_t& num_read, size_t max_read = I2CIP_EEPROM_SIZE);

    static i2cip_errorlevel_t writeByte(const i2cip_fqa_t& fqa, const uint16_t& bytenum, const uint8_t& value, bool setbus = true);

    static i2cip_errorlevel_t clearContents(const i2cip_fqa_t& fqa, bool setbus = true, uint16_t numbytes = I2CIP_EEPROM_SIZE);

    static i2cip_errorlevel_t overwriteContents(const i2cip_fqa_t& fqa, const char* contents, bool clear = true, bool setbus = true);

    static i2cip_errorlevel_t overwriteContents(const i2cip_fqa_t& fqa, uint8_t* buffer, size_t len, bool clear = true, bool setbus = true);
  };
};

#endif