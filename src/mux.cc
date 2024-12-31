#include <mux.h>

#include <fqa.h>
#include <debug.h>

// #define I2CIP_DEBUG_SERIAL Serial // just this once
#ifndef DEBUG_DELAY
#define DEBUG_DELAY() {delayMicroseconds(2);}
#endif

// It's ok to have this globally bc it's a microcontroller
bool _busses_reset = false;
I2CIP::i2cip_errorlevel_t resetBusses(uint8_t wire) {
  I2CIP::i2cip_errorlevel_t errlev = I2CIP::I2CIP_ERR_NONE;
  for(uint8_t m = 0; m < I2CIP_MUX_COUNT; m++) {
    I2CIP::i2cip_errorlevel_t err = I2CIP::MUX::resetBus(wire, m);
    if(err > errlev) { errlev = err; } 
  }
  _busses_reset = true;
  return errlev;
}

#ifdef I2CIP_MUX_BUS_FAKE
#ifdef I2CIP_DEBUG_SERIAL
#define FAKEBUS_BREAK(fqa) {\
  if(I2CIP_FQA_SEG_MUXBUS(fqa) == I2CIP_MUX_BUS_FAKE) {\
    DEBUG_DELAY();\
    I2CIP_DEBUG_SERIAL.println(F("--> FAKE BUS; MUX NOP"));\
    DEBUG_DELAY();\
    return resetBusses(I2CIP_FQA_SEG_I2CBUS(fqa));\
  }\
}
#else
#define FAKEBUS_BREAK(fqa) { if(I2CIP_FQA_SEG_MUXBUS(fqa) == I2CIP_MUX_BUS_FAKE) { return resetBusses(I2CIP_FQA_SEG_WIRE(fqa)); } }
#endif
#endif

#ifdef I2CIP_MUX_NUM_FAKE
#ifdef I2CIP_DEBUG_SERIAL
#define FAKEMUX_BREAK(fqa) {\
  if(I2CIP_FQA_SEG_MODULE(fqa) == I2CIP_MUX_NUM_FAKE) {\
    DEBUG_DELAY();\
    I2CIP_DEBUG_SERIAL.println(F("--> FAKE MUX; NOP"));\
    DEBUG_DELAY();\
    return resetBusses(I2CIP_FQA_SEG_I2CBUS(fqa));\
  }\
}
#else
#define FAKEMUX_BREAK(fqa) { if(I2CIP_FQA_SEG_MODULE(fqa) == I2CIP_MUX_NUM_FAKE) { resetBusses(I2CIP_FQA_SEG_WIRE(fqa)); } }
#endif
#endif

namespace I2CIP {

  namespace MUX {
    bool pingMUX(const uint8_t& wire, const uint8_t& m) {
      if(wire > I2CIP_NUM_WIRES) return false;
      if(m > I2CIP_MUX_COUNT) return false;
      beginWire(wire);
      #ifdef I2CIP_DEBUG_SERIAL
        I2CIP_DEBUG_SERIAL.print(F("-> MUX "));
        I2CIP_DEBUG_SERIAL.print(m, HEX);
        I2CIP_DEBUG_SERIAL.print(F(" PING WRITE {"));
        I2CIP_DEBUG_SERIAL.print(I2CIP_MODULE_TO_MUXADDR(m), HEX);
        I2CIP_DEBUG_SERIAL.print(F("}... "));
      #endif
      wires[wire]->beginTransmission(I2CIP_MODULE_TO_MUXADDR(m));
      // return (wires[wire]->endTransmission() == 0);
      bool r = (wires[wire]->endTransmission(true) == 0);
      #ifdef I2CIP_DEBUG_SERIAL
        if(r) {
          DEBUG_DELAY();
          I2CIP_DEBUG_SERIAL.println(F("PONG!"));
          DEBUG_DELAY();
        } else {
          DEBUG_DELAY();
          I2CIP_DEBUG_SERIAL.println(F("FAIL!"));
          DEBUG_DELAY();
        }
      #endif
      return r;
    }

    bool pingMUX(const i2cip_fqa_t& fqa) {
      #ifdef I2CIP_MUX_BUS_FAKE
        FAKEBUS_BREAK(fqa);
      #endif
      #ifdef I2CIP_MUX_NUM_FAKE
        FAKEMUX_BREAK(fqa);
      #endif
      return pingMUX(I2CIP_FQA_SEG_I2CBUS(fqa), I2CIP_FQA_SEG_MODULE(fqa));
    }
    
    i2cip_errorlevel_t setBus(const i2cip_fqa_t& fqa) { return setBus(I2CIP_FQA_SEG_I2CBUS(fqa), I2CIP_FQA_SEG_MODULE(fqa), I2CIP_FQA_SEG_MUXBUS(fqa)); }
    
    i2cip_errorlevel_t setBus(const uint8_t& wire, const uint8_t& m, const uint8_t& bus) {
      // Note: no need to ping MUX, we'll see in real time what the result is
      beginWire(wire);

      _busses_reset = false;

      i2cip_fqa_t nofqa = createFQA(wire, m, bus, 0x00);

      #ifdef I2CIP_MUX_BUS_FAKE
        FAKEBUS_BREAK(nofqa);
      #endif

      #ifdef I2CIP_MUX_NUM_FAKE
        FAKEMUX_BREAK(nofqa);
      #endif

      #ifdef I2CIP_DEBUG_SERIAL
        I2CIP_DEBUG_SERIAL.print(F("-> MUX "));
        I2CIP_DEBUG_SERIAL.print(m, HEX);
        I2CIP_DEBUG_SERIAL.print(F(" SET BUS "));
        I2CIP_DEBUG_SERIAL.print(bus, BIN);
        I2CIP_DEBUG_SERIAL.print(F(" WRITE {0x"));
        I2CIP_DEBUG_SERIAL.print(I2CIP_MODULE_TO_MUXADDR(m), HEX);
        I2CIP_DEBUG_SERIAL.print(F(", 0b"));
        I2CIP_DEBUG_SERIAL.print(I2CIP_MUX_BUS_TO_INSTR(bus), BIN);
        I2CIP_DEBUG_SERIAL.print(F("} "));
      #endif

      // Was the bus switched successfully?
      bool success = true;

      // Begin transmission
      I2CIP_WIRES(wire)->beginTransmission(I2CIP_MODULE_TO_MUXADDR(m));

      // Write the bus switch instruction
      uint8_t instruction = I2CIP_MUX_BUS_TO_INSTR(bus);
      if (I2CIP_WIRES(wire)->write(&instruction, 1) != 1) {
        success = false;

        #ifdef I2CIP_DEBUG_SERIAL
          DEBUG_DELAY();
          I2CIP_DEBUG_SERIAL.println(F("FAIL EINVAL"));
        #endif
      }

      // End transmission
      if (I2CIP_WIRES(wire)->endTransmission(true) != 0) {
        #ifdef I2CIP_DEBUG_SERIAL
          I2CIP_DEBUG_SERIAL.println(F("FAIL EIO"));
          DEBUG_DELAY();
        #endif
        return I2CIP_ERR_HARD;
      }

      #ifdef I2CIP_DEBUG_SERIAL
        I2CIP_DEBUG_SERIAL.println(F("PASS"));
        DEBUG_DELAY();
      #endif

      return (success ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT);
    }

    i2cip_errorlevel_t resetBus(const i2cip_fqa_t& fqa) { 
      #ifdef I2CIP_MUX_BUS_FAKE
        FAKEBUS_BREAK(fqa);
      #endif
      return resetBus(I2CIP_FQA_SEG_I2CBUS(fqa), I2CIP_FQA_SEG_MODULE(fqa));
    }
    
    i2cip_errorlevel_t resetBus(const uint8_t& wire, const uint8_t& m) {
      // Note: no need to ping MUX, we'll see in real time what the result is
      beginWire(wire);

      // if(I2CIP_FQA_SEG_MUXBUS(fqa) == I2CIP_MUX_BUS_MAX) {
      //   // Fell for the ol fake bus cantrip
      //   #ifdef I2CIP_DEBUG_SERIAL
      //     DEBUG_DELAY();
      //     I2CIP_DEBUG_SERIAL.println(F("FAKE BUS NOP! This device exists independent of any MUX, directly on the I2C bus."));
      //     DEBUG_DELAY();
      //   #endif
      //   return I2CIP_ERR_NONE;
      // }

      i2cip_fqa_t nofqa = createFQA(wire, m, I2CIP_MUX_BUS_DEFAULT, 0x00);

      #ifdef I2CIP_MUX_NUM_FAKE
        FAKEMUX_BREAK(nofqa);
      #endif

      #ifdef I2CIP_DEBUG_SERIAL
        I2CIP_DEBUG_SERIAL.print(F("-> MUX "));
        I2CIP_DEBUG_SERIAL.print(m, HEX);
        I2CIP_DEBUG_SERIAL.print(F(" RESET BUS WRITE {0x"));
        I2CIP_DEBUG_SERIAL.print(I2CIP_MODULE_TO_MUXADDR(m), HEX);
        I2CIP_DEBUG_SERIAL.print(F(", 0b"));
        I2CIP_DEBUG_SERIAL.print(I2CIP_MUX_INSTR_RST, BIN);
        I2CIP_DEBUG_SERIAL.print(F("} "));
      #endif

      // Begin transmission
      I2CIP_WIRES(wire)->beginTransmission(I2CIP_MODULE_TO_MUXADDR(m));

      // Write the "inactive" bus switch instruction
      const uint8_t instruction = I2CIP_MUX_INSTR_RST;
      if (I2CIP_WIRES(wire)->write(&instruction, 1) != 1) {
        #ifdef I2CIP_DEBUG_SERIAL
          I2CIP_DEBUG_SERIAL.println(F("FAIL EINVAL"));
        #endif
        return I2CIP_ERR_SOFT;
      }

      // End transmission
      if (I2CIP_WIRES(wire)->endTransmission(true) != 0) {
        #ifdef I2CIP_DEBUG_SERIAL
          I2CIP_DEBUG_SERIAL.println(F("FAIL EIO"));
        #endif
        return I2CIP_ERR_HARD;
      }

      #ifdef I2CIP_DEBUG_SERIAL
        I2CIP_DEBUG_SERIAL.println(F("PASS"));
      #endif

      return I2CIP_ERR_NONE;
    }
  };
};