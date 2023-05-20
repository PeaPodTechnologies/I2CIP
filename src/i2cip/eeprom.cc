#include <I2CIP.h>

using namespace I2CIP;

EEPROM::EEPROM(const i2cip_fqa_t& fqa) : Device(fqa) { }

EEPROM::EEPROM(const uint8_t& wire, const uint8_t& module) : Device(createFQA(wire, module, I2CIP_MUX_BUS_DEFAULT, I2CIP_EEPROM_ADDR)) { }

i2cip_errorlevel_t EEPROM::readContents(uint8_t* dest, size_t& num_read, size_t max_read) {
  size_t bytes_read = max_read;
  i2cip_errorlevel_t errlev = Device::readRegister(fqa, (uint16_t)0, dest, bytes_read, true, false);
  num_read = bytes_read;
  return errlev;
}

i2cip_errorlevel_t EEPROM::clearContents(bool setbus, uint16_t numbytes) {
  i2cip_errorlevel_t errlev = pingTimeout(setbus, false);
  I2CIP_ERR_BREAK(errlev);

  uint8_t zeroes[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  for (uint16_t bytes_written = 0; bytes_written < numbytes; bytes_written+=8) {
    const uint8_t pagelen = min(numbytes - bytes_written, 8);
    
    errlev = writeRegister(bytes_written, zeroes, pagelen, false);
    if(errlev == I2CIP_ERR_SOFT) {
      // Actual failed write
      return errlev;
    }

    #ifdef DEBUG_SERIAL
      Serial.print("Cleared EEPROM bytes ");
      Serial.print(bytes_written);
      Serial.print(" - ");
      Serial.println((bytes_written + pagelen - 1));
    #endif

    // Note: Timeout ping before each byte write to await completion of last write cycle
    errlev = pingTimeout(false, false);
    I2CIP_ERR_BREAK(errlev);
  }

  return errlev;
}

i2cip_errorlevel_t EEPROM::overwriteContents(const char* contents, bool clear, bool setbus) {
  for(size_t i = 0; i < I2CIP_EEPROM_SIZE; i++) {
    if(contents[i] == '\0') {
      return overwriteContents((uint8_t*)contents, i, clear, setbus);
    }
  }
  return I2CIP_ERR_SOFT;
}

i2cip_errorlevel_t EEPROM::overwriteContents(uint8_t* buffer, size_t len, bool clear, bool setbus) {
  i2cip_errorlevel_t errlev = I2CIP_ERR_NONE;
  if(clear) {
    errlev = clearContents(setbus, len);
  } else if(setbus) {
    errlev = MUX::setBus(this->fqa);
  }
  I2CIP_ERR_BREAK(errlev);

  for (uint16_t bytes_written = 0; bytes_written < len; bytes_written+=8) {
    const uint8_t pagelen = min(len - bytes_written, 8);
    errlev = writeRegister(bytes_written, buffer+bytes_written, pagelen, false);
    if(errlev == I2CIP_ERR_SOFT) {
      // Actual failed write
      return errlev;
    }

    #ifdef DEBUG_SERIAL
      Serial.print("Bytes ");
      Serial.print(bytes_written);
      Serial.print(" - ");
      Serial.print(bytes_written + pagelen);
      Serial.print(" written '");
      for(int i = 0; i < pagelen; i++) {
        Serial.print((char)((buffer+bytes_written)[i]));
      }
      Serial.print("'... ");
    #endif

    // Note: Timeout ping before each byte write to await completion of last write cycle
    errlev = pingTimeout(false, false);
    I2CIP_ERR_BREAK(errlev);

    #ifdef DEBUG_SERIAL
      Serial.println("... ACK'd");
    #endif

    if(buffer[bytes_written] == '\0') {
      // Null terminator
      #ifdef DEBUG_SERIAL
        Serial.println("Overwrite stopped - null terminator");
      #endif
      break;
    }
  }

  return errlev;
}