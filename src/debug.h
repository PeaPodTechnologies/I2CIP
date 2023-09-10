#ifndef I2CIP_DEBUG_H_
#define I2CIP_DEBUG_H_

// CROSS-LIBRARY DEBUG COMPATIBILITY
#ifdef DEBUG_SERIAL
#define I2CIP_DEBUG_SERIAL DEBUG_SERIAL
#endif

#ifdef DEBUG
#if DEBUG == true
#define I2CIP_DEBUG_SERIAL Serial
#endif
#endif

#endif