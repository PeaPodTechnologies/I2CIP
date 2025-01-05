#ifndef UNIT_TEST
#define UNIT_TEST 1
#define IS_MAIN 1

#include <Arduino.h>

#include <DebugJson.h>

#include "../test/config.h"

// #define MAIN_DEBUG_SERIAL Serial
#define MAIN_DEBUG_SERIAL DebugJsonOut
// #define CYCLE_DELAY 1000
#define EPSILON_TEMPERATURE 0.5f
#define EPSILON_HUMIDITY 5.0f // 0.11f
#define LCD_REFRESH_MAX 2000

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

void setup(void) {
  Serial.begin(115200);

  m = new TestModule(WIRENUM, MODULE);

  // i2cip_errorlevel_t errlev = m->operator()();

  // NOTE: module.eeprom == nullptr; and module["eeprom"] == nullptr
  ht16k33 = new HT16K33(WIRENUM, I2CIP_MUX_NUM_FAKE, I2CIP_MUX_BUS_FAKE, "SEVENSEG");

  // Each operator() call adds the Device to I2CIP::devicegroups and I2CIP::devicetree
  m->operator()<HT16K33>(ht16k33, true, _i2cip_args_io_default, DebugJsonBreakpoints);
  // m->operator()<PCA9685>(fqa_pca9685, false, _i2cip_args_io_default, DebugJsonBreakpoints);
  m->operator()<SHT45>(fqa_sht45, false, _i2cip_args_io_default, DebugJsonBreakpoints);
  m->operator()<JHD1313>(fqa_lcd, false, _i2cip_args_io_default, DebugJsonBreakpoints);
  m->operator()<RotaryEncoder>(fqa_rotary, false, _i2cip_args_io_default, DebugJsonBreakpoints);
}

String timestampToString(unsigned long t) {
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

i2cip_errorlevel_t errlev;
uint8_t cycle = 0;
unsigned long last = 0;
uint32_t fps = 0; // Something other than zero

// bool temphum = false;
state_sht45_t temphum = {NAN, NAN};
int32_t rotary_zero = 0;
unsigned long last_lcd = LCD_REFRESH_MAX; // Fixes first-frame bug
void loop(void) {
  last = millis();
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
    // Prep Args: LCD
    String msg = String(fps) + "Hz " + String("T+") + timestampToString(last) + "\n"; // Further append will be on second line
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
      state_sht45_t th = {0.0f, 0.0f};
      for(uint8_t i = 0; i < entry->value.numdevices; i++) {
        th.temperature += ((SHT45*)(entry->value.devices[i]))->getCache().temperature;
        th.humidity += ((SHT45*)(entry->value.devices[i]))->getCache().humidity;
      }
      th.temperature /= entry->value.numdevices; th.humidity /= entry->value.numdevices;

      msg += '[' + String(th.temperature, 1) + "C, " + String(th.humidity, 1) + "%]"; // The double-space might cut off the % but who cares
      seg_data.f = th.temperature;
      seg_mode = SEG_1F;

      // If messages match and neither temperature nor humidity have changed (and are not NaN), skip LCD
      if(!isnan(temphum.temperature) && !isnan(temphum.humidity) && (abs(th.temperature - temphum.temperature) < EPSILON_TEMPERATURE && abs(th.humidity - temphum.humidity) < EPSILON_HUMIDITY) && ((long int)millis() - last_lcd < LCD_REFRESH_MAX)) {
        MAIN_DEBUG_SERIAL.println(F("[I2CIP | LCD SKIP; RGB ONLY]"));
      } else {
        temphum.temperature = th.temperature; temphum.humidity = th.humidity;
        args_lcd.s = &msg;
      }
    } else {
      // LCD Default
      msg += "SHT45 ERR 0x" + String(errlev_sht45, HEX) + " :(";
      args_lcd.s = &msg;
    }

    // LCD: On-Module SHT45 Status Display
    i2cip_errorlevel_t errlev_lcd = m->operator()<JHD1313>(fqa_lcd, true, args_lcd, DebugJsonBreakpoints);
    if(errlev_lcd == I2CIP_ERR_NONE && args_lcd.s != nullptr && fps != 0) {
      last_lcd = millis();
    }

    RotaryEncoder* rotary = (RotaryEncoder*)m->operator[](fqa_rotary);
    if(rotary != nullptr) {

      // seg_data.f = NAN; // Will produce "NUL.L" on display instead of temperature
      i2cip_errorlevel_t errlev_rotary = m->operator()<RotaryEncoder>(fqa_rotary, true, _i2cip_args_io_default, DebugJsonOut);

      if(errlev_rotary == I2CIP_ERR_NONE) {
        i2cip_rotaryencoder_t cache = rotary->getCache();
        if(cache.button) rotary_zero = cache.encoder;
        uint32_t angle_ticks = -(cache.encoder - rotary_zero); // Invert rotation
        uint32_t position = Seesaw::_encoderDegrees(angle_ticks, 9999); // 27 * 360 = 9720; Maximum revolutions on 4 digit display

        // Overwrite 7Seg Args
        seg_mode = SEG_UINT;
        seg_data.h = position; // Now h is the active member
      }
      //  else {
      //   if(errlev == I2CIP_ERR_NONE) { // still SHT45 error
      //     mode = SEG_1F;
      //     data.f = temphum.temperature;
      //     hargs.s = &data.f;
      //   }
      // }
    }
  } else {
    seg_mode = SEG_SNAKE;
  }
  // 7SEG: Off-Module (MCU Featherwing/Shield) Multi-Status Display: Rotary, else SHT45, else Snake
  i2cip_errorlevel_t errlev_7seg = m->operator()<HT16K33>(ht16k33, true, args_7seg, DebugJsonBreakpoints);

  // DEBUG PRINT: CYCLE COUNT, FPS, and ERRLEV
  unsigned long delta = millis() - last;
  fps = (uint32_t)round(1000.f / (delta < 10 ? 10 : delta)); // 100FPS max
  // MAIN_DEBUG_SERIAL.print(F("[I2CIP | CYCLE "));
  // MAIN_DEBUG_SERIAL.print(cycle);
  // MAIN_DEBUG_SERIAL.print(F(" | "));
  // MAIN_DEBUG_SERIAL.print(fps);
  // MAIN_DEBUG_SERIAL.print(F(" FPS | 0x"));
  // MAIN_DEBUG_SERIAL.print(max(errlev, max(errlev_7seg, errlev_lcd)), HEX);
  // MAIN_DEBUG_SERIAL.println(F("]"));

  cycle++;

  #ifdef CYCLE_DELAY
  delay(CYCLE_DELAY);
  #endif
}

#endif