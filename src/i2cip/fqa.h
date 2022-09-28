#ifndef I2CIP_FQA_H_
#define I2CIP_FQA_H_

// --------------------------------
// FQA: Fully Qualified Addressing
// --------------------------------

// Address segments: Least Significant Bit positions and lengths
#define I2CIP_FQA_I2CBUS_LSB  13
#define I2CIP_FQA_I2CBUS_LEN  3
#define I2CIP_FQA_MUXNUM_LSB  10
#define I2CIP_FQA_MUXNUM_LEN  3
#define I2CIP_FQA_MUXBUS_LSB  7
#define I2CIP_FQA_MUXBUS_LEN  3
#define I2CIP_FQA_DEVADR_LSB  0
#define I2CIP_FQA_DEVADR_LEN  7

// Compound segments
#define I2CIP_FQA_SUBNET_LSB  7
#define I2CIP_FQA_SUBNET_LEN  9

/**
 * Segment extraction. Shifts all bits so LSB is at index 0. Returns as unsigned char.
 * @param fqa FQA of interest
 * @param lsb Right-most (least significant) bit position
 * @param len Length of the segment
 */
#define I2CIP_FQA_SEG(fqa, lsb, len) (unsigned char)((fqa >> lsb) & (0xFFFF >> (16 - len)))

// Shorthands
#define I2CIP_FQA_SEG_DEVADR(fqa) I2CIP_FQA_SEG(fqa, I2CIP_FQA_DEVADR_LSB, I2CIP_FQA_DEVADR_LEN) // Extracts the device address segment from an FQA
#define I2CIP_FQA_SEG_MUXBUS(fqa) I2CIP_FQA_SEG(fqa, I2CIP_FQA_MUXBUS_LSB, I2CIP_FQA_MUXBUS_LEN) // Extracts the MUX bus number segment from an FQA
#define I2CIP_FQA_SEG_MUXNUM(fqa) I2CIP_FQA_SEG(fqa, I2CIP_FQA_MUXNUM_LSB, I2CIP_FQA_MUXNUM_LEN) // Extracts the MUX number segment from an FQA
#define I2CIP_FQA_SEG_I2CBUS(fqa) I2CIP_FQA_SEG(fqa, I2CIP_FQA_I2CBUS_LSB, I2CIP_FQA_I2CBUS_LEN) // Extracts the I2C bus number segment from an FQA

// A useful typedef
typedef uint16_t i2cip_fqa_t;

#endif