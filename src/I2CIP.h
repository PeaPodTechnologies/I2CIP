#ifndef I2CIP_H_
#define I2CIP_H_

// HEADERS

#include <string.h>

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>

#include <i2cip/fqa.h>
#include <i2cip/mux.h>
#include <i2cip/eeprom.h>
#include <i2cip/routingtable.h>

// ---------------
// I2C Parameters
// ---------------

#define I2CIP_MAXBUFFER 32  // I2C buffer size
#define I2CIP_NUM_WIRES 1   // Number of I2C wires - TODO: autodetect and populate `wires[]` based on hardware

extern TwoWire Wire;

static TwoWire* const wires[I2CIP_NUM_WIRES] = { &Wire };

#define I2CIP_FQA_TO_WIRE(fqa) (wires[I2CIP_FQA_SEG_I2CBUS(fqa)])

// -----------------------
// I2C Intranet Protocols
// -----------------------

namespace I2CIP {
  /**
   * Errorlevels for I2CIP communication.
   */
  typedef enum {
    I2CIP_ERR_NONE, // No error
    I2CIP_ERR_SOFT, // Communications error, device still reachable
    I2CIP_ERR_HARD, // Device unreachable
  } i2cip_errorlevel_t;

  /**
   * Create an FQA from segments.
   * @param wire The index of the I2C bus this network is on.
   * @param 
   */
  i2cip_fqa_t createFQA(const uint8_t& wire, const uint8_t& mux, const uint8_t& bus, const uint8_t& addr);

  /**
   * Initialize an I2C interface (if it has not already been initialized)
   * @param fqa
   */
  void beginWire(const i2cip_fqa_t fqa);

  namespace Device {
    /**
     * Attempt to communicate with a device.
     * @param fqa FQA of the device
     * @return Hardware failure: Device and/or module lost. Software failure: Failed to switch MUX bus
     */
    i2cip_errorlevel_t ping(const i2cip_fqa_t& fqa, bool reset = true);

    /**
     * Write data to a device.
     * @param fqa FQA of the device
     * @param buffer Bytes to be sent
     * @param len Number of bytes (Default: `1`)
     * @param reset Should the MUX be reset? (Default: `true`)
     * @return Hardware failure: Device and/or module lost. Software failure: Failed to write and/or failed to switch MUX bus
     */
    i2cip_errorlevel_t write(const i2cip_fqa_t& fqa, const uint8_t* buffer, size_t len = 1, bool reset = true);

    /**
     * Write one byte to a device.
     * @param fqa FQA of the device
     * @param b Byte to be written
     * @param reset Should the MUX be reset? (Default: `true`)
     * @return Hardware failure: Device and/or module lost. Software failure: Failed to write and/or failed to switch MUX bus
     */
    i2cip_errorlevel_t write(const i2cip_fqa_t& fqa, const uint8_t& b, bool reset = true);

    /**
     * Write one byte to a specific device register.
     * @param fqa FQA of the device
     * @param reg Register address
     * @param value Byte to be written
     * @param reset Should the MUX be reset? (Default: `true`)
     * @return Hardware failure: Device and/or module lost. Software failure: Failed to write and/or failed to switch MUX bus
     */
    i2cip_errorlevel_t writeRegister(const i2cip_fqa_t& fqa, const uint8_t& reg, const uint8_t& value, bool reset = true);

    /**
     * Read data from the device.
     * @param fqa FQA of the device
     * @param buffer Bytes to read into
     * @param len Number of bytes to read (Default: `1`)
     * @param reset Should the MUX be reset? (Default: `true`)
     */
    i2cip_errorlevel_t read(const i2cip_fqa_t& fqa, uint8_t* buffer, size_t len = 1, bool reset = true);
  };

  namespace MUX {
      /**
       * Pings the MUX.
       * @param fqa FQA of a device this MUX is in front of.
       * @return Ping success?
       */
      bool pingMUX(const i2cip_fqa_t& fqa);

      /**
       * Sets the MUX bus.
       * @param fqa FQA of a device that is on the target Subnet.
       * @return Hardware failure: Module lost. Software failure: Failed to write to MUX
       */
      i2cip_errorlevel_t setBus(const i2cip_fqa_t& fqa);

      /**
       * Reset the MUX to the "inactive" bus.
       * @param fqa FQA of a device that this MUX is in front of
       */
      i2cip_errorlevel_t resetBus(const i2cip_fqa_t& fqa);
  };

  namespace Routing {

    namespace EEPROM {
      i2cip_errorlevel_t readByte(i2cip_fqa_t fqa, uint16_t bytenum, uint8_t& dest, bool reset = true);

      i2cip_errorlevel_t readContents(i2cip_fqa_t fqa, uint8_t* dest, uint16_t& num_read, uint16_t max_read = I2CIP_EEPROM_SIZE);

      // i2cip_errorlevel_t writeByte(i2cip_fqa_t fqa, uint16_t bytenum, uint8_t dest);

      // i2cip_errorlevel_t overwriteContents(i2cip_fqa_t fqa, uint8_t* newcontents, uint16_t numbytes = I2CIP_EEPROM_SIZE);
    };

    /**
     * Scans the network for modules, and allocates and builds a routing table based on SPRT EEROM.
     */
    RoutingTable* createRoutingTable(void);
  }
};

#endif