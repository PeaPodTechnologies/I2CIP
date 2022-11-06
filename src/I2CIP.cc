#include <I2CIP.h>

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>

#include <i2cip/fqa.h>
#include <i2cip/mux.h>
#include <i2cip/eeprom.h>
#include <i2cip/routingtable.h>

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
      }

      // End transmission
      if (I2CIP_FQA_TO_WIRE(fqa)->endTransmission() != 0) {
        return I2CIP_ERR_HARD;
      }
      return (success ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT);
    }

    i2cip_errorlevel_t resetBus(const i2cip_fqa_t& fqa) {
      // Note: no need to ping MUX, we'll see in real time what the result is
      beginWire(fqa);

      // Begin transmission
      I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_MUX_NUM_TO_ADDR(I2CIP_FQA_SEG_MUXNUM(fqa)));

      // Write the "inactive" bus switch instruction
      const uint8_t instruction = I2CIP_MUX_BUS_TO_INSTR(I2CIP_MUX_BUS_INACTIVE);
      if (I2CIP_FQA_TO_WIRE(fqa)->write(&instruction, 1) != 1) {
        return I2CIP_ERR_SOFT;
      }

      // End transmission
      if (I2CIP_FQA_TO_WIRE(fqa)->endTransmission() != 0) {
        return I2CIP_ERR_HARD;
      }

      return I2CIP_ERR_NONE;
    }
  };

  namespace Device {
    i2cip_errorlevel_t ping(const i2cip_fqa_t& fqa, bool resetbus) {
      // Switch MUX bus
      i2cip_errorlevel_t errlev = MUX::setBus(fqa);
      if (errlev > I2CIP_ERR_NONE) {
        return errlev;
      }

      // Begin transmission
      I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_FQA_SEG_DEVADR(fqa));

      // End transmission, check state
      if(I2CIP_FQA_TO_WIRE(fqa)->endTransmission() != 0) {
        return I2CIP_ERR_HARD;
      }

      // Switch MUX bus back
      if (resetbus) {
        errlev = MUX::resetBus(fqa);
        if (errlev > I2CIP_ERR_NONE) {
          return errlev;
        }
      }
      
      // If we made it this far, no errors occurred.
      return I2CIP_ERR_NONE;
    }

    i2cip_errorlevel_t pingTimeout(const i2cip_fqa_t& fqa, bool resetbus, unsigned int timeout) {
      // Switch MUX bus
      i2cip_errorlevel_t errlev = MUX::setBus(fqa);
      if (errlev > I2CIP_ERR_NONE) {
        return errlev;
      }

      // Check if it's actually lost
      I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_FQA_SEG_DEVADR(fqa));
      errlev = (I2CIP_FQA_TO_WIRE(fqa)->endTransmission() != 0 ? I2CIP_ERR_HARD : I2CIP_ERR_NONE);;

      // Count down until out of time of found
      while (errlev != I2CIP_ERR_NONE && timeout--) {
        // Delta 1ms
        delay(1);

        // Begin transmission
        I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_FQA_SEG_DEVADR(fqa));

        // End transmission, check state
        errlev = (I2CIP_FQA_TO_WIRE(fqa)->endTransmission() != 0 ? I2CIP_ERR_HARD : I2CIP_ERR_NONE);
      }
      
      // Double check MUX before attempting to switch
      if(errlev == I2CIP_ERR_HARD && !MUX::pingMUX(fqa)) {
        return I2CIP_ERR_HARD;
      }

      // Switch MUX bus back
      if (resetbus) {
        errlev = MUX::resetBus(fqa);
      }
      
      // If we made it this far, no errors occurred.
      return errlev;
    }

    i2cip_errorlevel_t writeByte(const i2cip_fqa_t& fqa, const uint8_t& value, bool setbus) {
      i2cip_errorlevel_t errlev;
      if (setbus) {
        // Switch MUX bus
        i2cip_errorlevel_t errlev = MUX::setBus(fqa);
        if (errlev > I2CIP_ERR_NONE) {
          return errlev;
        }
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
        if (errlev > I2CIP_ERR_NONE) {
          return errlev;
        }
      }

      return (success ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT);
    }

    i2cip_errorlevel_t write(const i2cip_fqa_t& fqa, const uint8_t* buffer, size_t len, bool setbus) {
      i2cip_errorlevel_t errlev = I2CIP_ERR_NONE;
      if (setbus) {
        errlev = MUX::setBus(fqa);
        if (errlev > I2CIP_ERR_NONE) {
          return errlev;
        }
      }

      // Was the write performed successfully?
      bool success = true;

      // Begin transmission
      I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_FQA_SEG_DEVADR(fqa));

      // Write the buffer
      if (I2CIP_FQA_TO_WIRE(fqa)->write(buffer, len) != len) {
        success = false;
      }

      // End transmission
      if (I2CIP_FQA_TO_WIRE(fqa)->endTransmission() != 0) {
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

    i2cip_errorlevel_t read(const i2cip_fqa_t& fqa, uint8_t* dest, size_t& len, bool resetbus) {
      // Device alive?
      i2cip_errorlevel_t errlev = ping(fqa, false);
      if (errlev > I2CIP_ERR_NONE) {
        return errlev;
      }

      // Read in chunks (buffer size limitation)
      size_t pos = 0;
      bool success = true;
      while (pos < len) {
        // Read whichever is greater: number of bytes remaining, or buffer size
        uint8_t read_len = ((len - pos) > I2CIP_MAXBUFFER) ? I2CIP_MAXBUFFER : (len - pos);

        // Don't stop the bus unless we've read everything
        bool read_stop = (pos >= (len - read_len));

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
          dest[i] = I2CIP_FQA_TO_WIRE(fqa)->read();
        }
        
        // Advance the index by the amount of bytes read
        pos += read_len;
      }

      // Reset MUX bus if `reset` == true
      if (resetbus) {
        errlev = MUX::resetBus(fqa);
        if (errlev > I2CIP_ERR_NONE) {
          return errlev;
        }
      }

      // Did we read all the bytes we hoped to?
      return (success ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT);
    }

    i2cip_errorlevel_t readRegisterByte(const i2cip_fqa_t& fqa, const uint8_t& reg, uint8_t& dest, bool resetbus) {
      size_t len = 1;
      return readRegister(fqa, reg, &dest, len, resetbus);
    }

    i2cip_errorlevel_t readRegisterByte(const i2cip_fqa_t& fqa, const uint16_t& reg, uint8_t& dest, bool resetbus) {
      size_t len = 1;
      return readRegister(fqa, reg, &dest, len, resetbus);
    }

    i2cip_errorlevel_t readRegister(const i2cip_fqa_t& fqa, const uint8_t& reg, uint8_t* dest, size_t& len, bool resetbus) {
      // Device alive?
      i2cip_errorlevel_t errlev = ping(fqa, false);
      if (errlev > I2CIP_ERR_NONE) {
        return errlev;
      }

      // Read in chunks (buffer size limitation)
      size_t pos = 0;
      bool success = true;
      while (pos < len) {
        // Read whichever is greater: number of bytes remaining, or buffer size
        uint8_t read_len = ((len - pos) > I2CIP_MAXBUFFER) ? I2CIP_MAXBUFFER : (len - pos);

        // Don't stop the bus unless we've read everything
        bool read_stop = (pos >= (len - read_len));

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
          dest[i] = I2CIP_FQA_TO_WIRE(fqa)->read();
        }
        
        // Advance the index by the amount of bytes read
        pos += read_len;
      }

      // Reset MUX bus if `reset` == true
      if (resetbus) {
        errlev = MUX::resetBus(fqa);
        if (errlev > I2CIP_ERR_NONE) {
          return errlev;
        }
      }

      // Did we read all the bytes we hoped to?
      return (success ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT);
    }

    i2cip_errorlevel_t readRegister(const i2cip_fqa_t& fqa, const uint16_t& reg, uint8_t* dest, size_t& len, bool resetbus) {
      // Device alive?
      i2cip_errorlevel_t errlev = ping(fqa, false);
      if (errlev > I2CIP_ERR_NONE) {
        return errlev;
      }

      // Read in chunks (buffer size limitation)
      size_t pos = 0;
      bool success = true;
      while (pos < len) {
        // Read whichever is greater: number of bytes remaining, or buffer size
        uint8_t read_len = ((len - pos) > I2CIP_MAXBUFFER) ? I2CIP_MAXBUFFER : (len - pos);

        // Don't stop the bus unless we've read everything
        bool read_stop = (pos >= (len - read_len));

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
          dest[i] = I2CIP_FQA_TO_WIRE(fqa)->read();
        }
        
        // Advance the index by the amount of bytes read
        pos += read_len;
      }

      // Reset MUX bus if `reset` == true
      if (resetbus) {
        errlev = MUX::resetBus(fqa);
        if (errlev > I2CIP_ERR_NONE) {
          return errlev;
        }
      }

      // Did we read all the bytes we hoped to?
      return (success ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT);
    }
  };

  namespace EEPROM {

    using namespace Device;

    i2cip_errorlevel_t readContents(const i2cip_fqa_t& fqa, uint8_t* dest, uint16_t& num_read, uint16_t max_read) {
      i2cip_errorlevel_t errlev = I2CIP_ERR_NONE;
      // Byte currently being read
      uint16_t bytes_read = 0;
      for (; bytes_read < max_read; bytes_read++) {
        // Read in and store each byte
        size_t len = 1;
        errlev = readRegister(fqa, bytes_read, dest + bytes_read, len, false);
        if(errlev > I2CIP_ERR_NONE || len != 1) {
          // Stop reading the EEPROM; terminate string
          dest[bytes_read] = '\0';
          bytes_read++;
          break;
        }

        // Stop reading at a null termination
        if (dest[bytes_read] == '\0') {
          bytes_read++;
          break;
        }
      }
      // Null-terminate if not already
      if (dest[bytes_read] != '\0') {
        bytes_read++;
        dest[bytes_read] = '\0';
      }
      // Destination string terminated; copy len and return status
      num_read = bytes_read;
      return errlev;
    }

    i2cip_errorlevel_t clearContents(const i2cip_fqa_t& fqa, bool setbus, uint16_t numbytes) {
      // Ping EEPROM
      i2cip_errorlevel_t errlev = ping(fqa, false);
      if (errlev > I2CIP_ERR_NONE) {
        return errlev;
      }

      for (uint16_t bytes_written = 0; bytes_written < numbytes; bytes_written++) {
        const uint8_t buff[3] = {(uint8_t)(bytes_written >> 8), (uint8_t)(bytes_written & 0xFF), 0};

        // clear each byte
        errlev = write(fqa, buff, 3, false);
        if(errlev > I2CIP_ERR_NONE) {
          return errlev;
        }

        errlev = pingTimeout(fqa, I2CIP_EEPROM_TIMEOUT, false);
        if(errlev > I2CIP_ERR_NONE) {
          return errlev;
        }
      }

      return pingTimeout(fqa, I2CIP_EEPROM_TIMEOUT, setbus);
    }

    i2cip_errorlevel_t overwriteContents(const i2cip_fqa_t& fqa, const char* contents, bool clear, bool setbus) {
      for(int i = 0; i < I2CIP_EEPROM_SIZE; i++) {
        if(contents[i] == '\0') {
          return overwriteContents(fqa, (uint8_t*)contents, i, clear, setbus);
        }
      }
      return I2CIP_ERR_SOFT;
    }

    i2cip_errorlevel_t overwriteContents(const i2cip_fqa_t& fqa, uint8_t* buffer, uint16_t len, bool clear, bool setbus) {
      i2cip_errorlevel_t errlev = clearContents(fqa, false, len);
      if(errlev > I2CIP_ERR_NONE) {
        return errlev;
      }

      // Note: No need to ping

      for (uint16_t bytes_written = 0; bytes_written < len; bytes_written++) {
        const uint8_t buff[3] = {(uint8_t)(bytes_written >> 8), (uint8_t)(bytes_written & 0xFF), buffer[bytes_written]};

        // clear each byte
        errlev = write(fqa, buff, 3, false);
        if(errlev > I2CIP_ERR_NONE) {
          return errlev;
        }

        errlev = pingTimeout(fqa, I2CIP_EEPROM_TIMEOUT, false);
        if(errlev > I2CIP_ERR_NONE) {
          return errlev;
        }
      }

      return pingTimeout(fqa, I2CIP_EEPROM_TIMEOUT, setbus);
    }
  };

  /**
   * Scans the network for modules, and allocates and builds a route table based on SPRT EEROM.
   */
  i2cip_errorlevel_t scanModule(RoutingTable& rt, const uint8_t& modulenum, const uint8_t& wirenum) {
    // Reusable buffer
    char eeprom_raw[I2CIP_EEPROM_SIZE] = { '\0' };

    // Scan every module's EEPROM
      
    i2cip_fqa_t fqa = createFQA(wirenum, modulenum, I2CIP_MUX_BUS_DEFAULT, I2CIP_EEPROM_ADDR);

    uint16_t bytes_read = 0;
    i2cip_errorlevel_t errlev = EEPROM::readContents(fqa, (uint8_t*)eeprom_raw, bytes_read);
    if(errlev > I2CIP_ERR_NONE) {
      return errlev;
    }

    // Convert char[] to json
    StaticJsonDocument<I2CIP_EEPROM_SIZE> eeprom_json;
    DeserializationError jsonerr = deserializeJson(eeprom_json, eeprom_raw);
    if(jsonerr) {
      // This module's JSON is a dud
      return I2CIP_ERR_SOFT;
    }

    // Read JSON to allocate table
    // TODO: Find a better way to do this
    JsonArray arr = eeprom_json.as<JsonArray>();

    uint8_t busnum = 0, totaldevices = 0;
    for (JsonObject bus : arr) {
      // Count reachable devices in each device group
      uint8_t devicecount = 0;
      for (JsonPair device : bus) {
        // Device addresses
        JsonArray addresses = device.value().as<JsonArray>();

        // See if each device is reachable
        uint8_t addressindex = 0;
        for (JsonVariant address : addresses) {
          fqa = createFQA(wirenum, modulenum, busnum, address.as<uint8_t>());
          if(Device::ping(fqa) > I2CIP_ERR_NONE) {
            // Device unreachable, remove from JSON; next device
            addresses.remove(addressindex);
            errlev = I2CIP_ERR_SOFT;
            continue;
          }
          // Device reachable; increment device count and address index
          devicecount++;
          addressindex++;
        }
      }
      busnum++;

      // Add the number of devices on this bus to the tally
      totaldevices += devicecount;
    }

    // Read JSON to fill table
    busnum = 0;
    for (JsonObject bus : arr) {
      for (JsonPair device : bus) {
        // Device addresses
        JsonArray addresses = device.value().as<JsonArray>();
        // Device ID (stack)
        const char* id = device.key().c_str();
        
        // Add to table
        for (JsonVariant address : addresses) {
          fqa = createFQA(wirenum, modulenum, busnum, address.as<uint8_t>());
          rt.add(id, fqa);
        }
      }
      busnum++;
    }
    return errlev;
  }
};