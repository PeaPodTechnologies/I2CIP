#include <device.h>

#include <Wire.h>

#include <fqa.h>
#include <mux.h>
#include <debug.h>

using namespace I2CIP;

uint8_t requestFromRegister(i2cip_fqa_t fqa, uint8_t len, uint8_t reg, bool sendStop = true) {
  // send internal address; this mode allows sending a repeated start to access
  // some devices' internal registers. This function is executed by the hardware
  // TWI module on other processors (for example Due's TWI_IADR and TWI_MMR registers)

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("REQUEST "));
    I2CIP_DEBUG_SERIAL.print(len);
    I2CIP_DEBUG_SERIAL.print(F(" BYTES FROM 0x"));
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(F(" REG(1) 0x"));
    I2CIP_DEBUG_SERIAL.print(reg, HEX);
    DEBUG_DELAY();
  #endif

  I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_FQA_SEG_DEVADR(fqa));

  // write internal register address - most significant byte first
  if(I2CIP_FQA_TO_WIRE(fqa)->write(reg) != 1) return 0;
  
  if(I2CIP_FQA_TO_WIRE(fqa)->endTransmission(sendStop) != 0) return 0;

  if(sendStop) delayMicroseconds(10); // delay for some devices (one frame at standard 100kHz)

  uint8_t r = I2CIP_FQA_TO_WIRE(fqa)->requestFrom(I2CIP_FQA_SEG_DEVADR(fqa), (uint8_t)len, sendStop ? (uint8_t)1 : (uint8_t)0);
  #ifdef I2CIP_DEBUG_SERIAL
  if(r == 0) {
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.println(F(" FAIL"));
    DEBUG_DELAY();
  } else {
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F(" PASS RX["));
    I2CIP_DEBUG_SERIAL.print(r);
    I2CIP_DEBUG_SERIAL.println(F("]"));
    DEBUG_DELAY();
  }
  #endif
  return r;
}

uint8_t requestFromRegister(i2cip_fqa_t fqa, uint8_t len, uint16_t reg, bool sendStop = true) {
  // send internal address; this mode allows sending a repeated start to access
  // some devices' internal registers. This function is executed by the hardware
  // TWI module on other processors (for example Due's TWI_IADR and TWI_MMR registers)

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("REQUEST "));
    I2CIP_DEBUG_SERIAL.print(len);
    I2CIP_DEBUG_SERIAL.print(F(" BYTES FROM 0x"));
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(F(" REG(2) @0x"));
    I2CIP_DEBUG_SERIAL.print(reg, HEX);
    DEBUG_DELAY();
  #endif

  I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_FQA_SEG_DEVADR(fqa));

  // write internal register address - most significant byte first
  if(I2CIP_FQA_TO_WIRE(fqa)->write((uint8_t)((reg >> 8) & 0xFF)) != 1) return 0;
  if(I2CIP_FQA_TO_WIRE(fqa)->write((uint8_t)(reg & 0xFF)) != 1) return 0;
  
  if(I2CIP_FQA_TO_WIRE(fqa)->endTransmission(sendStop) != 0) return 0;

  if(sendStop) delayMicroseconds(10); // delay for some devices (one frame at standard 100kHz)

  // return I2CIP_FQA_TO_WIRE(fqa)->requestFrom(I2CIP_FQA_SEG_DEVADR(fqa), (uint8_t)len, sendStop ? (uint8_t)1 : (uint8_t)0);
  uint8_t r = I2CIP_FQA_TO_WIRE(fqa)->requestFrom(I2CIP_FQA_SEG_DEVADR(fqa), (uint8_t)len, sendStop ? (uint8_t)1 : (uint8_t)0);
  #ifdef I2CIP_DEBUG_SERIAL
  if(r == 0) {
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.println(F(" FAIL"));
    DEBUG_DELAY();
  } else {
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F(" PASS RX["));
    I2CIP_DEBUG_SERIAL.print(r);
    I2CIP_DEBUG_SERIAL.println(F("]"));
    DEBUG_DELAY();
  }
  #endif
  return r;
}

// CONSTRUCTORs AND PROPERTY GETTERS/SETTERS

Device::Device(i2cip_fqa_t fqa, i2cip_id_t id) : fqa(fqa), id(id) { }

Device::~Device() { 
  // #ifdef I2CIP_DEBUG_SERIAL
  //   DEBUG_DELAY();
  //   I2CIP_DEBUG_SERIAL.print(F("~Device\n"));
  //   DEBUG_DELAY();
  // #endif
}

void Device::setInput(InputGetter* input) { this->input = input; }
void Device::setOutput(OutputSetter* output) { this->output = output; }

void Device::removeInput(void) { this->input = nullptr; }
void Device::removeOutput(void) { this->output = nullptr; }

InputGetter* Device::getInput(void) const { return this->input; }
OutputSetter* Device::getOutput(void) const { return this->output; }

const char InputGetter::failptr_get;
const char OutputSetter::failptr_set;

InputGetter::~InputGetter() { 
  // #ifdef I2CIP_DEBUG_SERIAL
  //   DEBUG_DELAY();
  //   I2CIP_DEBUG_SERIAL.print(F("~InputGetter"));
  //   DEBUG_DELAY();
  // #endif
}
OutputSetter::~OutputSetter() {
  // #ifdef I2CIP_DEBUG_SERIAL
  //   DEBUG_DELAY();
  //   I2CIP_DEBUG_SERIAL.print(F("~OutputSetter"));
  //   DEBUG_DELAY();
  // #endif
}

// i2cip_errorlevel_t Device::get(const void* args) { return (this->getInput() == nullptr) ? I2CIP_ERR_SOFT : this->input->get(args); }
// i2cip_errorlevel_t Device::set(const void* value, const void* args) { return (this->output == nullptr) ? I2CIP_ERR_SOFT : this->output->set(value, args); }
i2cip_errorlevel_t Device::get(const void* args) { return (this->getInput() == nullptr) ? I2CIP_ERR_HARD : this->input->get(args); } // TODO: Should this be NOP/NONE? or are you clearly doing something wrong
i2cip_errorlevel_t Device::set(const void* value, const void* args) { return (this->getOutput() == nullptr) ? I2CIP_ERR_HARD : this->output->set(value, args); } // TODO: Should this be NOP/NONE? or are you clearly doing something wrong

const i2cip_fqa_t& Device::getFQA(void) const { return this->fqa; }

// i2cip_id_t Device::getID(void) const { return this->id; }
const i2cip_id_t& Device::getID(void) const { return this->id; }

// STATIC CLASS-MEMBER FUNCTIONS (PRIVATE INTERNAL API)

i2cip_errorlevel_t Device::ping(const i2cip_fqa_t& fqa, bool resetbus, bool setbus) {
  // Switch MUX bus
  i2cip_errorlevel_t errlev;
  if(setbus) {
    errlev = MUX::setBus(fqa);
    I2CIP_ERR_BREAK(errlev);
  }

  // Begin transmission
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("-> Device 0x"));
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(F(" Ping... "));
  #endif

  I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_FQA_SEG_DEVADR(fqa));

  // End transmission, check state
  if(I2CIP_FQA_TO_WIRE(fqa)->endTransmission(true) != 0) {
    return I2CIP_ERR_HARD;
  }

  #ifdef I2CIP_DEBUG_SERIAL
    I2CIP_DEBUG_SERIAL.println("Pong!");
    DEBUG_DELAY();
  #endif
  
  // If we made it this far, no errors occurred.
  return resetbus ? I2CIP_ERR_NONE : MUX::resetBus(fqa);
}

i2cip_errorlevel_t Device::pingTimeout(const i2cip_fqa_t& fqa, bool setbus, bool resetbus, unsigned int timeout) {

  i2cip_errorlevel_t errlev = setbus ? MUX::setBus(fqa) : I2CIP_ERR_NONE;
  I2CIP_ERR_BREAK(errlev);

  // Check if it's actually lost

  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("-> Device 0x"));
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(F(" Ping... "));
  #endif
  
  I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_FQA_SEG_DEVADR(fqa));
  errlev = (I2CIP_FQA_TO_WIRE(fqa)->endTransmission(true) == 0 ? I2CIP_ERR_NONE : I2CIP_ERR_HARD);
  if(errlev == I2CIP_ERR_NONE) {
    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.println(F("Pong!"));
    #endif
    return resetbus ? MUX::resetBus(fqa) : I2CIP_ERR_NONE;
  }
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("LOST; Interval -> Ping... "));
  #endif

  unsigned long start = millis();

  // Count down until out of time of found
  while (errlev != I2CIP_ERR_NONE) {
    if (millis() - start > timeout) {
      errlev = I2CIP_ERR_HARD;
      break;
    }
    #ifndef I2CIP_DEBUG_SERIAL
      // Delta 0.1ms
      delayMicroseconds(100);
    #endif

    // Begin transmission
    I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_FQA_SEG_DEVADR(fqa));

    // End transmission, check state
    errlev = (I2CIP_FQA_TO_WIRE(fqa)->endTransmission(true) == 0 ? I2CIP_ERR_NONE : I2CIP_ERR_HARD);

    if (errlev == I2CIP_ERR_NONE) {
      // Default case/quick-break
      break;
    }

    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("Ping... "));
    #endif
  }
  
  // Double check MUX before attempting to switch
  if(errlev == I2CIP_ERR_SOFT && !MUX::pingMUX(fqa)) {
    errlev = I2CIP_ERR_HARD;
  }

  #ifdef I2CIP_DEBUG_SERIAL
    if(errlev == I2CIP_ERR_HARD) {
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.println("Lost!");
      DEBUG_DELAY();
    } else if (errlev == I2CIP_ERR_SOFT) {
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.println("Failed!");
      DEBUG_DELAY();
    }else {
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("Pong! Timeout: "));
      I2CIP_DEBUG_SERIAL.print(millis()-start);
      I2CIP_DEBUG_SERIAL.print(F("ms\n"));
      DEBUG_DELAY();
    }
  #endif

  // Switch MUX bus back
  if (resetbus) {
    if(errlev == I2CIP_ERR_NONE) {
      // Default case
      return MUX::resetBus(fqa);
    } else if(MUX::resetBus(fqa) == I2CIP_ERR_NONE) {
      return errlev;
    } else {
      return I2CIP_ERR_HARD;
    }
  }
  
  return errlev;
}

i2cip_errorlevel_t Device::writeByte(const i2cip_fqa_t& fqa, const uint8_t& value, bool setbus) {
  i2cip_errorlevel_t errlev;
  if (setbus) {
    // Switch MUX bus
    i2cip_errorlevel_t errlev = MUX::setBus(fqa);
    I2CIP_ERR_BREAK(errlev);
  }

  // Was the write performed successfully?
  bool success = true;

  // Begin transmission
  I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_FQA_SEG_DEVADR(fqa));

  // Write the buffer
  if (I2CIP_FQA_TO_WIRE(fqa)->write(value) != 1) {
    success = false;
  }

  // End transmission
  if (I2CIP_FQA_TO_WIRE(fqa)->endTransmission(true) != 0) {
    return I2CIP_ERR_HARD;
  }
  #ifdef I2CIP_DEBUG_SERIAL
    else if(success) {
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("TX "));
      I2CIP_DEBUG_SERIAL.println(value, HEX);
      DEBUG_DELAY();
    }
  #endif

  // Reset MUX bus if `reset` == true
  if (setbus) {
    errlev = MUX::resetBus(fqa);
    I2CIP_ERR_BREAK(errlev);
  }

  return (success ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT);
}

i2cip_errorlevel_t Device::write(const i2cip_fqa_t& fqa, const uint8_t* buffer, size_t len, bool setbus) {
  i2cip_errorlevel_t errlev = I2CIP_ERR_NONE;
  if (setbus) {
    errlev = MUX::setBus(fqa);
    I2CIP_ERR_BREAK(errlev);
  }

  // Was the write performed successfully?
  bool success = true;

  // Begin transmission
  I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_FQA_SEG_DEVADR(fqa));

  // Write the buffer
  size_t sent = I2CIP_FQA_TO_WIRE(fqa)->write(buffer, len);
  if (sent != len) {
    success = false;
    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("Write Failed ("));
      I2CIP_DEBUG_SERIAL.print(sent);
      I2CIP_DEBUG_SERIAL.print(F("/"));
      I2CIP_DEBUG_SERIAL.print(len);
      I2CIP_DEBUG_SERIAL.print(F(" bytes sent)\n"));
      DEBUG_DELAY();
    #endif
  }

  // End transmission
  if (I2CIP_FQA_TO_WIRE(fqa)->endTransmission(true) != 0) {
    #ifdef I2CIP_DEBUG_SERIAL
      I2CIP_DEBUG_SERIAL.print(F("No ACK On Write! "));
      DEBUG_DELAY();
    #endif
    return I2CIP_ERR_HARD;
  }
  #ifdef I2CIP_DEBUG_SERIAL
    // else if(success) {
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(fqaToString(fqa));
      I2CIP_DEBUG_SERIAL.print(F(" TX ["));
      I2CIP_DEBUG_SERIAL.print(len);
      I2CIP_DEBUG_SERIAL.print(F("] "));
      for (size_t i = 0; i < len; i++) {
        I2CIP_DEBUG_SERIAL.print(buffer[i], HEX);
        I2CIP_DEBUG_SERIAL.print(F(" "));
      }
      I2CIP_DEBUG_SERIAL.println();
      DEBUG_DELAY();
    // }
  #endif

  // Reset MUX bus if `reset` == true
  if (setbus) {
    errlev = MUX::resetBus(fqa);
  }

  return ((success || errlev > I2CIP_ERR_NONE) ? errlev : I2CIP_ERR_SOFT);
}

i2cip_errorlevel_t Device::writeRegister(const i2cip_fqa_t& fqa, const uint8_t& reg, const uint8_t& value, bool setbus) {
  const uint8_t buf[2] = { reg, value };
  return write(fqa, buf, 2, setbus);
}

i2cip_errorlevel_t Device::writeRegister(const i2cip_fqa_t& fqa, const uint16_t& reg, const uint8_t& value, bool setbus) {
  const uint8_t buf[3] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF), value };
  return write(fqa, buf, 3, setbus);
}

i2cip_errorlevel_t Device::writeRegister(const i2cip_fqa_t& fqa, const uint8_t& reg, uint8_t* buffer, size_t len, bool setbus) {
  if(len < 0) return I2CIP_ERR_SOFT;
  uint8_t buff[len + 1] = { reg };
  for(size_t i = 0; i < len; i++) {
    buff[i + 1] = buffer[i];
  }
  return write(fqa, buff, len + 1, setbus);
}

i2cip_errorlevel_t Device::writeRegister(const i2cip_fqa_t& fqa, const uint16_t& reg, uint8_t* buffer, size_t len, bool setbus) {
  if(len < 0) return I2CIP_ERR_SOFT;
  uint8_t buff[len + 2] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF) };
  for(size_t i = 0; i < len; i++) {
    buff[i + 2] = buffer[i];
  }
  return write(fqa, buff, len + 2, setbus);
}

i2cip_errorlevel_t Device::read(const i2cip_fqa_t& fqa, uint8_t* dest, size_t& len, bool nullterminate, bool resetbus, bool setbus) {
  // Device alive?
  i2cip_errorlevel_t errlev;
  if (setbus) {
    errlev = MUX::setBus(fqa);
    I2CIP_ERR_BREAK(errlev);
  }

  // Read in chunks (buffer size limitation)
  size_t pos = 0;
  bool success = true;
  while (pos < len) {
    // Read whichever is greater: number of bytes remaining, or buffer size
    uint8_t read_len = ((len - pos) > I2CIP_MAXBUFFER) ? I2CIP_MAXBUFFER : (len - pos);

    // Don't stop the bus unless we've read everything
    bool read_stop = (pos + read_len >= len);

    // Request bytes; How many have we received?
    size_t recv = I2CIP_FQA_TO_WIRE(fqa)->requestFrom(I2CIP_FQA_SEG_DEVADR(fqa), read_len, (uint8_t)read_stop);
    
    // We didn't get all the bytes we expected
    // if (recv != read_len) {
    //   success = false;
    //   len = pos;
    //   break;
    // }
    if (recv != read_len) {
      success = false;
      len = pos + recv;
    }
    if(recv == 0) {
      goto endloop0;
    }

    delayMicroseconds(100);

    // Read in all the bytes
    for (uint16_t i = 0; i < read_len; i++) {
      dest[pos + i] = I2CIP_FQA_TO_WIRE(fqa)->read();
      if(nullterminate && dest[pos + i] == '\0') {
        len = pos + i;
        goto endloop0;
      }
    }
    
    // Advance the index by the amount of bytes read
    pos += recv;
  }
endloop0:

  #ifdef I2CIP_DEBUG_SERIAL
    // if(success) {
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("RX["));
      I2CIP_DEBUG_SERIAL.print(len);
      I2CIP_DEBUG_SERIAL.print(F("] "));
      for (size_t i = 0; i < len; i++) {
        I2CIP_DEBUG_SERIAL.print(dest[i], HEX);
        I2CIP_DEBUG_SERIAL.print(F(" "));
      }
      I2CIP_DEBUG_SERIAL.println();
      DEBUG_DELAY();
    // }
  #endif

  // Reset MUX bus if `reset` == true
  if (resetbus) {
    errlev = MUX::resetBus(fqa);
    I2CIP_ERR_BREAK(errlev);
  }

  // Did we read all the bytes we hoped to?
  return (success ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT);
}

i2cip_errorlevel_t Device::readByte(const i2cip_fqa_t& fqa, uint8_t& dest, bool resetbus, bool setbus) {
  size_t len = 1;
  i2cip_errorlevel_t errlev = read(fqa, &dest, len, resetbus, setbus);
  return errlev == I2CIP_ERR_NONE ? (len == 2 ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT) : errlev;
}

i2cip_errorlevel_t Device::readWord(const i2cip_fqa_t& fqa, uint16_t& dest, bool resetbus, bool setbus) {
  size_t len = 2;
  uint8_t buff[2];
  i2cip_errorlevel_t errlev = read(fqa, buff, len, resetbus, setbus);
  I2CIP_ERR_BREAK(errlev);
  dest = ((uint16_t)buff[1] << 8) | (uint16_t)buff[0];
  return errlev == I2CIP_ERR_NONE ? (len == 2 ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT) : errlev;
}

i2cip_errorlevel_t Device::readRegisterByte(const i2cip_fqa_t& fqa, const uint8_t& reg, uint8_t& dest, bool resetbus, bool setbus) {
  size_t len = 1;
  i2cip_errorlevel_t errlev = readRegister(fqa, reg, &dest, len, false, resetbus, setbus);
  return errlev == I2CIP_ERR_NONE ? (len == 1 ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT) : errlev;
}

i2cip_errorlevel_t Device::readRegisterByte(const i2cip_fqa_t& fqa, const uint16_t& reg, uint8_t& dest, bool resetbus, bool setbus) {
  size_t len = 1;
  i2cip_errorlevel_t errlev = readRegister(fqa, reg, &dest, len, false, resetbus, setbus);
  return errlev == I2CIP_ERR_NONE ? (len == 1 ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT) : errlev;
}

i2cip_errorlevel_t Device::readRegisterWord(const i2cip_fqa_t& fqa, const uint8_t& reg, uint16_t& dest, bool resetbus, bool setbus) {
  size_t len = 2;
  uint8_t buff[2];
  i2cip_errorlevel_t errlev = readRegister(fqa, reg, buff, len, false, resetbus, setbus);
  I2CIP_ERR_BREAK(errlev);
  dest = ((uint16_t)buff[1] << 8) | (uint16_t)buff[0];
  return errlev == I2CIP_ERR_NONE ? (len == 2 ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT) : errlev;
}

i2cip_errorlevel_t Device::readRegisterWord(const i2cip_fqa_t& fqa, const uint16_t& reg, uint16_t& dest, bool resetbus, bool setbus) {
  size_t len = 2;
  uint8_t buff[2];
  i2cip_errorlevel_t errlev = readRegister(fqa, reg, buff, len, false, resetbus, setbus);
  I2CIP_ERR_BREAK(errlev);
  dest = ((uint16_t)buff[1] << 8) | (uint16_t)buff[0];
  return errlev == I2CIP_ERR_NONE ? (len == 2 ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT) : errlev;
}

i2cip_errorlevel_t Device::readRegister(const i2cip_fqa_t& fqa, const uint8_t& reg, uint8_t* dest, size_t& len, bool nullterminate, bool resetbus, bool setbus) {
  // Device alive?
  i2cip_errorlevel_t errlev;
  if (setbus) {
    errlev = MUX::setBus(fqa);
    I2CIP_ERR_BREAK(errlev);
  }

  // Read in chunks (buffer size limitation)
  size_t pos = 0;
  bool success = true;
  while (pos < len) {
    // Read whichever is greater: number of bytes remaining, or buffer size
    uint8_t read_len = ((len - pos) > I2CIP_MAXBUFFER) ? I2CIP_MAXBUFFER : (len - pos);

    // Don't stop the bus unless we've read everything
    bool read_stop = (pos + read_len >= len);

    // Request bytes; How many have we received?
    size_t recv = requestFromRegister(fqa, read_len, reg, read_stop);
    // size_t recv = I2CIP_FQA_TO_WIRE(fqa)->requestFrom(I2CIP_FQA_SEG_DEVADR(fqa), read_len));
    
    // We didn't get all the bytes we expected
    // if (recv != read_len) {
    //   success = false;
    //   len = pos;
    //   break;
    // }

    if (recv != read_len) {
      success = false;
      len = pos + recv;
    }
    if(recv == 0) {
      goto endloop1;
    }

    delayMicroseconds(100);

    // Read in all the bytes
    for (uint16_t i = 0; i < read_len; i++) {
      dest[pos + i] = I2CIP_FQA_TO_WIRE(fqa)->read();
      if(nullterminate && dest[pos + i] == '\0') {
        len = pos + i;
        goto endloop1;
      }
    }
    
    // Advance the index by the amount of bytes read
    pos += recv;
  }
endloop1:

  #ifdef I2CIP_DEBUG_SERIAL
    // if(success) {
      DEBUG_DELAY();
      for (size_t i = 0; i < len; i++) {
        I2CIP_DEBUG_SERIAL.print((uint8_t)dest[i], HEX);
        I2CIP_DEBUG_SERIAL.print(F(" "));
      }
      I2CIP_DEBUG_SERIAL.println();
      DEBUG_DELAY();
    // }
  #endif

  // Reset MUX bus if `reset` == true
  if (resetbus) {
    errlev = MUX::resetBus(fqa);
    I2CIP_ERR_BREAK(errlev);
  } else if(!success && !MUX::pingMUX(fqa)) {
    return I2CIP_ERR_HARD;
  }

  // Did we read all the bytes we hoped to?
  return (success ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT);
}

i2cip_errorlevel_t Device::readRegister(const i2cip_fqa_t& fqa, const uint16_t& reg, uint8_t* dest, size_t& len, bool nullterminate, bool resetbus, bool setbus) {
  // Device alive?
  i2cip_errorlevel_t errlev;
  if (setbus) {
    errlev = MUX::setBus(fqa);
    I2CIP_ERR_BREAK(errlev);
  }

  // Read in chunks (buffer size limitation)
  size_t pos = 0;
  bool success = true;
  while (pos < len) {
    // Read whichever is greater: number of bytes remaining, or buffer size
    uint8_t read_len = min((int)(len - pos), I2CIP_MAXBUFFER);

    // Don't stop the bus unless we've read everything
    bool read_stop = (pos + read_len >= len);

    // #ifdef I2CIP_DEBUG_SERIAL
    //   DEBUG_DELAY();
    //   I2CIP_DEBUG_SERIAL.print(F("Reading bytes "));
    //   I2CIP_DEBUG_SERIAL.print(pos);
    //   I2CIP_DEBUG_SERIAL.print(F(" - "));
    //   I2CIP_DEBUG_SERIAL.print(pos+read_len);
    //   I2CIP_DEBUG_SERIAL.print(F(": '"));
    //   DEBUG_DELAY();
    // #endif

    // Request bytes; How many have we received?
    size_t recv = requestFromRegister(fqa, read_len, reg, read_stop);
    // size_t recv = I2CIP_FQA_TO_WIRE(fqa)->requestFrom(I2CIP_FQA_SEG_DEVADR(fqa), read_len, reg, 2, (uint8_t)read_stop);
    
    // We didn't get all the bytes we expected
    if (recv != read_len) {
      success = false;
      len = pos + recv;
    }

    if(recv == 0) {
      goto endloop2;
    }

    delayMicroseconds(100);

    // Read in all the bytes received
    for (uint16_t i = 0; i < recv; i++) {
      dest[pos + i] = I2CIP_FQA_TO_WIRE(fqa)->read();
      if(nullterminate && dest[pos + i] == '\0') {
        len = pos + i;
        goto endloop2;
      }
    }
    
    // Advance the index by the amount of bytes received
    pos += recv;
  }
endloop2:
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    for (size_t i = 0; i < len; i++) {
      I2CIP_DEBUG_SERIAL.print((uint8_t)dest[i], HEX);
      I2CIP_DEBUG_SERIAL.print(F(" "));
    }
    I2CIP_DEBUG_SERIAL.println();
    DEBUG_DELAY();
  #endif

  // Reset MUX bus if `reset` == true
  if (resetbus) {
    errlev = MUX::resetBus(fqa);
    I2CIP_ERR_BREAK(errlev);
  } else if(!success && !MUX::pingMUX(fqa)) {
    return I2CIP_ERR_HARD;
  }

  // Did we read all the bytes we hoped to?
  return (success ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT);
}

// NON-STATIC OBJECT-MEMBER FUNCTIONS (PUBLIC EXTERNAL API)

i2cip_errorlevel_t Device::ping(bool resetbus, bool setbus) { return Device::ping(this->fqa, resetbus, setbus); }
i2cip_errorlevel_t Device::pingTimeout(bool setbus, bool resetbus, unsigned int timeout) { return Device::pingTimeout(this->fqa, setbus, resetbus, timeout); }
i2cip_errorlevel_t Device::writeByte(const uint8_t& value, bool setbus)  { return Device::writeByte(this->fqa, value, setbus); }
i2cip_errorlevel_t Device::write(const uint8_t* buffer, size_t len, bool setbus) { return Device::write(this->fqa, buffer, len, setbus); }
i2cip_errorlevel_t Device::writeRegister(const uint8_t& reg, const uint8_t& value, bool setbus) { return Device::writeRegister(this->fqa, reg, value, setbus); }
i2cip_errorlevel_t Device::writeRegister(const uint16_t& reg, const uint8_t& value, bool setbus) { return Device::writeRegister(this->fqa, reg, value, setbus); }
i2cip_errorlevel_t Device::writeRegister(const uint8_t& reg, uint8_t* buffer, size_t len, bool setbus) { return Device::writeRegister(this->fqa, reg, buffer, len, setbus); }
i2cip_errorlevel_t Device::writeRegister(const uint16_t& reg, uint8_t* buffer, size_t len, bool setbus) { return Device::writeRegister(this->fqa, reg, buffer, len, setbus); }
i2cip_errorlevel_t Device::read(uint8_t* dest, size_t& len, bool nullterminate, bool resetbus, bool setbus) { return Device::read(this->fqa, dest, len, nullterminate, resetbus, setbus); }
i2cip_errorlevel_t Device::readByte(uint8_t& dest, bool resetbus, bool setbus) { return Device::readByte(this->fqa, dest, resetbus, setbus); }
i2cip_errorlevel_t Device::readWord(uint16_t& dest, bool resetbus, bool setbus) { return Device::readWord(this->fqa, dest, resetbus, setbus); }
i2cip_errorlevel_t Device::readRegister(const uint8_t& reg, uint8_t* dest, size_t& len, bool nullterminate, bool resetbus, bool setbus) { return Device::readRegister(this->fqa, reg, dest, len, nullterminate, resetbus, setbus); }
i2cip_errorlevel_t Device::readRegister(const uint16_t& reg, uint8_t* dest, size_t& len, bool nullterminate, bool resetbus, bool setbus) { return Device::readRegister(this->fqa, reg, dest, len, nullterminate, resetbus, setbus); }
i2cip_errorlevel_t Device::readRegisterByte(const uint8_t& reg, uint8_t& dest, bool resetbus, bool setbus) { return Device::readRegisterByte(this->fqa, reg, dest, resetbus, setbus); }
i2cip_errorlevel_t Device::readRegisterByte(const uint16_t& reg, uint8_t& dest, bool resetbus, bool setbus) { return Device::readRegisterByte(this->fqa, reg, dest, resetbus, setbus); }
i2cip_errorlevel_t Device::readRegisterWord(const uint8_t& reg, uint16_t& dest, bool resetbus, bool setbus) { return Device::readRegisterWord(this->fqa, reg, dest, resetbus, setbus);  }
i2cip_errorlevel_t Device::readRegisterWord(const uint16_t& reg, uint16_t& dest, bool resetbus, bool setbus) { return Device::readRegisterWord(this->fqa, reg, dest, resetbus, setbus); }
