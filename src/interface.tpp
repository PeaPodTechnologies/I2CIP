#ifndef I2CIP_INTERFACE_H_
#error __FILE__ should only be included AFTER <interface.h>
// #include <comparators.h>
#endif

#ifdef I2CIP_INTERFACE_H_

#ifndef I2CIP_INTERFACE_T_
#define I2CIP_INTERFACE_T_

using I2CIP::i2cip_errorlevel_t;
using I2CIP::Device;
using I2CIP::InputGetter;
using I2CIP::OutputSetter;
using I2CIP::InputInterface;
using I2CIP::OutputInterface;
using I2CIP::IOInterface;
using I2CIP::i2cip_enable_if_t;
using I2CIP::i2cip_is_convertible_t;

template <typename G, typename A> InputInterface<G, A>::InputInterface(Device* device) { if(device != nullptr) device->setInput(this); }

template <typename G, typename A> InputInterface<G, A>::~InputInterface() { }

template <typename G, typename A> const G& InputInterface<G, A>::getCache(void) const { return this->cache; }

template <typename G, typename A> void InputInterface<G, A>::setCache(G value) { this->cache = value; }

template <typename G, typename A> void InputInterface<G, A>::clearCache(void) {
  // #ifdef I2CIP_DEBUG_SERIAL
  //   DEBUG_DELAY();
  //   I2CIP_DEBUG_SERIAL.print(F("InputInterface::clearCache() Not Implemented; Nothing Done\n"));
  //   DEBUG_DELAY();
  // #endif
}

template <typename G, typename A> void InputInterface<G, A>::setArgsA(A args) { this->argsA = args; }

template <typename G, typename A> A InputInterface<G, A>::getArgsA(void) const { return this->argsA; }

template <typename G, typename A> i2cip_errorlevel_t InputInterface<G, A>::get(const void* args) { 
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.println(F("INPUT GET"));
  //   if(args == &InputGetter::failptr_get) I2CIP_DEBUG_SERIAL.print(F("fail"));
  //   else if(args == nullptr) I2CIP_DEBUG_SERIAL.print(F("null"));
  //   else { 
  //     // I2CIP_DEBUG_SERIAL.print(*(A* const)args); // This line is EXPERIMENTAL
  //     I2CIP_DEBUG_SERIAL.print(F(" @0x"));
  //     I2CIP_DEBUG_SERIAL.print((ptrdiff_t)args, HEX);
  //   }
  //   I2CIP_DEBUG_SERIAL.print(F(")\n"));
    DEBUG_DELAY();
  #endif

  if (args == &InputGetter::failptr_get) this->clearCache();
  G temp = this->cache;

  if(!this->argsAset) { this->argsA = this->getDefaultA(); this->argsAset = true; }
  A arg = (args == &InputGetter::failptr_get) ? this->getDefaultA() : ((args == nullptr) ? this->getArgsA() : *(A* const)args);

  i2cip_errorlevel_t errlev = this->get(temp, arg);

  // If successful, update last cache
  if(errlev == I2CIP::i2cip_errorlevel_t::I2CIP_ERR_NONE) { 
    this->clearCache(); this->cache = temp; this->argsA = arg; this->lastrx = millis();
    // #ifdef I2CIP_DEBUG_SERIAL
    //   DEBUG_DELAY();
    //   I2CIP_DEBUG_SERIAL.println(F("Cache Set"));
    //   // I2CIP_DEBUG_SERIAL.println(temp); // This line is EXPERIMENTAL
    //   DEBUG_DELAY();
    // #endif
  }
  return errlev;
}

#ifdef FSM_STATE_H_
using I2CIP::CallbackArgsInput;
using I2CIP::makeCallbackArgsInput;

template <typename G, typename A> CallbackArgsInput<G, A> InputInterface<G, A>::asCallbackArgsInput(void) const { return CallbackArgsInput<G, A>(*this); }

template <typename G, typename A> CallbackArgsInput<G, A>::CallbackArgsInput(const InputInterface<G, A>& source) : FSM::CallbackArgs((const void*)&source, FSM::ARGS_VOID_PTR), source(&source) { }
template <typename G, typename A> const void* CallbackArgsInput<G, A>::getPointer(FSM::args_type_t* typeOut) const {
  if(typeOut != nullptr) {
    (*typeOut) = FSM::ARGS_VOID_PTR;
  }

  if(this->source == nullptr) {
    return nullptr;
  }

  return (const void*)&this->source->getCache();
}

template <typename G, typename A> template <typename T, typename i2cip_enable_if_t<i2cip_is_convertible_t<G, T>::value, int>::type> bool CallbackArgsInput<G, A>::getValue(T& out) const {
  if(this->source == nullptr) return false;
  out = static_cast<T>(this->source->getCache());
  return true;
}

template <typename G, typename A> template <typename T, typename i2cip_enable_if_t<!i2cip_is_convertible_t<G, T>::value, int>::type> bool CallbackArgsInput<G, A>::getValue(T& out) const {
  (void)out;
  return false;
}

template <typename G, typename A> bool CallbackArgsInput<G, A>::get(bool& out) const { return this->getValue(out); }
template <typename G, typename A> bool CallbackArgsInput<G, A>::get(char& out) const { return this->getValue(out); }
template <typename G, typename A> bool CallbackArgsInput<G, A>::get(short& out) const { return this->getValue(out); }
template <typename G, typename A> bool CallbackArgsInput<G, A>::get(int& out) const { return this->getValue(out); }
template <typename G, typename A> bool CallbackArgsInput<G, A>::get(long& out) const { return this->getValue(out); }
template <typename G, typename A> bool CallbackArgsInput<G, A>::get(unsigned char& out) const { return this->getValue(out); }
template <typename G, typename A> bool CallbackArgsInput<G, A>::get(unsigned short& out) const { return this->getValue(out); }
template <typename G, typename A> bool CallbackArgsInput<G, A>::get(unsigned int& out) const { return this->getValue(out); }
template <typename G, typename A> bool CallbackArgsInput<G, A>::get(unsigned long& out) const { return this->getValue(out); }
template <typename G, typename A> bool CallbackArgsInput<G, A>::get(float& out) const { return this->getValue(out); }
template <typename G, typename A> bool CallbackArgsInput<G, A>::get(double& out) const { return this->getValue(out); }
template <typename G, typename A> I2CIP::CallbackArgsInput<G, A> I2CIP::makeCallbackArgsInput(const I2CIP::InputInterface<G, A>& source) { return I2CIP::CallbackArgsInput<G, A>(source); }

#ifdef FSM_TIMER_H_
template <typename G, typename A> FSM::IntervalCallback* addIntervalWithInput(FSM::Chronograph& chronograph, const FSM::fsm_timestamp_t& delta, typename FSM::ConditionalCallback<FSM::fsm_timestamp_t>::cb_compvalrefargs_t cb, const InputInterface<G, A>* input, bool invert) {
  return addIntervalWithInput(chronograph, FSM::FSM_KEY_NULL, delta, 0, cb, input, invert);
}

template <typename G, typename A> FSM::IntervalCallback* addIntervalWithInput(FSM::Chronograph& chronograph, FSM::fsm_key_t key, const FSM::fsm_timestamp_t& delta, typename FSM::ConditionalCallback<FSM::fsm_timestamp_t>::cb_compvalrefargs_t cb, const InputInterface<G, A>* input, bool invert) {
  return addIntervalWithInput(chronograph, key, delta, 0, cb, input, invert);
}

template <typename G, typename A> FSM::IntervalCallback* addIntervalWithInput(FSM::Chronograph& chronograph, const FSM::fsm_timestamp_t& delta, const FSM::fsm_timestamp_t& phase, typename FSM::ConditionalCallback<FSM::fsm_timestamp_t>::cb_compvalrefargs_t cb, const InputInterface<G, A>* input, bool invert) {
  return addIntervalWithInput(chronograph, FSM::FSM_KEY_NULL, delta, phase, cb, input, invert);
}

template <typename G, typename A> FSM::IntervalCallback* addIntervalWithInput(FSM::Chronograph& chronograph, FSM::fsm_key_t key, const FSM::fsm_timestamp_t& delta, const FSM::fsm_timestamp_t& phase, typename FSM::ConditionalCallback<FSM::fsm_timestamp_t>::cb_compvalrefargs_t cb, const InputInterface<G, A>* input, bool invert) {
  if(input == nullptr) {
    return nullptr;
  }

  return chronograph.addInterval(key, delta, phase, cb, new CallbackArgsInput<G, A>(*input), invert, true);
}
#endif
#endif

template <typename S, typename B> OutputInterface<S, B>::OutputInterface(Device* device) { if(device != nullptr) device->setOutput(this); }

template <typename S, typename B> OutputInterface<S, B>::~OutputInterface() { }

template <typename S, typename B> void OutputInterface<S, B>::setValue(S value) { this->value = value; }

// template <typename S, typename B> void OutputInterface<S, B>::resetFailsafe(void) {
  // #ifdef I2CIP_DEBUG_SERIAL
  //   DEBUG_DELAY();
  //   I2CIP_DEBUG_SERIAL.print(F("OutputInterface::resetFailsafe() Not Implemented; Nothing Done\n"));
  //   DEBUG_DELAY();
  // #endif
// }

template <typename S, typename B> S OutputInterface<S, B>::getValue(void) const { return this->value; }

template <typename S, typename B> void OutputInterface<S, B>::setArgsB(B args) { this->argsB = args; }

template <typename S, typename B> B OutputInterface<S, B>::getArgsB(void) const { return this->argsB; }

template <typename S, typename B> i2cip_errorlevel_t OutputInterface<S, B>::set(const void* value, const void* args) {
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.println(F("OUTPUT SET"));
    // if(value == &OutputSetter::failptr_set) I2CIP_DEBUG_SERIAL.print(F("fail"));
    // else if(value == nullptr) I2CIP_DEBUG_SERIAL.print(F("null"));
    // else { 
    //   I2CIP_DEBUG_SERIAL.print(*(S* const)value);
    //   I2CIP_DEBUG_SERIAL.print(F(" @0x"));
    //   I2CIP_DEBUG_SERIAL.print((uintptr_t)value, HEX);
    // }
    // I2CIP_DEBUG_SERIAL.print(F(", "));
    // if(args == &OutputSetter::failptr_set) I2CIP_DEBUG_SERIAL.print(F("fail"));
    // else if(args == nullptr) I2CIP_DEBUG_SERIAL.print(F("null"));
    // else { 
    //   I2CIP_DEBUG_SERIAL.print(*(B* const)args);
    //   I2CIP_DEBUG_SERIAL.print(F(" @0x"));
    //   I2CIP_DEBUG_SERIAL.print((uintptr_t)args, HEX);
    // }
    // I2CIP_DEBUG_SERIAL.print(F(")\n"));
    DEBUG_DELAY();
  #endif

  // If fail, reset to failsafe value
  if (value == &OutputSetter::failptr_set) this->resetFailsafe();
  // 1. If `set` value is not given, repeat last action
  S val = ((value == nullptr || value == &OutputSetter::failptr_set) ? this->getValue() : *(S* const)value);

  if(!this->argsBset) { this->argsB = this->getDefaultB(); this->argsBset = true; }

  // 2. If `set` args are not given, use last args 
  B arg = (args == &OutputSetter::failptr_set) ? this->getDefaultB() : ((args == nullptr) ? this->getArgsB() : *(B* const)args);

  // 3. Attempt `set`
  i2cip_errorlevel_t errlev = this->set(val, arg);

  // 4. If successful, update cached `value` and `args`
  if(errlev == I2CIP::i2cip_errorlevel_t::I2CIP_ERR_NONE) { this->value = val; this->argsB = arg; this->lasttx = millis(); };

  return errlev;
}

template <typename G, typename A, typename S, typename B> IOInterface<G, A, S, B>::IOInterface(Device* device) : InputInterface<G, A>(device), OutputInterface<S, B>(device) { }

template <typename G, typename A, typename S, typename B> IOInterface<G, A, S, B>::~IOInterface() { }

#endif
#endif