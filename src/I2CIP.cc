#include <I2CIP.h>

#include <stdlib.h>
#include <string.h>

#include <Arduino.h>
#include <Wire.h>

#include <i2cip/fqa.h>
#include <i2cip/mux.h>
#include <i2cip/eeprom.h>
#include <i2cip/routingtable.h>

// Has wire N been wires[N].begin() yet?
static bool wiresBegun[I2CIP_NUM_WIRES] = { false };

namespace I2CIP {

  /**
   * Create an FQA from segments.
   * @param wire The index of the I2C bus this network is on.
   * @param 
   */
  i2cip_fqa_t createFQA(const uint8_t& wire, const uint8_t& mux, const uint8_t& bus, const uint8_t& addr) {
    return I2CIP_FQA_CREATE(wire, mux, bus, addr);
  }

  void beginWire(const i2cip_fqa_t fqa) {
    if(!wiresBegun[I2CIP_FQA_SEG_I2CBUS(fqa)]) {
      I2CIP_FQA_TO_WIRE(fqa)->begin();
      wiresBegun[I2CIP_FQA_SEG_I2CBUS(fqa)] = true;
    }
  }

  namespace Device {
    i2cip_errorlevel_t ping(const i2cip_fqa_t& fqa, bool reset) {
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
      if (reset) {
        errlev = MUX::resetBus(fqa);
        if (errlev > I2CIP_ERR_NONE) {
          return errlev;
        }
      }
      
      // If we made it this far, no errors occurred.
      return I2CIP_ERR_NONE;
    }

    /**
     * Write a buffer to the device.
     * Pings and sets the MUX bus, pings the device, 
     */
    i2cip_errorlevel_t write(const i2cip_fqa_t& fqa, const uint8_t* buffer, size_t len, bool reset) {
      // Device alive?
      i2cip_errorlevel_t errlev = ping(fqa, false);
      if (errlev > I2CIP_ERR_NONE) {
        return errlev;
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
      if(reset) {
        errlev = MUX::resetBus(fqa);
        if (errlev > I2CIP_ERR_NONE) {
          return errlev;
        }
      }

      return (success ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT);
    }

    i2cip_errorlevel_t write(const i2cip_fqa_t& fqa, const uint8_t& b, bool reset) {
      return write(fqa, &b, 1, reset);
    }

    i2cip_errorlevel_t writeRegister(const i2cip_fqa_t& fqa, const uint8_t& reg, const uint8_t& value, bool reset) {
      const uint8_t buf[2] = { reg, value };
      return write(fqa, buf, 2, reset);
    }

    // errorlevel_t write(const i2cip_fqa_t& fqa, const uint16_t& b, bool reset = true) {
    //   return writeRegister(fqa, (uint8_t)(b >> 8), (uint8_t)(b & 0xFF), reset);
    // }

    // errorlevel_t writeRegister(const i2cip_fqa_t& fqa, const uint8_t& reg, const uint16_t& value, bool reset = true) {
    //   const uint8_t buf[3] = { reg, value >> 8, value & 0xFF };
    //   return write(fqa, buf, 3, reset);
    // }

    i2cip_errorlevel_t read(const i2cip_fqa_t& fqa, uint8_t* buffer, size_t len, bool reset) {
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
        if(recv != read_len) {
          success = false;
          break;
        }

        // Read in all the bytes
        for (uint16_t i = 0; i < read_len; i++) {
          buffer[i] = I2CIP_FQA_TO_WIRE(fqa)->read();
        }
        
        // Advance the index by the amount of bytes read
        pos += read_len;
      }

      // Reset MUX bus if `reset` == true
      if(reset) {
        errlev = MUX::resetBus(fqa);
        if (errlev > I2CIP_ERR_NONE) {
          return errlev;
        }
      }

      // Did we read all the bytes we hoped to?
      return (success ? I2CIP_ERR_NONE : I2CIP_ERR_SOFT);
    }
  };

  namespace MUX {
    bool pingMUX(const i2cip_fqa_t& fqa) {
      beginWire(fqa);
      I2CIP_FQA_TO_WIRE(fqa)->beginTransmission(I2CIP_MUX_NUM_TO_ADDR(I2CIP_FQA_SEG_MUXNUM(fqa)));
      return (I2CIP_FQA_TO_WIRE(fqa)->endTransmission() == 0);
    }
    
    i2cip_errorlevel_t setBus(const i2cip_fqa_t& fqa) {
      // Ping MUX
      if(!pingMUX(fqa)) {
        return I2CIP_ERR_HARD;
      }

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
      // Ping MUX
      if(!pingMUX(fqa)) {
        return I2CIP_ERR_HARD;
      }

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
  namespace Routing {

    namespace EEPROM {

      i2cip_errorlevel_t readByte(i2cip_fqa_t fqa, uint16_t bytenum, uint8_t& dest, bool reset) {
        // Ping EEPROM
        i2cip_errorlevel_t errlev = Device::ping(fqa, false);
        if (errlev > I2CIP_ERR_NONE) {
          return errlev;
        }

        // Request data from the EEPROM chip at byte `bytenum`
        errlev = Device::writeRegister(fqa, (uint8_t)(bytenum >> 8), (uint8_t)(bytenum & 0xFF), false);
        if(errlev > I2CIP_ERR_NONE) {
          return errlev;
        }

        // Read in and store the data we requested
        return Device::read(fqa, &dest, 1, reset);
      }

      i2cip_errorlevel_t readContents(i2cip_fqa_t fqa, uint8_t* dest, uint16_t& num_read, uint16_t max_read) {
        digitalWrite(13, LOW);
        i2cip_errorlevel_t errlev = I2CIP_ERR_NONE;
        // Byte currently being read
        uint16_t bytes_read = 0;
        for (; bytes_read < max_read; bytes_read++) {
          // Read in and store each byte
          errlev = readByte(fqa, bytes_read, dest[bytes_read], false);
          if(errlev > I2CIP_ERR_NONE) {
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

      // i2cip_errorlevel_t writeByte(i2cip_fqa_t fqa, uint16_t bytenum, uint8_t dest) {

      // i2cip_errorlevel_t overwriteContents(i2cip_fqa_t fqa, uint8_t* newcontents, uint16_t numbytes = I2CIP_EEPROM_SIZE);
    };

    /**
     * Scans the network for modules, and allocates and builds a route table based on SPRT EEROM.
     */
    RoutingTable* createRoutingTable(void) {
      RoutingTable* newtable = new RoutingTable();
      // Reusable buffer
      char eeprom_raw[I2CIP_EEPROM_SIZE] = { '\0' };

      // Scan every module's EEPROM
      for(uint8_t wire = 0; wire < I2CIP_NUM_WIRES; wire++) {
        for (uint8_t mux = 0; mux < I2CIP_MUX_COUNT; mux++) {
        
          i2cip_fqa_t fqa = createFQA(wire, mux, I2CIP_MUX_BUS_DEFAULT, I2CIP_EEPROM_ADDR);

          uint16_t bytes_read = 0;
          if(EEPROM::readContents(fqa, (uint8_t*)eeprom_raw, bytes_read) > I2CIP_ERR_NONE) {
            continue;
          }

          // Convert char[] to json
          StaticJsonDocument<I2CIP_EEPROM_SIZE> eeprom_json;
          DeserializationError jsonerr = deserializeJson(eeprom_json, eeprom_raw);
          if(jsonerr) {
            // This module's JSON is a dud
            continue;
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
                fqa = createFQA(wire, mux, busnum, address.as<uint8_t>());
                if(Device::ping(fqa) > I2CIP_ERR_NONE) {
                  // Device unreachable, remove from JSON; next device
                  addresses.remove(addressindex);
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
                fqa = createFQA(wire, mux, busnum, address.as<uint8_t>());
                newtable->add(id, fqa);
              }
            }
            busnum++;
          }
        }

      }
      return newtable;
    }
  };
};