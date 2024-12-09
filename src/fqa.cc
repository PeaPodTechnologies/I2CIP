#include <fqa.h>

#include <debug.h>

// Has wire N been wires[N].begin() yet?
#ifndef fqa_member_guard
bool I2CIP::wiresBegun[I2CIP_NUM_WIRES] = { false };
TwoWire* const I2CIP::wires[I2CIP_NUM_WIRES] = { &Wire };
#define fqa_member_guard
#endif

i2cip_fqa_t I2CIP::createFQA(const uint8_t& wire, const uint8_t& mux, const uint8_t& bus, const uint8_t& addr) {
  if (( wire < I2CIP_FQA_I2CBUS_MAX ) &&
      ( mux  < I2CIP_FQA_MODULE_MAX ) &&
      ( bus  < I2CIP_FQA_MUXBUS_MAX ) &&
      ( addr < I2CIP_FQA_DEVADR_MAX )
  ) {
    return I2CIP_FQA_CREATE(wire, mux, bus, addr);
  }
  return (i2cip_fqa_t)0;
}

void I2CIP::beginWire(const uint8_t& wire) {
  if(!wiresBegun[wire]) {
    I2CIP::wires[wire]->begin();
    I2CIP::wiresBegun[wire] = true;
    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("Initializing I2C wire "));
      I2CIP_DEBUG_SERIAL.println(wire);
      DEBUG_DELAY();
    #endif
  }
}