#ifndef I2CIP_MUX_H_
#define I2CIP_MUX_H_

// ----------------------
// MUX: I2C Multiplexers
// ----------------------

#define I2CIP_MUX_ADDR_MIN      0x70 // The lowest device address
#define I2CIP_MUX_ADDR_MAX      0x77 // The highest device address
#define I2CIP_MUX_COUNT         (I2CIP_MUX_ADDR_MAX - I2CIP_MUX_ADDR_MIN + 1)
#define I2CIP_MUX_BUS_MIN       0x00 // The lowest bus number
#define I2CIP_MUX_BUS_MAX       0x07 // The highest bus number
#define I2CIP_MUX_BUS_DEFAULT   0x00 // The default bus for "base" interfaces.
#define I2CIP_MUX_INSTR_RST     0x00 // Disable all busses

/**
 * Converts a bus number to a MUX instruction.
 * @param bus Bus number (0-7)
 */
#define I2CIP_MUX_BUS_TO_INSTR(bus) (uint8_t)(1 << bus)

/**
 * Converts a module number to its MUX address.
 * @param num MUX number (0-7)
 */
#define I2CIP_MODULE_TO_MUXADDR(module) (module + I2CIP_MUX_ADDR_MIN)

#endif