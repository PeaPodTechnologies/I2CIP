#include <I2CIP.h>

#include <string.h>

#include <Arduino.h>
#include <Wire.h>

extern TwoWire Wire;

#define NUM_WIRES 1

static PROGMEM TwoWire* const wires[NUM_WIRES] = { &Wire };

#define I2CIP_FQA_TO_WIRE(fqa) (wires[I2CIP_FQA_SEG_I2CBUS(fqa)])

namespace I2CIP {
  namespace Device {
    i2cip_errorlevel_t ping(const i2cip_fqa_t& fqa) {
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
      errlev = MUX::resetBus(fqa);
      if (errlev > I2CIP_ERR_NONE) {
        return errlev;
      }
      
      // If we made it this far, no errors occurred.
      return I2CIP_ERR_NONE;
    }

    /**
     * Write a buffer to the device.
     * Pings and sets the MUX bus, pings the device, 
     */
    i2cip_errorlevel_t write(const i2cip_fqa_t& fqa, const unsigned char* buffer, size_t len, bool reset) {
      // Switch MUX bus
      i2cip_errorlevel_t errlev = MUX::setBus(fqa);
      if (errlev > I2CIP_ERR_NONE) {
        return errlev;
      }

      // Device alive?
      errlev = ping(fqa);
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

    i2cip_errorlevel_t write(const i2cip_fqa_t& fqa, const unsigned char& b, bool reset) {
      return write(fqa, &b, 1, reset);
    }

    i2cip_errorlevel_t writeRegister(const i2cip_fqa_t& fqa, const unsigned char& reg, const unsigned char& value, bool reset) {
      const unsigned char buf[2] = { reg, value };
      return write(fqa, buf, 2, reset);
    }

    // errorlevel_t write(const i2cip_fqa_t& fqa, const uint16_t& b, bool reset = true) {
    //   return writeRegister(fqa, (unsigned char)(b >> 8), (unsigned char)(b & 0xFF), reset);
    // }

    // errorlevel_t writeRegister(const i2cip_fqa_t& fqa, const unsigned char& reg, const uint16_t& value, bool reset = true) {
    //   const unsigned char buf[3] = { reg, value >> 8, value & 0xFF };
    //   return write(fqa, buf, 3, reset);
    // }

    i2cip_errorlevel_t read(const i2cip_fqa_t& fqa, unsigned char* buffer, size_t len, bool reset) {
      // Switch MUX bus
      i2cip_errorlevel_t errlev = MUX::setBus(fqa);
      if (errlev > I2CIP_ERR_NONE) {
        return errlev;
      }

      // Device alive?
      errlev = ping(fqa);
      if (errlev > I2CIP_ERR_NONE) {
        return errlev;
      }

      // Read in chunks (buffer size limitation)
      size_t pos = 0;
      bool success = true;
      while (pos < len) {
        // Read whichever is greater: number of bytes remaining, or buffer size
        unsigned char read_len = ((len - pos) > I2CIP_MAXBUFFER) ? I2CIP_MAXBUFFER : (len - pos);

        // Don't stop the bus unless we've read everything
        bool read_stop = (pos >= (len - read_len));

        // Request bytes; How many have we received?
        size_t recv = I2CIP_FQA_TO_WIRE(fqa)->requestFrom(I2CIP_FQA_SEG_DEVADR(fqa), read_len, read_stop);
        
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
      unsigned char instruction = I2CIP_MUX_BUS_TO_INSTR(I2CIP_FQA_SEG_MUXBUS(fqa));
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
      unsigned char instruction = I2CIP_MUX_BUS_TO_INSTR(I2CIP_MUX_BUS_INACTIVE);
      if (I2CIP_FQA_TO_WIRE(fqa)->write(instruction, 1) != 1) {
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
    /**
     * Scans the network for modules, and allocates and builds a route table based on SPRT EEROM.
     */
    RoutingTable* createRoutingTable(void) {
      RoutingTable* newtable = new RoutingTable();
      char eeprom_raw[I2CIP_EEPROM_SIZE];
      StaticJsonDocument<I2CIP_EEPROM_SIZE> eeprom_json;
      for(unsigned char wire = 0; wire < NUM_WIRES; wire++) {
        for (unsigned char mux = 0; mux < I2CIP_MUX_COUNT; mux++) {
          
          i2cip_fqa_t fqa = createFQA(wire, mux, 0, 0);

          if(!MUX::pingMUX(fqa)) {
            break;
          }

          // Ping the EEPROM (Default bus and address)
          fqa = createFQA(wire, mux, I2CIP_MUX_BUS_DEFAULT, I2CIP_EEPROM_ADDR);
          i2cip_errorlevel_t errlev = Device::ping(fqa);
          if (errlev > I2CIP_ERR_NONE) {
            // Skip this module entirely until next scan
            break;
          }

          // Byte currently being read
          uint8_t bytenum = 0, bytes_read = 0;
          for (; bytes_read < I2CIP_EEPROM_SIZE; bytes_read++) {
            // Request data from the EEPROM chip at byte `bytenum`
            errlev = Device::writeRegister(fqa, (unsigned char)(bytenum >> 8), (unsigned char)(bytenum & 0xFF));
            if(errlev > I2CIP_ERR_NONE) {
              // Stop reading the EEPROM
              eeprom_raw[bytes_read] = '\0';
              bytes_read++;
              break;
            }
            // Store the requested data (copy as a byte pointer)
            errlev = Device::read(fqa, &eeprom_raw[bytes_read], 1, false);
            if(errlev > I2CIP_ERR_NONE) {
              // Stop reading the EEPROM
              eeprom_raw[bytes_read] = '\0';
              bytes_read++;
              break;
            }
            if (eeprom_raw[bytes_read] == '\0') {
              // END
              bytes_read++;
              break;
            }
            bytenum++;
          }

          // Convert char[] to json
          DeserializationError jsonerr = deserializeJson(eeprom_json, eeprom_raw);
          if(jsonerr) {
            // This module's JSON is a dud
            break;
          }

          // Read JSON to allocate table
          // TODO: Find a better way to do this
          JsonArray arr = eeprom_json.as<JsonArray>();

          unsigned char busnum = 0, totaldevices = 0;
          for (JsonObject bus : arr) {
            // Count reachable devices in each device group
            unsigned char devicecount = 0;
            for (JsonPair device : bus) {
              // Device addresses
              JsonArray addresses = device.value().as<JsonArray>();

              // See if each device is reachable
              unsigned char addressindex = 0;
              for (JsonVariant address : addresses) {
                fqa = createFQA(wire, mux, busnum, address.as<unsigned char>());
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
          unsigned char devicecount = 0, busnum = 0;
          for (JsonObject bus : arr) {
            for (JsonPair device : bus) {
              // Device addresses
              JsonArray addresses = device.value().as<JsonArray>();
              // Device ID (stack)
              const char* id = device.key().c_str();
              
              // Add to table
              for (JsonVariant address : addresses) {
                fqa = createFQA(wire, mux, busnum, address.as<unsigned char>());
                newtable->add(fqa, id);
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