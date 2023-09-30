#include <eeprom.h>

#include <fqa.h>
#include <device.h>

const char* csos_id_eeprom = "eeprom";

using namespace I2CIP;

EEPROM::EEPROM(const i2cip_fqa_t& fqa) : Device(fqa, csos_id_eeprom) {
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print("EEPROM Constructed (0x");
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(")\n");
    DEBUG_DELAY();
  #endif
}

EEPROM::EEPROM(const uint8_t& wire, const uint8_t& module, const uint8_t& addr) : Device(createFQA(wire, module, I2CIP_MUX_BUS_DEFAULT, addr), csos_id_eeprom) {
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print("EEPROM Constructed (0x");
    I2CIP_DEBUG_SERIAL.print(addr, HEX);
    I2CIP_DEBUG_SERIAL.print(")\n");
    DEBUG_DELAY();
  #endif
}

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

    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print("Cleared EEPROM bytes ");
      I2CIP_DEBUG_SERIAL.print(bytes_written);
      I2CIP_DEBUG_SERIAL.print(" - ");
      I2CIP_DEBUG_SERIAL.println((bytes_written + pagelen - 1));
      DEBUG_DELAY();
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

    #ifdef I2CIP_DEBUG_SERIAL
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print("Bytes ");
      I2CIP_DEBUG_SERIAL.print(bytes_written);
      I2CIP_DEBUG_SERIAL.print(" - ");
      I2CIP_DEBUG_SERIAL.print(bytes_written + pagelen);
      I2CIP_DEBUG_SERIAL.print(" written '");
      for(int i = 0; i < pagelen; i++) {
        I2CIP_DEBUG_SERIAL.print((char)((buffer+bytes_written)[i]));
      }
      I2CIP_DEBUG_SERIAL.print("'... ");
      DEBUG_DELAY();
    #endif

    // Note: Timeout ping before each byte write to await completion of last write cycle
    errlev = pingTimeout(false, false);
    I2CIP_ERR_BREAK(errlev);

    #ifdef I2CIP_DEBUG_SERIAL
      I2CIP_DEBUG_SERIAL.println("... ACK'd");
      DEBUG_DELAY();
    #endif

    if(buffer[bytes_written] == '\0') {
      // Null terminator
      #ifdef I2CIP_DEBUG_SERIAL
        DEBUG_DELAY();
        I2CIP_DEBUG_SERIAL.println("Overwrite stopped - null terminator");
        DEBUG_DELAY();
      #endif
      break;
    }
  }

  return errlev;
}

i2cip_errorlevel_t EEPROM::get(uint8_t*& dest, const args_eeprom_t& args) {
  if(args.len >= I2CIP_EEPROM_SIZE) {
    return I2CIP_ERR_SOFT;
  }

  // Read in register
  size_t len = args.len;
  return readRegister(args.pos, dest, len);
}

i2cip_errorlevel_t EEPROM::set(uint8_t* const& value, const args_eeprom_t& args) {
  if(args.len >= I2CIP_EEPROM_SIZE) {
    return I2CIP_ERR_SOFT;
  }

  // Write register
  size_t len = args.len;
  return writeRegister(args.pos, value, len);
}

// Args: len (2 bytes)
// int EEPROM::read(char*& dest, uint8_t argc, uint8_t[] argv) {
//   size_t len = 0;
//   if(argc == 0 || argv == nullptr) {
//     this->readContents((uint8_t*&)dest, len);
//   } else {
//     this->readContents((uint8_t*&)dest, len, argc > 1 ? ((argv[1] << 8) & argv[0]) : argv[0]);

//     argv[0] = (len & 0xFF);
//     if(argc > 1) argv[1] = (len >> 8);
//   }

//   return (len > 0 ? 0 : 1);
// }