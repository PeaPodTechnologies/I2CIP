#ifndef I2CIP_ZCM_H_
#define I2CIP_ZCM_H_

#define I2CIP_ZCM_BAUD_DEFAULT 115200

namespace ZCMSerial {

  bool begin(uint32_t baud = I2CIP_ZCM_BAUD_DEFAULT);

  bool write(const char* channel, const uint8_t* buf, uint32_t buflen);
}

#endif