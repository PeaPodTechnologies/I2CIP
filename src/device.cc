#include <I2CIP.h>

#include <Arduino.h>

using namespace I2CIP;

// CONSTRUCTORs AND PROPERTY GETTERS/SETTERS

Device::Device(const i2cip_fqa_t& fqa) : fqa(fqa) { }

const i2cip_fqa_t& Device::getFQA(void) { return this->fqa; }

// STATIC CLASS-MEMBER FUNCTIONS (PRIVATE INTERNAL API)

i2cip_errorlevel_t Device::ping(const i2cip_fqa_t& fqa, bool resetbus) {
  // Switch MUX bus
  i2cip_errorlevel_t errlev = MUX::setBus(fqa);
  I2CIP_ERR_BREAK(errlev);

  // Begin transmission
  #ifdef DEBUG_SERIAL
    Serial.print("Ping... ");
  #endif

  I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_FQA_SEG_DEVADR(fqa));

  // End transmission, check state
  if(I2CIP_FQA_TO_WIRE(fqa)->endTransmission() != 0) {
    return I2CIP_ERR_HARD;
  }

  #ifdef DEBUG_SERIAL
    Serial.println("Pong!");
  #endif

  // Switch MUX bus back
  if (resetbus) {
    errlev = MUX::resetBus(fqa);
    I2CIP_ERR_BREAK(errlev);
  }
  
  // If we made it this far, no errors occurred.
  return I2CIP_ERR_NONE;
}

i2cip_errorlevel_t Device::pingTimeout(const i2cip_fqa_t& fqa, bool setbus, bool resetbus, unsigned int timeout) {
  if(setbus) {
    i2cip_errorlevel_t errlev = MUX::setBus(fqa);
    I2CIP_ERR_BREAK(errlev);
  }

  // Check if it's actually lost
  #ifdef DEBUG_SERIAL
    Serial.print("Ping... ");
  #endif
  
  I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_FQA_SEG_DEVADR(fqa));
  i2cip_errorlevel_t errlev = (I2CIP_FQA_TO_WIRE(fqa)->endTransmission() == 0 ? I2CIP_ERR_NONE : I2CIP_ERR_HARD);

  unsigned long start = millis();

  // Count down until out of time of found
  while (errlev != I2CIP_ERR_NONE && (millis()-start) < timeout) {
    // Delta 1ms
    delay(1);

    // Begin transmission
    #ifdef DEBUG_SERIAL
      Serial.print("Ping... ");
    #endif

    I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_FQA_SEG_DEVADR(fqa));

    // End transmission, check state
    errlev = (I2CIP_FQA_TO_WIRE(fqa)->endTransmission() == 0 ? I2CIP_ERR_NONE : I2CIP_ERR_HARD);
  }
  
  // Double check MUX before attempting to switch
  if(errlev == I2CIP_ERR_HARD && !MUX::pingMUX(fqa)) {
    return I2CIP_ERR_HARD;
  }

  // Switch MUX bus back
  if (resetbus) {
    errlev = MUX::resetBus(fqa);
  }

  #ifdef DEBUG_SERIAL
    if(errlev == I2CIP_ERR_HARD) {
      Serial.println("Timed Out!");
    } else {
      Serial.print("Pong! Ping Timeout: ");
      Serial.print(millis()-start);
      Serial.print("ms... ");
    }
  #endif
  
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
  if (I2CIP_FQA_TO_WIRE(fqa)->endTransmission() != 0) {
    return I2CIP_ERR_HARD;
  }

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
    #ifdef DEBUG_SERIAL
      Serial.print("Write Failed (");
      Serial.print(sent);
      Serial.print("/");
      Serial.print(len);
      Serial.println(" bytes sent)");
    #endif
  }

  // End transmission
  if (I2CIP_FQA_TO_WIRE(fqa)->endTransmission() != 0) {
    #ifdef DEBUG_SERIAL
      Serial.print("No ACK On Write! ");
    #endif
    return I2CIP_ERR_HARD;
  }

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
  uint8_t buff[len + 1] = { reg };
  for(size_t i = 0; i < len; i++) {
    buff[i + 1] = buffer[i];
  }
  return write(fqa, buff, len + 1, setbus);
}

i2cip_errorlevel_t Device::writeRegister(const i2cip_fqa_t& fqa, const uint16_t& reg, uint8_t* buffer, size_t len, bool setbus) {
  uint8_t buff[len + 2] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF) };
  for(size_t i = 0; i < len; i++) {
    buff[i + 2] = buffer[i];
  }
  return write(fqa, buff, len + 2, setbus);
}

i2cip_errorlevel_t Device::read(const i2cip_fqa_t& fqa, uint8_t* dest, size_t& len, bool nullterminate, bool resetbus) {
  // Device alive?
  i2cip_errorlevel_t errlev = ping(fqa, false);
  I2CIP_ERR_BREAK(errlev);

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
    if (recv != read_len) {
      success = false;
      len = pos;
      break;
    }

    // Read in all the bytes
    for (uint16_t i = 0; i < read_len; i++) {
      dest[pos + i] = I2CIP_FQA_TO_WIRE(fqa)->read();
      if(nullterminate && dest[pos + i] == '\0') {
        len = pos + i;
        goto endloop0;
      }
    }
    
    // Advance the index by the amount of bytes read
    pos += read_len;
  }
endloop0:

  // Reset MUX bus if `reset` == true
  if (resetbus) {
    errlev = MUX::resetBus(fqa);
    I2CIP_ERR_BREAK(errlev);
  }

  // Did we read all the bytes we hoped to?
  return (success ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT);
}

i2cip_errorlevel_t Device::readByte(const i2cip_fqa_t& fqa, uint8_t& dest, bool resetbus) {
  size_t len = 1;
  return read(fqa, &dest, len, resetbus);
}

i2cip_errorlevel_t Device::readWord(const i2cip_fqa_t& fqa, uint16_t& dest, bool resetbus) {
  size_t len = 2;
  uint8_t buff[2];
  i2cip_errorlevel_t errlev = read(fqa, buff, len, resetbus);
  I2CIP_ERR_BREAK(errlev);
  dest = ((uint16_t)buff[1] << 8) | (uint16_t)buff[0];
  return errlev;
}

i2cip_errorlevel_t Device::readRegisterByte(const i2cip_fqa_t& fqa, const uint8_t& reg, uint8_t& dest, bool resetbus) {
  size_t len = 1;
  return readRegister(fqa, reg, &dest, len, false, resetbus);
}

i2cip_errorlevel_t Device::readRegisterByte(const i2cip_fqa_t& fqa, const uint16_t& reg, uint8_t& dest, bool resetbus) {
  size_t len = 1;
  return readRegister(fqa, reg, &dest, len, false, resetbus);
}

i2cip_errorlevel_t Device::readRegisterWord(const i2cip_fqa_t& fqa, const uint8_t& reg, uint16_t& dest, bool resetbus) {
  size_t len = 2;
  uint8_t buff[2];
  i2cip_errorlevel_t errlev = readRegister(fqa, reg, buff, len, false, resetbus);
  I2CIP_ERR_BREAK(errlev);
  dest = ((uint16_t)buff[1] << 8) | (uint16_t)buff[0];
  return errlev;
}

i2cip_errorlevel_t Device::readRegisterWord(const i2cip_fqa_t& fqa, const uint16_t& reg, uint16_t& dest, bool resetbus) {
  size_t len = 2;
  uint8_t buff[2];
  i2cip_errorlevel_t errlev = readRegister(fqa, reg, buff, len, false, resetbus);
  I2CIP_ERR_BREAK(errlev);
  dest = ((uint16_t)buff[1] << 8) | (uint16_t)buff[0];
  return errlev;
}

i2cip_errorlevel_t Device::readRegister(const i2cip_fqa_t& fqa, const uint8_t& reg, uint8_t* dest, size_t& len, bool nullterminate, bool resetbus) {
  // Device alive?
  i2cip_errorlevel_t errlev = ping(fqa, false);
  I2CIP_ERR_BREAK(errlev);

  // Read in chunks (buffer size limitation)
  size_t pos = 0;
  bool success = true;
  while (pos < len) {
    // Read whichever is greater: number of bytes remaining, or buffer size
    uint8_t read_len = ((len - pos) > I2CIP_MAXBUFFER) ? I2CIP_MAXBUFFER : (len - pos);

    // Don't stop the bus unless we've read everything
    bool read_stop = (pos + read_len >= len);

    // Request bytes; How many have we received?
    size_t recv = I2CIP_FQA_TO_WIRE(fqa)->requestFrom(I2CIP_FQA_SEG_DEVADR(fqa), read_len, reg, 1, (uint8_t)read_stop);
    
    // We didn't get all the bytes we expected
    if (recv != read_len) {
      success = false;
      len = pos;
      break;
    }

    // Read in all the bytes
    for (uint16_t i = 0; i < read_len; i++) {
      dest[pos + i] = I2CIP_FQA_TO_WIRE(fqa)->read();
      if(nullterminate && dest[pos + i] == '\0') {
        len = pos + i;
        goto endloop1;
      }
    }
    
    // Advance the index by the amount of bytes read
    pos += read_len;
  }
endloop1:

  // Reset MUX bus if `reset` == true
  if (resetbus) {
    errlev = MUX::resetBus(fqa);
    I2CIP_ERR_BREAK(errlev);
  }

  // Did we read all the bytes we hoped to?
  return (success ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT);
}

i2cip_errorlevel_t Device::readRegister(const i2cip_fqa_t& fqa, const uint16_t& reg, uint8_t* dest, size_t& len, bool nullterminate, bool resetbus) {
  // Device alive?
  i2cip_errorlevel_t errlev = ping(fqa, false);
  I2CIP_ERR_BREAK(errlev);

  // Read in chunks (buffer size limitation)
  size_t pos = 0;
  bool success = true;
  while (pos < len) {
    // Read whichever is greater: number of bytes remaining, or buffer size
    uint8_t read_len = min(len - pos, I2CIP_MAXBUFFER);

    // Don't stop the bus unless we've read everything
    bool read_stop = (pos + read_len >= len);

    #ifdef DEBUG_SERIAL
      Serial.print("Reading bytes ");
      Serial.print(pos);
      Serial.print(" - ");
      Serial.print(pos+read_len);
      Serial.print(": '");
    #endif

    // Request bytes; How many have we received?
    size_t recv = I2CIP_FQA_TO_WIRE(fqa)->requestFrom(I2CIP_FQA_SEG_DEVADR(fqa), read_len, reg, 2, (uint8_t)read_stop);
    
    // We didn't get all the bytes we expected
    if (recv != read_len) {
      success = false;
    }

    // Read in all the bytes received
    for (uint16_t i = 0; i < recv; i++) {
      dest[pos + i] = I2CIP_FQA_TO_WIRE(fqa)->read();
      #ifdef DEBUG_SERIAL
        Serial.print((char)dest[pos + i]);
      #endif
      if(nullterminate && dest[pos + i] == '\0') {
        len = pos + i;
        goto endloop2;
      }
    }
    #ifdef DEBUG_SERIAL
      Serial.println("'");
    #endif
    
    // Advance the index by the amount of bytes received
    pos += recv;
  }
endloop2:
  #ifdef DEBUG_SERIAL
    Serial.println("'");
  #endif

  // Reset MUX bus if `reset` == true
  if (resetbus) {
    errlev = MUX::resetBus(fqa);
    I2CIP_ERR_BREAK(errlev);
  }

  // Did we read all the bytes we hoped to?
  return (success ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT);
}

// NON-STATIC OBJECT-MEMBER FUNCTIONS (PUBLIC EXTERNAL API)

i2cip_errorlevel_t Device::ping(bool resetbus) { return Device::ping(this->fqa, resetbus); }
i2cip_errorlevel_t Device::pingTimeout(bool setbus, bool resetbus, unsigned int timeout) { return Device::pingTimeout(this->fqa, setbus, resetbus, timeout); }
i2cip_errorlevel_t Device::writeByte(const uint8_t& value, bool setbus)  { return Device::writeByte(this->fqa, value, setbus); }
i2cip_errorlevel_t Device::write(const uint8_t* buffer, size_t len, bool setbus) { return Device::write(this->fqa, buffer, len, setbus); }
i2cip_errorlevel_t Device::writeRegister(const uint8_t& reg, const uint8_t& value, bool setbus) { return Device::writeRegister(this->fqa, reg, value, setbus); }
i2cip_errorlevel_t Device::writeRegister(const uint16_t& reg, const uint8_t& value, bool setbus) { return Device::writeRegister(this->fqa, reg, value, setbus); }
i2cip_errorlevel_t Device::writeRegister(const uint8_t& reg, uint8_t* buffer, size_t len, bool setbus) { return Device::writeRegister(this->fqa, reg, buffer, len, setbus); }
i2cip_errorlevel_t Device::writeRegister(const uint16_t& reg, uint8_t* buffer, size_t len, bool setbus) { return Device::writeRegister(this->fqa, reg, buffer, len, setbus); }
i2cip_errorlevel_t Device::read(uint8_t* dest, size_t& len, bool nullterminate, bool resetbus) { return Device::read(this->fqa, dest, len, nullterminate, resetbus); }
i2cip_errorlevel_t Device::readByte(uint8_t& dest, bool resetbus) { return Device::readByte(this->fqa, dest, resetbus); }
i2cip_errorlevel_t Device::readWord(uint16_t& dest, bool resetbus) { return Device::readWord(this->fqa, dest, resetbus); }
i2cip_errorlevel_t Device::readRegister(const uint8_t& reg, uint8_t* dest, size_t& len, bool nullterminate, bool resetbus) { return Device::readRegister(this->fqa, reg, dest, len, nullterminate, resetbus); }
i2cip_errorlevel_t Device::readRegister(const uint16_t& reg, uint8_t* dest, size_t& len, bool nullterminate, bool resetbus) { return Device::readRegister(this->fqa, reg, dest, len, nullterminate, resetbus); }
i2cip_errorlevel_t Device::readRegisterByte(const uint8_t& reg, uint8_t& dest, bool resetbus) { return Device::readRegisterByte(this->fqa, reg, dest, resetbus); }
i2cip_errorlevel_t Device::readRegisterByte(const uint16_t& reg, uint8_t& dest, bool resetbus) { return Device::readRegisterByte(this->fqa, reg, dest, resetbus); }
i2cip_errorlevel_t Device::readRegisterWord(const uint8_t& reg, uint16_t& dest, bool resetbus) { return Device::readRegisterWord(this->fqa, reg, dest, resetbus);  }
i2cip_errorlevel_t Device::readRegisterWord(const uint16_t& reg, uint16_t& dest, bool resetbus) { return Device::readRegisterWord(this->fqa, reg, dest, resetbus); }