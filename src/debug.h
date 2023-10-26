#ifndef I2CIP_DEBUG_SERIAL

#include <Arduino.h>

// Uncomment to enable debug
// #define DEBUG 1

// CROSS-LIBRARY DEBUG COMPATIBILITY
#ifdef DEBUG
#if DEBUG == true
#ifndef DEBUG_SERIAL
#define DEBUG_SERIAL Serial
#endif
#endif
#endif

#ifdef DEBUG_SERIAL
// Just once
#define I2CIP_DEBUG_SERIAL DEBUG_SERIAL

#ifndef DEBUG_DELAY
#define DEBUG_DELAY() {delay(10);}
#endif
#endif

#endif