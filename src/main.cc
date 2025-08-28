#ifndef UNIT_TEST
#define UNIT_TEST 1
#define IS_MAIN 1

#include <Arduino.h>

#include <DebugJson.h> // Debugging JSON Serial Outputs (Breakpoints, Telemetry, etc.)
#include <state.h>
#include <chronograph.h>

#include "../test/config.h"

using namespace I2CIP;

// PeaPod Finite State Machine Flags
FSM::Variable cycle(FSM::Number(0, false, false), "cycle");
FSM::Flag watering(false);
FSM::Flag lighting(false);

void setup(void) {
  // 0. Builtin LED Pinmode; Serial Begin

  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  while(!Serial) { digitalWrite(LED_BUILTIN, HIGH); delay(100); digitalWrite(LED_BUILTIN, LOW); delay(100); }

  // PeaPod Stuff - Finite State Machine Conditionals and Chronograph Flag-Set Events & Intervals
  // lighting.addLatchingConditional(true, false, controlLights); // Latching Flag Conditional - Call on toggle
  // FSM::Chronos.addEventFlag(1000, &lighting); // Timer Flag Event - Lighting ON (No-Invert; Delayed 1s)
  // FSM::Chronos.addEventFlag(DURATION_LIGHTING, &lighting, true); // Timer Flag Event - Lighting OFF (Inverted)

  watering.addLatchingConditional(true, false, controlPin<LED_BUILTIN>); // Latching Flag Conditional - Call on toggle
  FSM::Chronos.addIntervalFlag(PERIOD_WATERING, &watering); // Timer Flag Interval - Watering ON (No-Invert)
  FSM::Chronos.addIntervalFlag(PERIOD_WATERING, DURATION_WATERING, &watering, true); // Timer Flag Interval - Watering OFF (Invert)
}

// LOOP GLOBALS
unsigned long last = 0;
unsigned long lastHeartbeat = 0;
uint32_t fps = 0; // Something other than zero
bool revision = false;

void loop(void) {
  last = millis();
  #ifdef FSM_TIMER_H_
    FSM::Chronos.set(last); // Update chronograph and do event/interval conditionals & callbacks(?)
  #endif

  while(Serial.available() > 0) { // With baud 115200, this should not block
    DebugJson::update(Serial, I2CIP::commandRouter);
  }

  if(millis() - lastHeartbeat >= HEARTBEAT_DELAY) {
    DebugJson::heartbeat(millis(), Serial);
    DebugJson::revision(I2CIP_REVISION, Serial);
    DebugJson::telemetry(millis(), fps, "fps", Serial);
    lastHeartbeat = millis();
  }

  for(uint8_t m = 0; m < I2CIP_MUX_COUNT; m++) {
    if(I2CIP::MUX::pingMUX(WIRENUM, m)) {
      if(I2CIP::modules[m] == nullptr) {
        I2CIP::modules[m] = new TestModule(WIRENUM, m);

        if(m == 0) {
          // First Module - Add HT16K33
          I2CIP::modules[m]->operator()<HT16K33>(I2CIP::sevenSegmentFQA, true, _i2cip_args_io_default, NullStream);
        }
      }

      I2CIP::errlev[m] = I2CIP::modules[m]->operator()();
      // if(I2CIP::errlev[m] == I2CIP_ERR_NONE) {
      //   if(!revision) {
      //     DebugJson::revision(I2CIP_REVISION, Serial); // sends revision
      //     revision = true; // Revision sent
      //   }
      // } else {
      //   revision = false; // No revision sent
      // }
    } else {
      I2CIP::errlev[m] = I2CIP_ERR_HARD;
    }

    #ifdef I2CIP_DEBUG_SERIAL
      // Debug Serial Output
      DEBUG_DELAY();
      I2CIP_DEBUG_SERIAL.print(F("-> Module "));
      I2CIP_DEBUG_SERIAL.print(m);
      I2CIP_DEBUG_SERIAL.print(": ");
      I2CIP_DEBUG_SERIAL.println(I2CIP::modules[m] == nullptr ? "Null" : ("0x" + String(I2CIP::errlev[m], HEX)));
      DEBUG_DELAY();
    #endif
  }

  for(uint8_t m = 0; m < I2CIP_MUX_COUNT; m++) {
    if(I2CIP::modules[m] != nullptr && I2CIP::errlev[m] == I2CIP_ERR_HARD) {
      delete I2CIP::modules[m];
      I2CIP::modules[m] = nullptr;
    }
  }
  
  cycle.set(cycle.get() + FSM::Number(1, false, false)); // Set cycle and do conditionals & callbacks(?)

  #ifdef CYCLE_DELAY
  delay(CYCLE_DELAY);
  #endif

  // DEBUG PRINT: CYCLE COUNT, FPS, and ERRLEV
  unsigned long delta = millis() - last;
  fps += 1000.f / max(1.f, (float)delta);
  fps /= 2;
}

#endif