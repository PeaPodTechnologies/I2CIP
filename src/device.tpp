#ifndef I2CIP_DEVICE_H_
#error __FILE__ should only be included AFTER <device.h>
// #include <comparators.h>
#endif

#ifdef I2CIP_DEVICE_H_

#ifndef I2CIP_DEVICE_T_
#define I2CIP_DEVICE_T_

using I2CIP::i2cip_errorlevel_t;
using I2CIP::InputGetter;
using I2CIP::OutputSetter;
using I2CIP::InputInterface;
using I2CIP::OutputInterface;
using I2CIP::IOInterface;

template <typename G, typename A> InputInterface<G, A>::InputInterface(Device* device) { device->setInput(this); }

template <typename G, typename A> G InputInterface<G, A>::getCache(void) const { return this->cache; }

template <typename G, typename A> void InputInterface<G, A>::setCache(G value) { this->cache = value; }

template <typename G, typename A> A InputInterface<G, A>::getArgsA(void) const { return this->argsA; }

template <typename G, typename A> i2cip_errorlevel_t InputInterface<G, A>::get(const void* args) { 
  G temp = this->cache;
  A arg = (args == nullptr) ? this->getDefaultA() : *(A* const)args;
  i2cip_errorlevel_t errlev = this->get(temp, arg);

  // If successful, update last cache
  if(errlev == I2CIP::i2cip_errorlevel_t::I2CIP_ERR_NONE) { this->cache = temp; this->argsA = arg; };
  return errlev;
}

template <typename S, typename B> OutputInterface<S, B>::OutputInterface(Device* device) { device->setOutput(this); }

template <typename S, typename B> S OutputInterface<S, B>::getValue(void) const { return this->value; }

template <typename S, typename B> B OutputInterface<S, B>::getArgsB(void) const { return this->argsB; }

template <typename S, typename B> i2cip_errorlevel_t OutputInterface<S, B>::set(const void* value, const void* args) {
  // 1. If `set` value is not given, use failsafe
  S val = (value == nullptr) ? this->getFailsafe() : *(S* const)value;

  // 2. If `set` args are not given, use default
  B arg = (args == nullptr) ? this->getDefaultB() : *(B* const)args;

  // 3. Attempt `set`
  i2cip_errorlevel_t errlev = this->set(val, arg);

  // 4. If successful, update cached `value` and `args`
  if(errlev == I2CIP::i2cip_errorlevel_t::I2CIP_ERR_NONE) { this->value = val; this->argsB = arg; };

  return errlev;
}

template <typename G, typename A, typename S, typename B> IOInterface<G, A, S, B>::IOInterface(Device* device) : InputInterface<G, A>(device), OutputInterface<S, B>(device) { }

#endif
#endif