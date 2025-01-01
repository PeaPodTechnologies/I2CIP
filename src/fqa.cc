#include "fqa.h"

#include "debug.h"

// Has wire N been wires[N].begin() yet?
bool wiresBegun[I2CIP_NUM_WIRES] = { false };

i2cip_fqa_t I2CIP::createFQA(uint8_t wire, uint8_t mux, uint8_t bus, uint8_t addr) {
  if (( wire <= I2CIP_FQA_I2CBUS_MAX ) &&
      ( mux  <= I2CIP_FQA_MODULE_MAX ) &&
      ( bus  <= I2CIP_FQA_MUXBUS_MAX ) &&
      ( addr <= I2CIP_FQA_DEVADR_MAX )
  ) {
    return I2CIP_FQA_CREATE(wire, mux, bus, addr);
  }
  return (i2cip_fqa_t)(~0);
}

void I2CIP::beginWire(uint8_t wire) {
  wires[wire]->begin();
  if(!wiresBegun[wire]) {
    wiresBegun[wire] = true;
    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("-> I2C WIRE "));
      I2CIP_DEBUG_SERIAL.print(wire);
      I2CIP_DEBUG_SERIAL.println(F(" BEGIN"));
      DEBUG_DELAY();
    #endif
  }
}

String I2CIP::fqaToString(const i2cip_fqa_t& fqa) {
  String s = F("I2C[");
  s += String(I2CIP_FQA_SEG_I2CBUS(fqa), HEX);
  s += F("]:");
  if(I2CIP_FQA_SEG_MODULE(fqa) == I2CIP_MUX_NUM_FAKE || I2CIP_FQA_SEG_MUXBUS(fqa) == I2CIP_MUX_BUS_FAKE) {
    // s += F(" NOMUX ");
  } else {
    // s += F(" Subnet ");
    s += String(I2CIP_FQA_SEG_MODULE(fqa), HEX);
    s += ':';
    s += String(I2CIP_FQA_SEG_MUXBUS(fqa), HEX);
    s += ':';
  }
  s += F("0x");
  s += String(I2CIP_FQA_SEG_DEVADR(fqa) & 0x7F, HEX);
  return s;
}