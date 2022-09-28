#ifndef I2CIP_H_
#define I2CIP_H_

// HEADERS

#include <string.h>

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>

#include <i2cip/fqa.h>
#include <i2cip/mux.h>
#include <i2cip/routingtable.h>

// ---------------
// I2C Parameters
// ---------------

#define I2CIP_MAXBUFFER   32
#define I2CIP_EEPROM_SIZE 4000
#define I2CIP_EEPROM_ADDR 0x50

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
  i2cip_fqa_t createFQA(const unsigned char& wire, const unsigned char& mux, const unsigned char& bus, const unsigned char& addr);

  namespace Device {
    /**
     * Attempt to communicate with a device.
     * @param fqa FQA of the device
     * @return Hardware failure: Device and/or module lost. Software failure: Failed to switch MUX bus
     */
    i2cip_errorlevel_t ping(const i2cip_fqa_t& fqa);

    /**
     * Write data to a device.
     * @param fqa FQA of the device
     * @param buffer Bytes to be sent
     * @param len Number of bytes (Default: `1`)
     * @param reset Should the MUX be reset? (Default: `true`)
     * @return Hardware failure: Device and/or module lost. Software failure: Failed to write and/or failed to switch MUX bus
     */
    i2cip_errorlevel_t write(const i2cip_fqa_t& fqa, const unsigned char* buffer, size_t len = 1, bool reset = true);

    /**
     * Write one byte to a device.
     * @param fqa FQA of the device
     * @param b Byte to be written
     * @param reset Should the MUX be reset? (Default: `true`)
     * @return Hardware failure: Device and/or module lost. Software failure: Failed to write and/or failed to switch MUX bus
     */
    i2cip_errorlevel_t write(const i2cip_fqa_t& fqa, const unsigned char& b, bool reset = true);

    /**
     * Write one byte to a specific device register.
     * @param fqa FQA of the device
     * @param reg Register address
     * @param value Byte to be written
     * @param reset Should the MUX be reset? (Default: `true`)
     * @return Hardware failure: Device and/or module lost. Software failure: Failed to write and/or failed to switch MUX bus
     */
    i2cip_errorlevel_t writeRegister(const i2cip_fqa_t& fqa, const unsigned char& reg, const unsigned char& value, bool reset = true);

    /**
     * Read data from the device.
     * @param fqa FQA of the device
     * @param buffer Bytes to read into
     * @param len Number of bytes to read (Default: `1`)
     * @param reset Should the MUX be reset? (Default: `true`)
     */
    i2cip_errorlevel_t read(const i2cip_fqa_t& fqa, unsigned char* buffer, size_t len = 1, bool reset = true);
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
    /**
     * Scans the network for modules, and allocates and builds a routing table based on SPRT EEROM.
     */
    RoutingTable* createRoutingTable(void);
  }
};

#endif