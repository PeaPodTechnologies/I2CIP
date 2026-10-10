#ifndef UNIT_TEST
#ifndef IS_MAIN
#define UNIT_TEST 1
#define IS_MAIN 1

#include <Arduino.h>

#include <DebugJson.h> // Debugging JSON Serial Outputs (Breakpoints, Telemetry, etc.)
#include <chronograph.h>

#include "../test/config.h"

using namespace I2CIP;

void readAndPrintTemperature(bool _, const FSM::Number& cycle);

FSM::Variable cycle(FSM::Number(0, false, false), "cycle");

void setup(void) {
  // 0. Builtin LED Pinmode; Serial Begin

  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  while(!Serial) { digitalWrite(LED_BUILTIN, HIGH); delay(100); digitalWrite(LED_BUILTIN, LOW); delay(100); }

  I2CIP::modules[I2CIP_MUX_NUM_FAKE] = new TestNoModule(I2CIP_WIRENUM_PRIMARY);

  cycle.addCallback(&readAndPrintTemperature);
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
    if(I2CIP::MUX::pingMUX(I2CIP_WIRENUM_PRIMARY, m)) {
      if(I2CIP::modules[m] == nullptr) {
        I2CIP::modules[m] = new TestModule(I2CIP_WIRENUM_PRIMARY, m);
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

void readAndPrintTemperature(bool _, const FSM::Number& cycle) {
  DeviceGroup* dg_sht45 = modules[I2CIP_TEST_MODULE]->operator[]("SHT45");
  if(dg_sht45 == nullptr || dg_sht45->getNumDevices() == 0) return;

  uint8_t num_sht45 = dg_sht45->getNumDevices();
  state_sht45_t state = { .temperature = 0.0f, .humidity = 0.0f };
  uint8_t count = 0;
  for(uint8_t i = 0; i < num_sht45; i++) {
    SHT45* sht45 = (SHT45*)dg_sht45->getDevice(i);
    if(sht45 == nullptr) continue;
    // state_sht45_t state;
    // args_sht45_t args = SHT45_HEATER_DISABLE;
    // sht45->get(state, args);
    // unsigned long now = millis();
    // DebugJson::telemetry<float>(now, state.temperature);
    // DebugJson::telemetry<float>(now, state.humidity);
    i2cip_errorlevel_t errlev_sht45 = modules[I2CIP_TEST_MODULE]->operator()<SHT45>(sht45->getFQA(), true, _i2cip_args_io_default, DebugJsonOut);
    if(errlev_sht45 != I2CIP_ERR_NONE) continue;

    DebugJson::telemetry(sht45->getLastRX(), sht45->getCache().temperature, "temperature");
    DebugJson::telemetry(sht45->getLastRX(), sht45->getCache().humidity, "humidity");

    state.temperature += sht45->getCache().temperature;
    state.humidity += sht45->getCache().humidity;
    count++;
  }

  if(count > 0) {
    state.temperature /= count;
    state.humidity /= count;
  }

  #ifdef I2CIP_TEST_USE_SEVENSEGMENT
    i2cip_ht16k33_mode_t seg_mode = SEG_2F;
    i2cip_ht16k33_data_t seg_data = { .f = (float)state.temperature };
    i2cip_args_io_t args_sevenseg = { .g = false, .a = nullptr, .s = &seg_data, .b = &seg_mode };
    if(I2CIP::modules[I2CIP_MUX_NUM_FAKE] != nullptr) {
      errlev_sevenseg = I2CIP::modules[I2CIP_MUX_NUM_FAKE]->operator()<HT16K33>(fqa_sevenseg, true, args_sevenseg, NullStream);
    } else {
      I2CIP::errlev[I2CIP_MUX_NUM_FAKE] = I2CIP_ERR_HARD;
    }
  #endif
}

#endif
#endif