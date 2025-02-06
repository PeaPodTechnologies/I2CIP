#ifndef UNIT_TEST
#define UNIT_TEST 1
#define IS_MAIN 1

#include <Arduino.h>

#include <DebugJson.h> // Debugging JSON Serial Outputs (Breakpoints, Telemetry, etc.)
#include <chrono.h> // Uncomment for Finite State Machine (w/ Timer)

#include "../test/config.h"

// #define MAIN_DEBUG_SERIAL Serial
#define MAIN_DEBUG_SERIAL DebugJsonOut
#define CYCLE_DELAY 20 // Max FPS 100Hz
#define EPSILON_TEMPERATURE 0.5f
#define EPSILON_HUMIDITY 2.0f // 0.11f
#define LCD_REFRESH_MAX 1000
#define RGB_REFRESH_MAX 1 // Near-instantaneous

// PeaPod Stuff
#define PEAPOD_PIN_TOGGLE 33 // Multipurpose; Interpreted input from rotary button (Debounced; Toggle)
#define PEAPOD_PIN_OUT 32 // Multipurpose; Direct input from rotary button
#define PEAPOD_PIN_PWM1 15 // Multipurpose; Interpreted output from rotary knob (0-360 -> 0-255)
#define PEAPOD_PIN_PWM2 14 // Multipurpose; Interpreted output from rotary knob (0-360 -> 0-255)

// Module* m;  // to be initialized in setup()
TestModule* m = nullptr;
char idbuffer[10];

HT16K33 *ht16k33 = nullptr;

void crashout(void) {
  if(m != nullptr) m->operator()<HT16K33>(ht16k33, true); // Display "FAIL" on seven segment
  while(true) { // Blinks
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}

i2cip_fqa_t fqa_sht45 = createFQA(WIRENUM, MODULE, 0, I2CIP_SHT45_ADDRESS);
// i2cip_fqa_t fqa_pca9685 = createFQA(WIRENUM, MODULE, 1, I2CIP_PCA9685_ADDRESS);
i2cip_fqa_t fqa_lcd = createFQA(WIRENUM, MODULE, 1, I2CIP_JHD1313_ADDRESS);
i2cip_fqa_t fqa_rotary = createFQA(WIRENUM, MODULE, 0, I2CIP_SEESAW_ADDRESS);
i2cip_fqa_t fqa_nunchuck = createFQA(WIRENUM, MODULE, 0, I2CIP_NUNCHUCK_ADDRESS);

void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  while(!Serial) { digitalWrite(LED_BUILTIN, HIGH); delay(100); digitalWrite(LED_BUILTIN, LOW); delay(100); }

  // PeaPod PinModes
  pinMode(PEAPOD_PIN_TOGGLE, OUTPUT);
  pinMode(PEAPOD_PIN_OUT, OUTPUT);
  pinMode(PEAPOD_PIN_PWM1, OUTPUT);
  pinMode(PEAPOD_PIN_PWM2, OUTPUT);

  m = new TestModule(WIRENUM, MODULE);

  // i2cip_errorlevel_t errlev = m->operator()();

  // NOTE: module.eeprom == nullptr; and module["eeprom"] == nullptr
  ht16k33 = new HT16K33(WIRENUM, I2CIP_MUX_NUM_FAKE, I2CIP_MUX_BUS_FAKE, "SEVENSEG");

  // Each operator() call adds the Device to I2CIP::devicegroups and I2CIP::devicetree
  m->operator()<HT16K33>(ht16k33, true, _i2cip_args_io_default, DebugJsonBreakpoints);
  // m->operator()<PCA9685>(fqa_pca9685, false, _i2cip_args_io_default, DebugJsonBreakpoints);
  m->operator()<SHT45>(fqa_sht45, false, _i2cip_args_io_default, DebugJsonBreakpoints);
  m->operator()<JHD1313>(fqa_lcd, false, _i2cip_args_io_default, DebugJsonBreakpoints);
}

/** LOOP SECTION */

// LOOP HELPERS

String printableTimestampString(unsigned long t) {
  uint8_t hours = (t / 1000) / 3600;
  uint8_t minutes = ((t / 1000) - (hours * 3600)) / 60;
  uint8_t seconds = ((t / 1000) - (hours * 3600) - (minutes * 60));
  uint8_t ms = (t % 1000);
  String msg;
  if(hours > 0) {
    if(hours < 10) msg += "0";
    msg += String(hours) + ":";
  }
  if(minutes > 0) {
    if(minutes < 10) msg += "0";
    msg += String(minutes) + ":";
    if(seconds < 10) msg += "0";
  }
  if(hours == 0 && minutes < 10) {
    // msg += String(seconds + (ms / 1000.f), 3);
    msg += String(seconds + (ms / 1000.f), 2);
    if(minutes == 0) msg += "s";
  } else {
    msg += String(seconds);
  }
  return msg;
}

#define I2CIP_JHD1313_BRIGHTNESS 0.65f // Keep it low for good contrast and readability
i2cip_jhd1313_args_t randomRGBLCD(void) {
  uint32_t lcdrgb = random(0, 0xFFFFFF); // Random color
  // Serial.print("Random RGB: 0x"); Serial.println(lcdrgb, HEX);
  uint8_t red = ((lcdrgb >> 16) & 0xFF); uint8_t green = ((lcdrgb >> 8) & 0xFF); uint8_t blue = (lcdrgb & 0xFF); // Extract RGB

  // float brightness = (float)lcdrgb / 0xFFFFFF; // Normal Brightness
  float brightness = (red / (float)0xFF) * 0.299f + (green / (float)0xFF) * 0.587f + (blue / (float)0xFF) * 0.114f; // Relative Spectral Luminance
  float _scale = I2CIP_JHD1313_BRIGHTNESS / brightness; // Raw scalings

  float _red = _scale * red / 255.f; float _green = _scale * green / 255.f; float _blue = _scale * blue / 255.f; // Normalize
  float scale = 1.f / max(1.f, max(_red, max(_green, _blue))); // Adjusted scaling

  red = (uint8_t)min(255.f, _red * scale * 255);
  green = (uint8_t)min(255.f, _green * scale * 255);
  blue = (uint8_t)min(255.f, _blue * scale * 255);
  // Serial.print("LCD RGB: 0x");
  // if(red < 0x10) Serial.print('0');
  // Serial.print(red, HEX);
  // if(green < 0x10) Serial.print('0');
  // Serial.print(green, HEX);
  // if(blue < 0x10) Serial.print('0');
  // Serial.println(blue, HEX);
  i2cip_jhd1313_args_t lcdargs = { .r = red, .g = green, .b = blue };
  return lcdargs;
}

// PING AND SOFT IF NOT FOUND, OTHERWISE RESULT
template <class C> i2cip_errorlevel_t handleDevice(C*& d, const i2cip_fqa_t& fqa, i2cip_args_io_t args = _i2cip_args_io_default) {
  if(d == nullptr) {
  // Not Given, Try to Find
    d = (C*)m->operator[](fqa);
    if(d != nullptr) {
      // FOUND!
      return m->operator()<C>(d, true, args, DebugJsonOut);
    } else {
      // Not Found, Try to Ping
      i2cip_errorlevel_t errlev = m->operator()<C>(fqa, false, _i2cip_args_io_default, DebugJsonBreakpoints);
      return ((errlev == I2CIP_ERR_HARD) ? I2CIP_ERR_HARD : I2CIP_ERR_SOFT);
    }
  }
  else return m->operator()<C>(d, true, args, DebugJsonOut);
}


// LOOP GLOBALS

i2cip_errorlevel_t errlev;
uint8_t cycle = 0;
unsigned long last = 0;
uint32_t fps = 0; // Something other than zero

// bool temphum = false;
state_sht45_t temphum = {NAN, NAN};
int32_t rotary_zero1 = 0;
int32_t rotary_zero2 = 0;
unsigned long last_lcd = LCD_REFRESH_MAX; unsigned long last_rgb = RGB_REFRESH_MAX; // Fixes first-frame bug
bool do_lcd = true;
float nunchuck_sum = 0.0f;

// PeaPod Stuff
uint8_t peapod_pwm1 = 0;
uint8_t peapod_pwm2 = 0;
bool peapod_toggle = false;
bool peapod_out = false;
bool flag_debounce = false;

void loop(void) {
  last = millis();
  #ifdef FSM_STATE_H_
    FSM::Chronos.set(last); // Update timer
  #endif

  // switch(checkModule(WIRENUM, MODULE)) {
  //   case I2CIP_ERR_HARD:
  //     // delete m;
  //     m = nullptr;
  //     return;
  //   case I2CIP_ERR_SOFT:
  //     if (!initializeModule(WIRENUM, MODULE)) { /*delete m;*/ m = nullptr; return; }
  //     break;
  //   default:
  //     errlev = updateModule(WIRENUM, MODULE);
  //     break;
  // }

  i2cip_ht16k33_mode_t seg_mode = SEG_SNAKE;
  i2cip_ht16k33_data_t seg_data = { .h = fps };
  i2cip_args_io_t args_7seg = { .a = nullptr, .s = &seg_data, .b = &seg_mode };

  // Module Errlev
  errlev = m->operator()(); // First-Time EEPROM Self-Registration and Discovery; Ping MUX && EEPROM
  if(errlev == I2CIP_ERR_NONE) {
    // DebugJson Heartbeat
    DebugJson::revision(0, Serial); // sends revision
    // Prep Args: LCD
    String msg = String(fps) + "Hz " + String("T+") + printableTimestampString(last) + "\n"; // Further append will be on second line
    i2cip_jhd1313_args_t rgb = randomRGBLCD();
    i2cip_args_io_t args_lcd = { .a = nullptr, .s = nullptr, .b = &rgb };
    
    // Prep Args: 7SEG
    seg_mode = SEG_UINT;
    // seg_data.f = NAN; // Will produce "NUL.L" on display instead of temperature
    // args_7seg.s = &seg_data.f;

    // SHT45
    i2cip_errorlevel_t errlev_sht45 = m->operator()<SHT45>(fqa_sht45, true, _i2cip_args_io_default, DebugJsonOut);

    // Average and print to LCD
    HashTableEntry<I2CIP::DeviceGroup&>* entry = I2CIP::devicegroups["SHT45"];
    if(errlev_sht45 == I2CIP_ERR_NONE && entry != nullptr && entry->value.numdevices > 0) {
      // AVERAGES
      state_sht45_t th = {0.0f, 0.0f}; uint8_t c = 0;
      for(uint8_t i = 0; i < entry->value.numdevices; i++) {
        SHT45* d = (SHT45*)(entry->value.devices[i]);
        if(d == nullptr) continue;
        DebugJson::telemetry(d->getLastRX(), d->getCache().temperature, "temperature");
        DebugJson::telemetry(d->getLastRX(), d->getCache().humidity, "humidity");
        th.temperature += ((SHT45*)(entry->value.devices[i]))->getCache().temperature;
        th.humidity += ((SHT45*)(entry->value.devices[i]))->getCache().humidity;
        c++;
      }
      th.temperature /= c; th.humidity /= c;

      msg += '[' + String(th.temperature, 1) + "C, " + String(th.humidity, 1) + "%]"; // The double-space might cut off the % but who cares
      seg_data.f = th.temperature;
      seg_mode = SEG_1F;

      // If messages match and neither temperature nor humidity have changed (and are not NaN), skip LCD
      if(!isnan(temphum.temperature) && !isnan(temphum.humidity) && (abs(th.temperature - temphum.temperature) < EPSILON_TEMPERATURE && abs(th.humidity - temphum.humidity) < EPSILON_HUMIDITY) && ((long int)millis() - last_lcd < LCD_REFRESH_MAX)) {
        MAIN_DEBUG_SERIAL.println(F("[I2CIP | LCD SKIP; RGB ONLY]"));
        do_lcd = false;
      } else {
        temphum.temperature = th.temperature; temphum.humidity = th.humidity;
        args_lcd.s = &msg;
        do_lcd = true;
      }
    } else {
      // LCD Default
      msg += "SHT45 ERR 0x" + String(errlev_sht45, HEX) + " :(";
      args_lcd.s = &msg;
      do_lcd = true;
    }

    Nunchuck* nunchuck = nullptr;
    if(handleDevice<Nunchuck>(nunchuck, fqa_nunchuck, _i2cip_args_io_default) == I2CIP_ERR_NONE && nunchuck != nullptr) {
      i2cip_nunchuck_t cache = nunchuck->getCache();

      nunchuck_sum += ((float)cache.x - 127.f) / 255.f; // += cache.y;

      DebugJson::telemetry(nunchuck->getLastRX(), cache.x, "joy_x");
      DebugJson::telemetry(nunchuck->getLastRX(), cache.y, "joy_y");
      DebugJson::telemetry(nunchuck->getLastRX(), cache.c, "button_c");
      DebugJson::telemetry(nunchuck->getLastRX(), cache.z, "button_z");
    } else {
      nunchuck_sum = 0.0f;
    }

    RotaryEncoder* rotary = nullptr;
    if(handleDevice<RotaryEncoder>(rotary, fqa_rotary, _i2cip_args_io_default) == I2CIP_ERR_NONE && rotary != nullptr) {
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
      uint32_t angle_ticks =0;

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

      DebugJson::telemetry(rotary->getLastRX(), cache.encoder, "encoder");
      DebugJson::telemetry(rotary->getLastRX(), position, "position");
      DebugJson::telemetry(rotary->getLastRX(), cache.button, "button");
      DebugJson::telemetry(rotary->getLastRX(), ((float)peapod_pwm1 / 255), "peapod_pwm1");
      DebugJson::telemetry(rotary->getLastRX(), ((float)peapod_pwm2 / 255), "peapod_pwm2");
      DebugJson::telemetry(rotary->getLastRX(), peapod_toggle, "peapod_toggle");
      DebugJson::telemetry(rotary->getLastRX(), peapod_out, "peapod_out");

      if(args_lcd.s != nullptr) {
        if(peapod_toggle) {
          *((String*)args_lcd.s) += " B";
        } else {
          *((String*)args_lcd.s) += " R";
        }
      }

      // Overwrite 7Seg Args
      seg_mode = SEG_UINT;
      seg_data.h = position + ((unsigned)nunchuck_sum); // Now h is the active member
    }
    //  else {
    //   rotary_zero = 0;
    // }

    if(do_lcd || (long int)millis() - last_rgb > RGB_REFRESH_MAX) {
    // LCD: On-Module SHT45 Status Display
      i2cip_errorlevel_t errlev_lcd = m->operator()<JHD1313>(fqa_lcd, true, args_lcd, DebugJsonOut);
      if(errlev_lcd == I2CIP_ERR_NONE && fps != 0) {
        if(args_lcd.s != nullptr) last_lcd = millis();
        last_rgb = millis();
      }
    }
  } else {
    seg_mode = SEG_SNAKE;
  }
  // 7SEG: Off-Module (MCU Featherwing/Shield) Multi-Status Display: Rotary, else SHT45, else Snake
  i2cip_errorlevel_t errlev_7seg = m->operator()<HT16K33>(ht16k33, true, args_7seg, DebugJsonOut);

  // PeaPod Stuff
  if(peapod_toggle) {
    digitalWrite(PEAPOD_PIN_TOGGLE, HIGH);
  } else {
    digitalWrite(PEAPOD_PIN_TOGGLE, LOW);
  }
  if(peapod_out) {
    digitalWrite(PEAPOD_PIN_OUT, HIGH);
  } else {
    digitalWrite(PEAPOD_PIN_OUT, LOW);
  }
  analogWrite(PEAPOD_PIN_PWM1, peapod_pwm1);
  analogWrite(PEAPOD_PIN_PWM2, peapod_pwm2);

  cycle++;

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