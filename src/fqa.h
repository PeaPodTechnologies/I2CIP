#ifndef I2CIP_FQA_H_
#define I2CIP_FQA_H_

#include <Arduino.h>
#include "Wire.h"

// --------------------------------
// FQA: Fully Qualified Addressing
// --------------------------------
// FQA is a 16-bit address space for I2C devices on a modular switched network of I2C buses.
// The FQA is a 16-bit number that encodes the I2C bus number, MUX number, MUX bus number, and device address.

// 0. Two useful typedefs
typedef uint16_t i2cip_fqa_t;
typedef const char* i2cip_id_t;

// 1. Address segments: Least Significant Bit positions, lengths, and maximum values
#define I2CIP_FQA_I2CBUS_LSB  13
#define I2CIP_FQA_I2CBUS_LEN  3
#define I2CIP_FQA_I2CBUS_MAX  0b111
#define I2CIP_FQA_MODULE_LSB  10
#define I2CIP_FQA_MODULE_LEN  3
#define I2CIP_FQA_MODULE_MAX  0b111
#define I2CIP_FQA_MUXBUS_LSB  7
#define I2CIP_FQA_MUXBUS_LEN  3
#define I2CIP_FQA_MUXBUS_MAX  0b111
#define I2CIP_FQA_DEVADR_LSB  0
#define I2CIP_FQA_DEVADR_LEN  7
#define I2CIP_FQA_DEVADR_MAX  0b1111111 // 7-bit address

/** 2.
 * FQA creation. Bit-shift and OR; no validation.
 * @param wire I2C bus number
 * @param module MUX number
 * @param bus MUX bus number
 * @param addr Device address
*/
#define I2CIP_FQA_CREATE(wire, module, bus, addr) (i2cip_fqa_t)((wire << (16 - I2CIP_FQA_I2CBUS_LEN)) | (module << (16 - I2CIP_FQA_I2CBUS_LEN - I2CIP_FQA_MODULE_LEN)) | (bus << (16 - I2CIP_FQA_I2CBUS_LEN - I2CIP_FQA_MODULE_LEN - I2CIP_FQA_MUXBUS_LEN)) | addr)

/** 3.
 * Segment extraction. Right-shift LSB and AND len-mask.
 * @param fqa FQA of interest
 * @param lsb Right-most (least significant) bit position
 * @param len Length of the segment
 */
#define I2CIP_FQA_SEG(fqa, lsb, len) ((uint8_t)((fqa >> lsb) & (0xFFFF >> (16 - len))))

// Segment Extraction Shorthands
#define I2CIP_FQA_SEG_DEVADR(fqa) I2CIP_FQA_SEG(fqa, I2CIP_FQA_DEVADR_LSB, I2CIP_FQA_DEVADR_LEN) // Extracts the device address segment from an FQA
#define I2CIP_FQA_SEG_MUXBUS(fqa) I2CIP_FQA_SEG(fqa, I2CIP_FQA_MUXBUS_LSB, I2CIP_FQA_MUXBUS_LEN) // Extracts the MUX bus number segment from an FQA
#define I2CIP_FQA_SEG_MODULE(fqa) I2CIP_FQA_SEG(fqa, I2CIP_FQA_MODULE_LSB, I2CIP_FQA_MODULE_LEN) // Extracts the MUX number segment from an FQA
#define I2CIP_FQA_SEG_I2CBUS(fqa) I2CIP_FQA_SEG(fqa, I2CIP_FQA_I2CBUS_LSB, I2CIP_FQA_I2CBUS_LEN) // Extracts the I2C bus number segment from an FQA

// 4. Faked-out MUX and Bus numbers - these trigger NO-MUX behaviour
#define I2CIP_MUX_BUS_FAKE 0x07 // Bus 7 fakeout for devices that are not on a MUX - Easter Egg
#define I2CIP_MUX_NUM_FAKE 0x07 // Module MUX 0x77 fakeout-mask i.e. for HT16K33 - Easter Egg

// 5. I2C Wire Implementation
#define I2CIP_MAXBUFFER 32  // I2C buffer size
#define I2CIP_NUM_WIRES 2   // Number of I2C wires - TODO: autodetect and populate `wires[]` based on hardware spec macros

extern TwoWire Wire; // Implemented in Wire.c

#if I2CIP_NUM_WIRES == 0
  #error "I2CIP_NUM_WIRES must be greater than 0"
#elif I2CIP_NUM_WIRES == 1
  static TwoWire* const wires[I2CIP_NUM_WIRES] = { &Wire }; // Array of I2C wires
#elif I2CIP_NUM_WIRES == 2 // Use Wire1 as well
  extern TwoWire Wire1; // Implemented in Wire.c
  static TwoWire* const wires[I2CIP_NUM_WIRES] = { &Wire, &Wire1 }; // Array of I2C wires
#endif

extern bool wiresBegun[]; // Has wire N been wires[N].begin() yet?

#define I2CIP_FQA_TO_WIRE(fqa) (wires[I2CIP_FQA_SEG_I2CBUS(fqa)])
#define I2CIP_WIRES(i) (wires[i])

// 6. Debugging
#define I2CIP_ERR_BREAK(errlev) if((errlev) != I2CIP::I2CIP_ERR_NONE) { return (errlev); }

namespace I2CIP {
  /**
   * Errorlevels for I2CIP communication.
   * @enum NONE No error
   * @enum SOFT Communications or other error, device may still be reachable
   * @enum HARD Device unreachable (No ACK)
   */
  typedef enum {
    I2CIP_ERR_NONE = 0x0,
    I2CIP_ERR_SOFT = 0x1,
    I2CIP_ERR_HARD = 0x2,
  } i2cip_errorlevel_t;
  
  /**
   * Create an FQA from segments with validation.
   * @param wire I2C bus number
   * @param mux MUX number
   * @param bus MUX bus number
   * @param addr Device address
   * @return A valid FQA, or 0xFFFF if any segment has an invalid value.
   */
  i2cip_fqa_t createFQA(uint8_t wire, uint8_t mux, uint8_t bus, uint8_t addr);

  /**
   * Initialize an I2C interface. Store the result in wiresBegun[wire].
   * @param wire
   * @return success
   */
  bool beginWire(uint8_t wire);

  /**
   * Convert an FQA to a string.
   * @param fqa FQA to convert
   * @return String representation of the FQA in the format `I2C[{wire}]:{mux}:{bus}:0x{addr}`
   */
  String fqaToString(const i2cip_fqa_t& fqa);
};

#endif

