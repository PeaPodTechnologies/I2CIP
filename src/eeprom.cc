#include <eeprom.h>

#include <fqa.h>
#include <device.h>
#include <debug.h>

const char* I2CIP::i2cip_eeprom_id = "eeprom";

// Default EEPROM module self-discovery JSON string
static char eeprom_default[20] = I2CIP_EEPROM_DEFAULT;

// Constant pointer
const char* const I2CIP::i2cip_eeprom_default = &eeprom_default[0];

const uint16_t I2CIP::i2cip_eeprom_capacity = I2CIP_EEPROM_SIZE;

const I2CIP::factory_device_t I2CIP::i2cip_eeprom_factory = &I2CIP::eepromFactory;

I2CIP::Device* I2CIP::eepromFactory(const i2cip_fqa_t& fqa) {
  return (I2CIP::Device*)(new I2CIP::EEPROM(fqa));
}

using namespace I2CIP;

EEPROM::EEPROM(const i2cip_fqa_t& fqa) : Device(fqa, i2cip_eeprom_id), IOInterface<char*, uint16_t, const char*, uint16_t>((Device*)this) {
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print("EEPROM Constructed (");
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_I2CBUS(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(":");
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MODULE(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(":");
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_MUXBUS(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(":");
    I2CIP_DEBUG_SERIAL.print(I2CIP_FQA_SEG_DEVADR(fqa), HEX);
    I2CIP_DEBUG_SERIAL.print(")\n");
    DEBUG_DELAY();
  #endif
}

EEPROM::EEPROM(const uint8_t& wire, const uint8_t& module, const uint8_t& addr) : Device(createFQA(wire, module, I2CIP_MUX_BUS_DEFAULT, addr), i2cip_eeprom_id), IOInterface<char*, uint16_t, const char*, uint16_t>((Device*)this) {
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print("EEPROM Constructed (");
    I2CIP_DEBUG_SERIAL.print(wire, HEX);
    I2CIP_DEBUG_SERIAL.print(":");
    I2CIP_DEBUG_SERIAL.print(module, HEX);
    I2CIP_DEBUG_SERIAL.print(":");
    I2CIP_DEBUG_SERIAL.print(I2CIP_MUX_BUS_DEFAULT, HEX);
    I2CIP_DEBUG_SERIAL.print(":");
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

i2cip_errorlevel_t EEPROM::get(char*& dest, const uint16_t& args) {
  // 0. Check args
  if(args > I2CIP_EEPROM_SIZE) {
    return I2CIP_ERR_SOFT;
  }

  // 1. Read register (until null terminator or max bytes, whichever comes first) into a local buffer
  size_t len = args;
  uint8_t buffer[len];
  i2cip_errorlevel_t errlev = readRegister((uint16_t)0, buffer, len);
  I2CIP_ERR_BREAK(errlev);

  // if((uint16_t)len != args) return I2CIP_ERR_SOFT;

  // 2. Copy local buffer to heap buffer (null-terminated)
  if(dest != nullptr) delete dest;
  dest = new char[len+1];
  if(dest == nullptr) return I2CIP_ERR_SOFT;

  for(size_t i = 0; i < len; i++) {
    dest[i] = (char)buffer[i];
  }
  dest[len] = '\0';

  return errlev;
}

i2cip_errorlevel_t EEPROM::set(const char * const& value, const uint16_t& args) {
  if(args > I2CIP_EEPROM_SIZE) {
    return I2CIP_ERR_SOFT;
  }

  // Write register
  size_t len = args;
  i2cip_errorlevel_t errlev = writeRegister((uint16_t)0, (uint8_t*)value, len);
  I2CIP_ERR_BREAK(errlev);

  // if((uint16_t)len != args) return I2CIP_ERR_SOFT;
  return errlev;
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

// G - Getter type: char* (null-terminated; writable heap)
void EEPROM::resetCache(void) {
  delete this->getCache();
  char* newcache = new char[1];
  newcache[0] = '\0';
  this->setCache(newcache);
}

// A - Getter argument type: uint16_t (max bytes to read)
const uint16_t& EEPROM::getDefaultA(void) const {
  return i2cip_eeprom_capacity;
}

// S - Setter type: const char* (null-terminated; immutable)
const char* const& EEPROM::getFailsafe(void) const {
  return i2cip_eeprom_default;
}

// B - Setter argument type: uint16_t (max bytes to write)
const uint16_t& EEPROM::getDefaultB(void) const {
  return i2cip_eeprom_capacity;
}