#ifndef I2CIP_DEBUG_H_
#define I2CIP_DEBUG_H_

#include <Arduino.h>

// Uncomment to enable debug
#define DEBUG_SERIAL Serial

// CROSS-LIBRARY DEBUG COMPATIBILITY
#ifdef DEBUG_SERIAL
#ifdef I2CIP_DEBUG_SERIAL
#undef I2CIP_DEBUG_SERIAL
#endif
#define I2CIP_DEBUG_SERIAL DEBUG_SERIAL
#endif

#ifdef DEBUG
#if DEBUG == true
#ifndef I2CIP_DEBUG_SERIAL
#define I2CIP_DEBUG_SERIAL Serial
#endif
#endif
#endif

#ifdef I2CIP_DEBUG_SERIAL
#ifndef DEBUG_DELAY
#define DEBUG_DELAY() {delay(1);}
#endif
#endif

#endif