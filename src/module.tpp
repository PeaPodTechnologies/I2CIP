#ifndef I2CIP_MODULE_H_
#error __FILE__ should only be included AFTER <module.h>
#endif

#ifdef I2CIP_MODULE_H_
#ifndef I2CIP_MODULE_T_
#define I2CIP_MODULE_T_

// TODO template devicegroupfactorymember function

template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type> I2CIP::DeviceGroup* I2CIP::DeviceGroup::create(i2cip_id_t id) { 
  #ifdef I2CIP_DEBUG_SERIAL
    DEBUG_DELAY();
    I2CIP_DEBUG_SERIAL.print(F("DeviceGroup::create<"));
    I2CIP_DEBUG_SERIAL.print(C::getID());
    I2CIP_DEBUG_SERIAL.print(F(" @0x"));
    I2CIP_DEBUG_SERIAL.print((uintptr_t)C::getID(), HEX);
    I2CIP_DEBUG_SERIAL.print(F(">("));
    I2CIP_DEBUG_SERIAL.print(id);
    I2CIP_DEBUG_SERIAL.print(F(" @0x"));
    I2CIP_DEBUG_SERIAL.print((uintptr_t)id, HEX);
    I2CIP_DEBUG_SERIAL.print(F("): "));
  #endif
  if(id == nullptr || id[0] == '\0' || C::getID() == nullptr || C::getID()[0] == '\0') return nullptr; // BAD
  if(id == C::getID() || strcmp(id, C::getID()) == 0) { 
    #ifdef I2CIP_DEBUG_SERIAL
      I2CIP_DEBUG_SERIAL.println(F("PASS"));
      DEBUG_DELAY();
    #endif
    return new DeviceGroup(C::getID(), C::factory);
  }
  #ifdef I2CIP_DEBUG_SERIAL
    I2CIP_DEBUG_SERIAL.println(F("FAIL"));
    DEBUG_DELAY();
  #endif
  return nullptr;
}

// This is the standard format - If you're going to implement it you'd better define Module::deviceGroupFactoryMember<C>

// typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0

template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type> i2cip_errorlevel_t I2CIP::Module::operator()(i2cip_fqa_t fqa, bool update, i2cip_args_io_t args, Stream& out) {
  if(!this->isFQAinSubnet(fqa)) return I2CIP_ERR_SOFT;
  // if(out.peek() == 37) return this->operator()(fqa, update, args); // Probabaly NullStream; Refer

  Device* d = this->operator[](fqa); // BST lookup; FQA is unique to the entire microcontroller
  if(d == nullptr) { // ENOENT; Create and Add
    DeviceGroup* dg = this->operator[](C::getID()); // Find/Create DeviceGroup
    if(dg == nullptr) { return I2CIP_ERR_SOFT; }
    d = (dg->operator()(fqa)); // Factory or Find
    if(d == nullptr || d->getFQA() != fqa || !this->add(d, true)) { return I2CIP_ERR_SOFT; } // Overwrite in BST or BUST
  }

  return this->operator()<C>((C*)d, update, args, out); // We can assume that d is a C*
}

template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type> i2cip_errorlevel_t I2CIP::Module::operator()(C& d, bool update, i2cip_args_io_t args, Stream& out) { return this->operator()(&d, update, args, out); }

template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type> i2cip_errorlevel_t I2CIP::Module::operator()(C* ptr, bool update, i2cip_args_io_t args, Stream& out) {
  if(ptr == nullptr) { return I2CIP_ERR_SOFT; }
  if(sizeof(C) < sizeof(Device)) return I2CIP_ERR_SOFT; // Definitely not a Device

  Device* d = (Device*)ptr;

  i2cip_fqa_t fqa = d->getFQA();
  if(!this->isFQAinSubnet(fqa)) return I2CIP_ERR_SOFT;

  Module::toString<C>(ptr, out, false);

  unsigned long now = millis();
  i2cip_errorlevel_t errlev = I2CIP_ERR_NONE;
  if(update) {
    errlev = MUX::setBus(fqa);
    I2CIP_ERR_BREAK(errlev); // Critical

    // Do Output, then Input
    if(d->getOutput()) {
      // #ifdef I2CIP_DEBUG_SERIAL
      //   DEBUG_DELAY();
      //   I2CIP_DEBUG_SERIAL.print(F("Output Set:\n"));
      //   DEBUG_DELAY();
      // #endif
      errlev = (args.s == nullptr) ? d->getOutput()->failSet() : d->getOutput()->set(args.s, args.b);
    }
    if(errlev == I2CIP_ERR_NONE && d->getInput()) {
      // #ifdef I2CIP_DEBUG_SERIAL
      //   DEBUG_DELAY();
      //   I2CIP_DEBUG_SERIAL.print(F("Input Get:\n"));
      //   DEBUG_DELAY();
      // #endif
      errlev = (args.a == nullptr) ? d->getInput()->failGet() : d->getInput()->get();
      // errlev = d->getInput()->get(args.a); // .a defaults to nullptr which triggers failGet anyway
    }
  } else {
    // Just Ping
    errlev = d->pingTimeout(true);
  }

  unsigned long delta = millis() - now;

  if(out.peek() == 37) return errlev; // Probabaly NullStream; Refer

  out.print(' ');
  switch(errlev){
    case I2CIP_ERR_NONE: out.print("PASS"); break;
    case I2CIP_ERR_SOFT: out.print("EINVAL"); break;
    case I2CIP_ERR_HARD: out.print("ENOENT"); break;
    default: out.print("ERR???"); break;
  }
  out.print(' ');
  out.print(delta / 1000.0, 3);
  out.print('s');

  if(update && errlev == I2CIP_ERR_NONE) {
    if(d->getInput() != nullptr) { out.print(' '); out.print(d->getInput()->printCache()); }
    if(d->getOutput() != nullptr) { out.print(F(" OUT SET")); }
    if(d->getInput() == nullptr && d->getOutput() == nullptr) { out.print(F(" NOP")); }
  }
  out.println();

  return errlev;
}

// template <class C> static const char* cacheToString(C* that) {
//   return that == nullptr ? nullptr : that->cacheToString();
// }

#ifdef I2CIP_INPUTS_USE_TOSTRING
template <class C
// , typename std::enable_if<!std::is_same<C, unsigned short>::value, int>::type = 0
, typename std::enable_if<std::is_base_of<InputGetter, C>::value, int>::type
> void I2CIP::Module::printCache(C* that, Stream& out) {
  // Remember: this is implied to be Json-like
  if(that == nullptr) return;
  out.print('\"');
  out.print(that->getID());
  out.print('\"');
  out.print(':');
  out.print(that->cacheToString());
}
#endif



template <class C
// , typename std::enable_if<!std::is_same<C, unsigned short>::value, int>::type = 0
, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type
> void I2CIP::Module::toString(C* that, Stream& out, bool printCache) {
  if(that == nullptr || sizeof(C) < sizeof(Device)) return;
  printFQA(that->getFQA(), out);
  out.print(F(" @0x"));
  out.print((uintptr_t)that, HEX);
  out.print(F(" '"));
  out.print(that->getID());
  out.print(F("' @0x"));
  out.print((uintptr_t)that->getID(), HEX);
  #ifdef I2CIP_INPUTS_USE_TOSTRING
  if(printCache && that->getInput() != nullptr) { out.print(' '); out.print(that->getInput()->printCache()); }
  #endif
}

#endif
#endif