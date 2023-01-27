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
        ( mux  < I2CIP_FQA_MUXNUM_MAX ) &&
        ( bus  < I2CIP_FQA_MUXBUS_MAX ) &&
        ( addr < I2CIP_FQA_DEVADR_MAX )
    ) {
      return I2CIP_FQA_CREATE(wire, mux, bus, addr);
    }
    return (i2cip_fqa_t)0;
  }

  void beginWire(const i2cip_fqa_t fqa) {
    if(!wiresBegun[I2CIP_FQA_SEG_I2CBUS(fqa)]) {
      #ifdef DEBUG_SERIAL
        Serial.print("Initializing I2C wire ");
        Serial.println(I2CIP_FQA_SEG_I2CBUS(fqa));
      #endif
      I2CIP_FQA_TO_WIRE(fqa)->begin();
      wiresBegun[I2CIP_FQA_SEG_I2CBUS(fqa)] = true;
    }
  }

  namespace MUX {
    bool pingMUX(const i2cip_fqa_t& fqa) {
      beginWire(fqa);
      I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_MUX_NUM_TO_ADDR(I2CIP_FQA_SEG_MUXNUM(fqa)));
      return (I2CIP_FQA_TO_WIRE(fqa)->endTransmission() == 0);
    }
    
    i2cip_errorlevel_t setBus(const i2cip_fqa_t& fqa) {
      // Note: no need to ping MUX, we'll see in real time what the result is
      beginWire(fqa);

      // Was the bus switched successfully?
      bool success = true;

      // Begin transmission
      I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_MUX_NUM_TO_ADDR(I2CIP_FQA_SEG_MUXNUM(fqa)));

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
      beginWire(fqa);

      // Begin transmission
      I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_MUX_NUM_TO_ADDR(I2CIP_FQA_SEG_MUXNUM(fqa)));

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

  namespace Device {
    i2cip_errorlevel_t ping(const i2cip_fqa_t& fqa, bool resetbus) {
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

    i2cip_errorlevel_t pingTimeout(const i2cip_fqa_t& fqa, bool setbus, bool resetbus, unsigned int timeout) {
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

    i2cip_errorlevel_t writeByte(const i2cip_fqa_t& fqa, const uint8_t& value, bool setbus) {
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

    i2cip_errorlevel_t write(const i2cip_fqa_t& fqa, const uint8_t* buffer, size_t len, bool setbus) {
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
    
    i2cip_errorlevel_t writeRegister(const i2cip_fqa_t& fqa, const uint8_t& reg, const uint8_t& value, bool setbus) {
      const uint8_t buf[2] = { reg, value };
      return write(fqa, buf, 2, setbus);
    }

    i2cip_errorlevel_t writeRegister(const i2cip_fqa_t& fqa, const uint16_t& reg, const uint8_t& value, bool setbus) {
      const uint8_t buf[3] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF), value };
      return write(fqa, buf, 3, setbus);
    }

    i2cip_errorlevel_t writeRegister(const i2cip_fqa_t& fqa, const uint8_t& reg, uint8_t* buffer, size_t len, bool setbus) {
      uint8_t buff[len + 1] = { reg };
      for(size_t i = 0; i < len; i++) {
        buff[i + 1] = buffer[i];
      }
      return write(fqa, buff, len + 1, setbus);
    }

    i2cip_errorlevel_t writeRegister(const i2cip_fqa_t& fqa, const uint16_t& reg, uint8_t* buffer, size_t len, bool setbus) {
      uint8_t buff[len + 2] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF) };
      for(size_t i = 0; i < len; i++) {
        buff[i + 2] = buffer[i];
      }
      return write(fqa, buff, len + 2, setbus);
    }

    i2cip_errorlevel_t read(const i2cip_fqa_t& fqa, uint8_t* dest, size_t& len, bool nullterminate, bool resetbus) {
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

    i2cip_errorlevel_t readByte(const i2cip_fqa_t& fqa, uint8_t& dest, bool resetbus) {
      size_t len = 1;
      return read(fqa, &dest, len, resetbus);
    }

    i2cip_errorlevel_t readWord(const i2cip_fqa_t& fqa, uint16_t& dest, bool resetbus) {
      size_t len = 2;
      uint8_t buff[2];
      i2cip_errorlevel_t errlev = read(fqa, buff, len, resetbus);
      I2CIP_ERR_BREAK(errlev);
      dest = ((uint16_t)buff[1] << 8) | (uint16_t)buff[0];
      return errlev;
    }

    i2cip_errorlevel_t readRegisterByte(const i2cip_fqa_t& fqa, const uint8_t& reg, uint8_t& dest, bool resetbus) {
      size_t len = 1;
      return readRegister(fqa, reg, &dest, len, false, resetbus);
    }

    i2cip_errorlevel_t readRegisterByte(const i2cip_fqa_t& fqa, const uint16_t& reg, uint8_t& dest, bool resetbus) {
      size_t len = 1;
      return readRegister(fqa, reg, &dest, len, false, resetbus);
    }

    i2cip_errorlevel_t readRegisterWord(const i2cip_fqa_t& fqa, const uint8_t& reg, uint16_t& dest, bool resetbus) {
      size_t len = 2;
      uint8_t buff[2];
      i2cip_errorlevel_t errlev = readRegister(fqa, reg, buff, len, false, resetbus);
      I2CIP_ERR_BREAK(errlev);
      dest = ((uint16_t)buff[1] << 8) | (uint16_t)buff[0];
      return errlev;
    }

    i2cip_errorlevel_t readRegisterWord(const i2cip_fqa_t& fqa, const uint16_t& reg, uint16_t& dest, bool resetbus) {
      size_t len = 2;
      uint8_t buff[2];
      i2cip_errorlevel_t errlev = readRegister(fqa, reg, buff, len, false, resetbus);
      I2CIP_ERR_BREAK(errlev);
      dest = ((uint16_t)buff[1] << 8) | (uint16_t)buff[0];
      return errlev;
    }

    i2cip_errorlevel_t readRegister(const i2cip_fqa_t& fqa, const uint8_t& reg, uint8_t* dest, size_t& len, bool nullterminate, bool resetbus) {
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

    i2cip_errorlevel_t readRegister(const i2cip_fqa_t& fqa, const uint16_t& reg, uint8_t* dest, size_t& len, bool nullterminate, bool resetbus) {
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
          len = pos;
          break;
        }

        // Read in all the bytes
        for (uint16_t i = 0; i < read_len; i++) {
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
        
        // Advance the index by the amount of bytes read
        pos += read_len;
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
  };

  namespace EEPROM {

    using namespace Device;

    i2cip_errorlevel_t readContents(const i2cip_fqa_t& fqa, uint8_t* dest, size_t& num_read, size_t max_read) {
      size_t bytes_read = max_read;
      i2cip_errorlevel_t errlev = readRegister(fqa, (uint16_t)0, dest, bytes_read, true, false);
      num_read = bytes_read;
      return errlev;
    }

    i2cip_errorlevel_t clearContents(const i2cip_fqa_t& fqa, bool setbus, uint16_t numbytes) {
      i2cip_errorlevel_t errlev = Device::pingTimeout(fqa, setbus, false);
      I2CIP_ERR_BREAK(errlev);

      uint8_t zeroes[8] = {0, 0, 0, 0, 0, 0, 0, 0};

      for (uint16_t bytes_written = 0; bytes_written < numbytes; bytes_written+=8) {
        const uint8_t pagelen = min(numbytes - bytes_written, 8);
        
        errlev = writeRegister(fqa, bytes_written, zeroes, pagelen, false);
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
        errlev = pingTimeout(fqa, false, false);
        I2CIP_ERR_BREAK(errlev);
      }

      return errlev;
    }

    i2cip_errorlevel_t overwriteContents(const i2cip_fqa_t& fqa, const char* contents, bool clear, bool setbus) {
      for(size_t i = 0; i < I2CIP_EEPROM_SIZE; i++) {
        if(contents[i] == '\0') {
          return overwriteContents(fqa, (uint8_t*)contents, i, clear, setbus);
        }
      }
      return I2CIP_ERR_SOFT;
    }

    i2cip_errorlevel_t overwriteContents(const i2cip_fqa_t& fqa, uint8_t* buffer, size_t len, bool clear, bool setbus) {
      i2cip_errorlevel_t errlev = I2CIP_ERR_NONE;
      if(clear) {
        errlev = clearContents(fqa, setbus, len);
      } else if(setbus) {
        errlev = MUX::setBus(fqa);
      }
      I2CIP_ERR_BREAK(errlev);

      for (uint16_t bytes_written = 0; bytes_written < len; bytes_written+=8) {
        const uint8_t pagelen = min(len - bytes_written, 8);
        errlev = writeRegister(fqa, bytes_written, buffer+bytes_written, pagelen, false);
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
        errlev = pingTimeout(fqa, false, false);
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
  };
};