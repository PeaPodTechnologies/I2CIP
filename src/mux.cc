#include <mux.h>

#include <fqa.h>
#include <debug.h>

namespace I2CIP {

  namespace MUX {
    bool pingMUX(const uint8_t& wire, const uint8_t& module) {
      beginWire(wire);
      wires[wire]->beginTransmission(I2CIP_MODULE_TO_MUXADDR(module));
      return (wires[wire]->endTransmission() == 0);
    }

    bool pingMUX(const i2cip_fqa_t& fqa) {
      beginWire(I2CIP_FQA_SEG_I2CBUS(fqa));
      I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_MODULE_TO_MUXADDR(I2CIP_FQA_SEG_MODULE(fqa)));
      return (I2CIP_FQA_TO_WIRE(fqa)->endTransmission() == 0);
    }
    
    i2cip_errorlevel_t setBus(const i2cip_fqa_t& fqa) {
      // Note: no need to ping MUX, we'll see in real time what the result is
      beginWire(I2CIP_FQA_SEG_I2CBUS(fqa));

      // Was the bus switched successfully?
      bool success = true;

      // Begin transmission
      I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_MODULE_TO_MUXADDR(I2CIP_FQA_SEG_MODULE(fqa)));

      // Write the bus switch instruction
      uint8_t instruction = I2CIP_MUX_BUS_TO_INSTR(I2CIP_FQA_SEG_MUXBUS(fqa));
      if (I2CIP_FQA_TO_WIRE(fqa)->write(&instruction, 1) != 1) {
        success = false;

        #ifdef I2CIP_DEBUG_SERIAL
          DEBUG_DELAY();
          I2CIP_DEBUG_SERIAL.print(F("MUX Write Failed\n"));
        #endif
      }

      // End transmission
      if (I2CIP_FQA_TO_WIRE(fqa)->endTransmission() != 0) {
        #ifdef I2CIP_DEBUG_SERIAL
          I2CIP_DEBUG_SERIAL.print(F("MUX Transmission Failed\n"));
          DEBUG_DELAY();
        #endif
        return I2CIP_ERR_HARD;
      }

      #ifdef I2CIP_DEBUG_SERIAL
        I2CIP_DEBUG_SERIAL.print(F("MUX Bus Set\n"));
        DEBUG_DELAY();
      #endif

      return (success ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT);
    }

    i2cip_errorlevel_t resetBus(const i2cip_fqa_t& fqa) {
      // Note: no need to ping MUX, we'll see in real time what the result is
      beginWire(I2CIP_FQA_SEG_I2CBUS(fqa));

      // Begin transmission
      I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_MODULE_TO_MUXADDR(I2CIP_FQA_SEG_MODULE(fqa)));

      // Write the "inactive" bus switch instruction
      const uint8_t instruction = I2CIP_MUX_INSTR_RST;
      if (I2CIP_FQA_TO_WIRE(fqa)->write(&instruction, 1) != 1) {
        return I2CIP_ERR_SOFT;
      }

      // End transmission
      if (I2CIP_FQA_TO_WIRE(fqa)->endTransmission() != 0) {
        return I2CIP_ERR_HARD;
      }

      #ifdef I2CIP_DEBUG_SERIAL
        DEBUG_DELAY();
        I2CIP_DEBUG_SERIAL.print(F("MUX Bus Reset\n"));
        DEBUG_DELAY();
      #endif

      return I2CIP_ERR_NONE;
    }
  };
};