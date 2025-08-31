#ifndef I2CIP_DEBUG_SERIAL

#include <Arduino.h>

#ifndef DEBUG
// Uncomment to enable debug
// #define DEBUG 1
#endif

// CROSS-LIBRARY DEBUG COMPATIBILITY
#ifdef DEBUG
#if DEBUG == true
#ifndef DEBUG_SERIAL
#define DEBUG_SERIAL Serial
#endif
#endif
#endif

// #include <DebugJson.h>

#ifdef DEBUG_SERIAL
// Just once
#define I2CIP_DEBUG_SERIAL DebugJsonBreakpoints
// #define I2CIP_DEBUG_SERIAL DEBUG_SERIAL

#ifndef DEBUG_DELAY
#define DEBUG_DELAY() {delayMicroseconds(10);}
#endif
#endif

#ifndef _F
#ifdef DEBUG_DISABLE_FSTRINGS
#if DEBUG_DISABLE_FSTRINGS == 1
#define _F(x) x
#endif
#endif
#define _F(x) F(x)
#endif

#endif