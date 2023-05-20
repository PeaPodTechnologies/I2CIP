#include <i2cip/zcm.h>

#include <zcm/zcm.h>
#include <zcm/transport/third-party/embedded/arduino/transport_arduino_serial.h>

namespace ZCMSerial {

  bool begin(uint32_t baud) {
    zcm_trans_t *zcm_transport = zcm_trans_arduino_serial_create(baud, timestamp_now, NULL);
    assert(zcm_transport);
    return (zcm_init_trans(&zcm, zcm_transport) == 0);
  }

  bool write(const char* channel, const uint8_t* buf, uint32_t buflen) {
    zcm_publish(&zcm, channel, buf, buflen);
  }
}

zcm_t zcm;

uint64_t timestamp_now(void* usr) {
  return micros();
}