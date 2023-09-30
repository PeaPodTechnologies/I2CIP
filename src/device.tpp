#ifndef I2CIP_DEVICE_H_
#error __FILE__ should only be included AFTER <device.h>
// #include <comparators.h>
#endif

#ifdef I2CIP_DEVICE_H_

#ifndef I2CIP_DEVICE_T_
#define I2CIP_DEVICE_T_

using I2CIP::InputInterface;
using I2CIP::OutputInterface;
using I2CIP::IOInterface;

template <typename G, typename A> InputInterface<G, A>::operator i2cip_itype_t() const { return I2CIP_ITYPE_INPUT; }

template <typename S, typename B> OutputInterface<S, B>::operator i2cip_itype_t() const { return I2CIP_ITYPE_OUTPUT; }

template <typename G, typename A, typename S, typename B> IOInterface<G, A, S, B>::operator i2cip_itype_t() const { return I2CIP_ITYPE_IO; }

#endif
#endif