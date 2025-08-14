#ifndef UNIT_TEST
#define UNIT_TEST 1
#define IS_MAIN 1

#include <Arduino.h>

#include <DebugJson.h> // Debugging JSON Serial Outputs (Breakpoints, Telemetry, etc.)

#include "../test/config.h"

using namespace I2CIP;

// #define HARDSTART 1 // Uncomment to enable hardcoded startup

#ifdef HARDSTART
  i2cip_fqa_t fqa_sht45 = createFQA(WIRENUM, MODULE, 0, I2CIP_SHT45_ADDRESS);
  // i2cip_fqa_t fqa_pca9685 = createFQA(WIRENUM, MODULE, 1, I2CIP_PCA9685_ADDRESS);
  i2cip_fqa_t fqa_lcd = createFQA(WIRENUM, MODULE, 1, I2CIP_JHD1313_ADDRESS);
  i2cip_fqa_t fqa_rotary = createFQA(WIRENUM, MODULE, 0, I2CIP_SEESAW_ADDRESS);
  i2cip_fqa_t fqa_nunchuck = createFQA(WIRENUM, MODULE, 0, I2CIP_NUNCHUCK_ADDRESS);
#endif

#ifdef FSM_STATE_H_
// PeaPod Finite State Machine Flags
FSM::Variable cycle(FSM::Number(0, false, false), "cycle");
// FSM::Flag watering(false);
// FSM::Flag lighting(false);
#endif

void setup(void) {
  // 0. Builtin LED Pinmode; Serial Begin

  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  while(!Serial) { digitalWrite(LED_BUILTIN, HIGH); delay(100); digitalWrite(LED_BUILTIN, LOW); delay(100); }

  #ifdef HARDSTART
    modules[MODULE] = new TestModule(WIRENUM, MODULE);

    // NOTE: module.eeprom == nullptr; and module["eeprom"] == nullptr
    

    // Each operator() call adds the Device to I2CIP::devicegroups and I2CIP::devicetree
    modules[MODULE]->operator()<HT16K33>(ht16k33->getFQA(), true, _i2cip_args_io_default, DebugJsonBreakpoints);
    // modules[MODULE]->operator()<PCA9685>(fqa_pca9685, false, _i2cip_args_io_default, DebugJsonBreakpoints);
    modules[MODULE]->operator()<SHT45>(fqa_sht45, true, _i2cip_args_io_default, DebugJsonBreakpoints);
    modules[MODULE]->operator()<JHD1313>(fqa_lcd, true, _i2cip_args_io_default, DebugJsonBreakpoints);
    modules[MODULE]->operator()<Seesaw>(fqa_rotary, true, _i2cip_args_io_default, DebugJsonBreakpoints);
  #endif

  // #ifdef FSM_STATE_H_
  //   // PeaPod Stuff - Finite State Machine Conditionals and Chronograph Flag-Set Events & Intervals
  //   // lighting.addLatchingConditional(true, false, controlLights); // Latching Flag Conditional - Call on toggle
  //   // FSM::Chronos.addEventFlag(1000, &lighting); // Timer Flag Event - Lighting ON (No-Invert; Delayed 1s)
  //   // FSM::Chronos.addEventFlag(DURATION_LIGHTING, &lighting, true); // Timer Flag Event - Lighting OFF (Inverted)

  //   watering.addLatchingConditional(true, false, controlPin<PIN_WATERING>); // Latching Flag Conditional - Call on toggle
  //   FSM::Chronos.addIntervalFlag(PERIOD_WATERING, &watering); // Timer Flag Interval - Watering ON (No-Invert)
  //   FSM::Chronos.addIntervalFlag(PERIOD_WATERING, DURATION_WATERING, &watering, true); // Timer Flag Interval - Watering OFF (Invert)
  // #endif
}

// LOOP GLOBALS
unsigned long last = 0;
unsigned long lastHeartbeat = 0;
uint32_t fps = 0; // Something other than zero
bool revision = false;

void loop(void) {
  last = millis();
  #ifdef FSM_STATE_H_
    FSM::Chronos.set(last); // Update chronograph and do event/interval conditionals & callbacks(?)
  #endif

  if(millis() - lastHeartbeat >= HEARTBEAT_DELAY) {
    DebugJson::heartbeat(millis(), Serial);
    DebugJson::revision(I2CIP_REVISION, Serial);
    lastHeartbeat = millis();
  }

  DebugJson::update(Serial, I2CIP::commandRouter);
  
  #ifndef HARDSTART
    // 1. MUX & EEPROM: Ping and load devices
    // for(uint8_t m = 0; m < 1; m++) {// I2CIP_MUX_COUNT; m++)  {
    //   if(I2CIP::modules[m] == nullptr || I2CIP::errlev[m] == I2CIP_ERR_HARD) {
    //     if(I2CIP::MUX::pingMUX(WIRENUM, m)) {
    //       modules[m] = new TestModule(WIRENUM, m);
    //     }
    //   }
    //   if(modules[m] != nullptr && errlev[m] != I2CIP_ERR_HARD) {
    //     errlev[m] = modules[m]->operator()(); // First-Time EEPROM Self-Registration and Discovery; Ping MUX && EEPROM
    //     // DebugJson Heartbeat
    //     if(errlev[m] == I2CIP_ERR_NONE) {
    //       I2CIP::rebuildTree(Serial);
    //     }
    //   }
    // }

    // // 2. Device IO
    // I2CIP::DeviceGroup* dg_sht45 = modules[0]->operator[]("SHT45");
    // if(dg_sht45 != nullptr && dg_sht45->getNumDevices() > 0) {
    //   for(uint8_t i = 0; i < dg_sht45->getNumDevices(); i++) {
    //     Device* d = dg_sht45->getDevice(i);
    //     if(d == nullptr) continue;
    //     // 2A. Input
    //     if(d->getInput() != nullptr) {
    //       i2cip_errorlevel_t errlev = d->getInput()->get();
    //       if(errlev != I2CIP_ERR_NONE) {
    //         DebugJson::telemetryJsonString(d->getInput()->getLastRX(), d->getInput()->cacheToString());
    //       }
    //     }
    //   }
    // }

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
  #else
    // 0. Prep Args: Onboard 7SEG (Default to Snake Mode)
    i2cip_ht16k33_mode_t seg_mode = SEG_SNAKE;
    i2cip_ht16k33_data_t seg_data = { .h = fps };
    i2cip_args_io_t args_7seg = { .a = nullptr, .s = &seg_data, .b = &seg_mode };

    // 1. MUX & EEPROM: Ping and load devices
    errlev[MODULE] = modules[MODULE]->operator()(); // First-Time EEPROM Self-Registration and Discovery; Ping MUX && EEPROM
    if(errlev[MODULE] == I2CIP_ERR_NONE) {

      // 2. DebugJson Heartbeat & Display FPS
      DebugJson::revision(0, Serial); // sends revision
      seg_mode = SEG_UINT;

      #ifdef I2CIP_DEBUG_SERIAL
        // Debug Serial Output
        DEBUG_DELAY();
        I2CIP_DEBUG_SERIAL.println(I2CIP::devicetree.toString());
        I2CIP_DEBUG_SERIAL.println(modules[MODULE]->toString());
        DEBUG_DELAY();
      #endif

      // 3. Prep Args: LCD
      String msg = String(fps) + "Hz I2CIP\n"; // Further append will be on second line
      i2cip_jhd1313_args_t rgb = JHD1313::randomRGBLCD();
      i2cip_args_io_t args_lcd = { .a = nullptr, .s = nullptr, .b = &rgb };
      
      // Prep Args: 7SEG
      // seg_data.f = NAN; // Will produce "NUL.L" on display instead of temperature
      // args_7seg.s = &seg_data.f;

      // SHT45 - Average and print to LCD
      I2CIP::DeviceGroup* dg_sht45 = modules[MODULE]->operator[]("SHT45");
      if(dg_sht45 != nullptr && dg_sht45->getNumDevices() > 0) {
        // AVERAGES
        state_sht45_t th = {0.0f, 0.0f}; uint8_t c = 0;
        for(uint8_t i = 0; i < dg_sht45->getNumDevices(); i++) {
          SHT45* d = (SHT45*)(dg_sht45->getDevice(i));
          if(d == nullptr) continue;

          i2cip_errorlevel_t errlev_sht45 = modules[MODULE]->operator()<SHT45>(d->getFQA(), true, _i2cip_args_io_default, DebugJsonBreakpoints);
          if(errlev_sht45 != I2CIP_ERR_NONE) continue;

          th.temperature += d->getCache().temperature;
          th.humidity += d->getCache().humidity;
          c++;

          DebugJson::telemetry(d->getLastRX(), d->getCache().temperature, "temperature");
          DebugJson::telemetry(d->getLastRX(), d->getCache().humidity, "humidity");
        }

        if(c > 0) {

          th.temperature /= c; th.humidity /= c;

          msg += '[' + String(th.temperature, 1) + "C, " + String(th.humidity, 1) + "%]"; // The double-space might cut off the % but who cares
          seg_data.f = th.temperature;
          seg_mode = SEG_1F;

        } else {
          // LCD Default
          msg += "SHT45 EIO :(";
          args_lcd.s = &msg;
          do_lcd = true;
        }

        // If messages match and neither temperature nor humidity have changed (and are not NaN), skip LCD
        if(!isnan(temphum.temperature) && !isnan(temphum.humidity) && (abs(th.temperature - temphum.temperature) < EPSILON_TEMPERATURE && abs(th.humidity - temphum.humidity) < EPSILON_HUMIDITY) && ((long int)millis() - last_lcd < LCD_REFRESH_MAX)) {
          // MAIN_DEBUG_SERIAL.println(F("[I2CIP | LCD SKIP; RGB ONLY]"));
          do_lcd = false;
        } else {
          temphum.temperature = th.temperature; temphum.humidity = th.humidity;
          args_lcd.s = &msg;
          do_lcd = true;
        }
      } else {
        // LCD Default
        msg += "SHT45 ENOENT :(";
        args_lcd.s = &msg;
        do_lcd = true;
      }

      // Nunchuck
      I2CIP::DeviceGroup* dg_nunchuck = modules[MODULE]->operator[]("NUNCHUCK");
      if(dg_nunchuck != nullptr && dg_nunchuck->getNumDevices() > 0) {
        Nunchuck* d = (Nunchuck*)(dg_nunchuck->getDevice(0)); // Use first device
        if(d != nullptr && d->getInput() != nullptr) {

          i2cip_errorlevel_t errlev_nunchuck = modules[MODULE]->operator()<Nunchuck>(d->getFQA(), true, _i2cip_args_io_default, DebugJsonBreakpoints);

          if(errlev_nunchuck != I2CIP_ERR_NONE) {
            nunchuck_sum += ((float)(d->getCache().x) - 127.f) / 255.f; // += cache.y;

            DebugJson::telemetry(d->getLastRX(), d->getCache().x, "joy_x");
            DebugJson::telemetry(d->getLastRX(), d->getCache().y, "joy_y");
            DebugJson::telemetry(d->getLastRX(), d->getCache().c, "button_c");
            DebugJson::telemetry(d->getLastRX(), d->getCache().z, "button_z");
          } else {
            nunchuck_sum = 0.0f;
          }
        } else {
          nunchuck_sum = 0.0f;
        }
      } else {
        nunchuck_sum = 0.0f;
      }
      
      I2CIP::DeviceGroup* dg_rotary = modules[MODULE]->operator[]("SEESAW");
      if(dg_rotary != nullptr && dg_rotary->getNumDevices() > 0) {
        RotaryEncoder* rotary = (RotaryEncoder*)(dg_rotary->getDevice(0)); // Use first device
        if(rotary != nullptr) {
          i2cip_errorlevel_t errlev_rotary = modules[MODULE]->operator()<Seesaw>(rotary->getFQA(), true, _i2cip_args_io_default, DebugJsonBreakpoints);
          if(errlev_rotary == I2CIP_ERR_NONE) {
            i2cip_rotaryencoder_t cache = rotary->getCache();

            // PeaPod Stuff
            if(cache.button == HIGH) { // Debounce to pulse ONCE on button press
              if(!flag_debounce){
                // Invert and set debounce
                peapod_toggle = !peapod_toggle;
                flag_debounce = true;
              }
              peapod_out = true;
            } else {
              // Reset debounce
              flag_debounce = false;
              peapod_out = false;
            }
            uint32_t angle_ticks = 0;

            // Toggle also selects PWM control target
            if(peapod_toggle) {
              if(cache.button) rotary_zero1 = cache.encoder; // Set zero-point
              angle_ticks = -(cache.encoder - rotary_zero1); // Invert rotation
              peapod_pwm1 = (uint8_t)((float)(Seesaw::_encoderDegrees(angle_ticks)) / 360 * 255); // 0-3840 (256 ticks) -> 0-255
            } else {
              if(cache.button) rotary_zero2 = cache.encoder; // Set zero-point
              angle_ticks = -(cache.encoder - rotary_zero2); // Invert rotation
              peapod_pwm2 = (uint8_t)((float)(Seesaw::_encoderDegrees(angle_ticks)) / 360 * 255); // 0-3840 (256 ticks) -> 0-255
            }
            uint32_t position = Seesaw::_encoderDegrees(angle_ticks, 9999); // 27 * 360 = 9720; Maximum revolutions on 4 digit display

            if(args_lcd.s != nullptr) {
              if(peapod_toggle) {
                *((String*)args_lcd.s) += " B";
              } else {
                *((String*)args_lcd.s) += " R";
              }
            }

            // DebugJson::telemetry(rotary->getLastRX(), cache.encoder, "encoder");
            DebugJson::telemetry(rotary->getLastRX(), position, "position");
            DebugJson::telemetry(rotary->getLastRX(), cache.button, "button");
            // DebugJson::telemetry(rotary->getLastRX(), ((float)peapod_pwm1 / 255), "peapod_pwm1");
            // DebugJson::telemetry(rotary->getLastRX(), ((float)peapod_pwm2 / 255), "peapod_pwm2");
            // DebugJson::telemetry(rotary->getLastRX(), peapod_toggle, "peapod_toggle");
            // DebugJson::telemetry(rotary->getLastRX(), peapod_out, "peapod_out");
            
            // Overwrite 7Seg Args
            seg_mode = SEG_UINT;
            seg_data.h = position + ((unsigned)nunchuck_sum); // Now h is the active member
          }
        }

      }
      //  else {
      //   rotary_zero = 0;
      // }

      if(do_lcd || (long int)millis() - last_rgb > RGB_REFRESH_MAX) {
      // LCD: On-Module SHT45 Status Display
        i2cip_errorlevel_t errlev_lcd = modules[MODULE]->operator()<JHD1313>(fqa_lcd, true, args_lcd, DebugJsonOut);
        if(errlev_lcd == I2CIP_ERR_NONE && fps != 0) {
          if(args_lcd.s != nullptr) last_lcd = millis();
          last_rgb = millis();
        }
      }
    } else {
      seg_mode = SEG_SNAKE;
    }
    // 7SEG: Off-Module (MCU Featherwing/Shield) Multi-Status Display: Rotary, else SHT45, else Snake
    i2cip_errorlevel_t errlev_7seg = modules[MODULE]->operator()<HT16K33>(ht16k33->getFQA(), true, args_7seg, DebugJsonOut);

    // PeaPod Stuff
    controlPin<PEAPOD_PIN_TOGGLE>(peapod_toggle);
    controlPin<PEAPOD_PIN_OUT>(peapod_out);

    controlPWM<PEAPOD_PIN_PWM1>(peapod_pwm1);
    controlPWM<PEAPOD_PIN_PWM2>(peapod_pwm2);
  #endif

  #ifdef FSM_STATE_H_
  cycle.set(cycle.get()++); // Set cycle and do conditionals & callbacks(?)
  #endif

  #ifdef CYCLE_DELAY
  delay(CYCLE_DELAY);
  #endif

  // DEBUG PRINT: CYCLE COUNT, FPS, and ERRLEV
  unsigned long delta = millis() - last;
  fps += 1000.f / max(1.f, (float)delta);
  fps /= 2;
  // MAIN_DEBUG_SERIAL.print(F("[I2CIP | CYCLE "));
  // MAIN_DEBUG_SERIAL.print(cycle);
  // MAIN_DEBUG_SERIAL.print(F(" | "));
  // MAIN_DEBUG_SERIAL.print(fps);
  // MAIN_DEBUG_SERIAL.print(F(" FPS | 0x"));
  // MAIN_DEBUG_SERIAL.print(max(errlev, max(errlev_7seg, errlev_lcd)), HEX);
  // MAIN_DEBUG_SERIAL.println(F("]"));

}

#endif