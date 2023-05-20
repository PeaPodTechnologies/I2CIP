#include <I2CIP.h>

#include <Arduino.h>
#include <Wire.h>

#include <i2cip/fqa.h>
#include <i2cip/mux.h>
#include <i2cip/eeprom.h>

// Has wire N been wires[N].begin() yet?
static bool wiresBegun[I2CIP_NUM_WIRES] = { false };

namespace I2CIP {

  i2cip_fqa_t createFQA(const uint8_t& wire, const uint8_t& mux, const uint8_t& bus, const uint8_t& addr) {
    if (( wire < I2CIP_FQA_I2CBUS_MAX ) &&
        ( mux  < I2CIP_FQA_MODULE_MAX ) &&
        ( bus  < I2CIP_FQA_MUXBUS_MAX ) &&
        ( addr < I2CIP_FQA_DEVADR_MAX )
    ) {
      return I2CIP_FQA_CREATE(wire, mux, bus, addr);
    }
    return (i2cip_fqa_t)0;
  }

  void beginWire(const uint8_t& wire) {
    if(!wiresBegun[wire]) {
      #ifdef DEBUG_SERIAL
        Serial.print("Initializing I2C wire ");
        Serial.println(wire);
      #endif
      wires[wire]->begin();
      wiresBegun[wire] = true;
    }
  }

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

        #ifdef DEBUG_SERIAL
          Serial.println("MUX Write Failed");
        #endif
      }

      // End transmission
      if (I2CIP_FQA_TO_WIRE(fqa)->endTransmission() != 0) {
        #ifdef DEBUG_SERIAL
          Serial.println("MUX Transmission Failed");
        #endif
        return I2CIP_ERR_HARD;
      }

      #ifdef DEBUG_SERIAL
        Serial.println("MUX Bus Set");
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

      #ifdef DEBUG_SERIAL
        Serial.println("MUX Bus Reset");
      #endif

      return I2CIP_ERR_NONE;
    }
  };
};