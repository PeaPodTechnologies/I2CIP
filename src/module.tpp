#ifndef I2CIP_MODULE_H_
#error __FILE__ should only be included AFTER <module.h>
#endif

#ifdef I2CIP_MODULE_H_
#ifndef I2CIP_MODULE_T_
#define I2CIP_MODULE_T_

#include "debug.h"

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

template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type> i2cip_errorlevel_t I2CIP::Module::operator()(i2cip_fqa_t fqa, bool update, i2cip_args_io_t args, Print& out) {
  // if(!this->isFQAinSubnet(fqa)) return I2CIP_ERR_SOFT; // This is handled by the operator
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

template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type> i2cip_errorlevel_t I2CIP::Module::operator()(i2cip_id_t id, bool update, i2cip_args_io_t args, Print& out) {
  i2cip_errorlevel_t errlev = I2CIP_ERR_NONE;
  DeviceGroup* dg = this->operator[](id);
  if(dg == nullptr) { return I2CIP_ERR_SOFT; } // ENOENT
  for(uint8_t i = 0; i < I2CIP_DEVICES_PER_GROUP; i++) {
    if(dg->devices[i] == nullptr) { continue; }
    i2cip_fqa_t fqa = dg->devices[i]->getFQA();
    if(I2CIP_FQA_SEG_MODULE(fqa) == I2CIP_MUX_NUM_FAKE || I2CIP_FQA_SEG_MUXBUS(fqa) == I2CIP_MUX_BUS_FAKE) { continue; } // Skip non-MUX devices
    // if(!this->isFQAinSubnet(fqa)) { continue; } // Skip devices not in subnet // This is handled by the operator
    i2cip_errorlevel_t err = this->operator()<C>((C*)dg->devices[i], update, args, out);
    if(err > errlev) { errlev = err; } // TODO: Something better
  }
  return errlev;
}

template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type> i2cip_errorlevel_t I2CIP::Module::operator()(C& d, bool update, i2cip_args_io_t args, Print& out) { return this->operator()(&d, update, args, out); }

template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type> i2cip_errorlevel_t I2CIP::Module::operator()(C* ptr, bool update, i2cip_args_io_t args, Print& out) {
  if(ptr == nullptr) { return I2CIP_ERR_SOFT; }
  if(sizeof(C) < sizeof(Device)) return I2CIP_ERR_SOFT; // Definitely not a Device

  Device* d = (Device*)ptr;

  i2cip_fqa_t fqa = d->getFQA();
  if(!this->isFQAinSubnet(fqa)) return I2CIP_ERR_SOFT;

  String m = fqaToString(fqa);
  m += " '"; m += d->getID(); m += "' ";

  unsigned long now = millis();
  i2cip_errorlevel_t errlev = I2CIP_ERR_NONE;
  if(update) {
    errlev = MUX::setBus(fqa);
    I2CIP_ERR_BREAK(errlev); // Critical

    // Do Output, then Input
    if(d->getOutput() != nullptr) {
      // #ifdef I2CIP_DEBUG_SERIAL
      //   DEBUG_DELAY();
      //   I2CIP_DEBUG_SERIAL.print(F("Output Set:\n"));
      //   DEBUG_DELAY();
      // #endif
      errlev = d->set(args.s, args.b);
    }
    if(errlev == I2CIP_ERR_NONE && (d->getInput() != nullptr)) {
      // #ifdef I2CIP_DEBUG_SERIAL
      //   DEBUG_DELAY();
      //   I2CIP_DEBUG_SERIAL.print(F("Input Get:\n"));
      //   DEBUG_DELAY();
      // #endif
      errlev = d->get(args.a);
      // errlev = d->getInput()->get(args.a); // .a defaults to nullptr which triggers failGet anyway
    }
  } else {
    // Just Ping
    errlev = d->pingTimeout(true);
  }

  unsigned long delta = millis() - now;

  // if(out.peek() == 37) return errlev; // Probabaly NullStream; Refer

  m += (' ');
  switch(errlev){
    case I2CIP_ERR_NONE: m += "PASS"; break;
    case I2CIP_ERR_SOFT: m += "EINVAL"; break;
    case I2CIP_ERR_HARD: m += "EIO"; break;
    default: out.print("ERR???"); break;
  }
  m += (' ');
  m += String(delta / 1000.0, 3);
  m += ('s');

  if(update && errlev == I2CIP_ERR_NONE) {
    if(d->getInput() != nullptr) {
      m += (F(" INPGET ")); 
      #ifdef I2CIP_INPUTS_USE_TOSTRING
        m += (d->getInput()->printCache());
      #endif
    }
    if(d->getOutput() != nullptr) {
      m += (F(" OUTSET "));
      #ifdef I2CIP_OUTPUTS_USE_TOSTRING
        m += ((args.s == nullptr) ? "NULL" : (d->getOutput()->valueToString()));
      #endif
    }
    // if(d->getInput() == nullptr && d->getOutput() == nullptr) { m += (F(" NOP")); }
  }
  out.println(m);

  return errlev;
}

// template <class C> static const char* cacheToString(C* that) {
//   return that == nullptr ? nullptr : that->cacheToString();
// }

#ifdef I2CIP_INPUTS_USE_TOSTRING
template <class C, typename std::enable_if<std::is_base_of<InputGetter, C>::value, int>::type = 0> void I2CIP::Module::printDevice(C* that, Print& out) {
  // Remember: this is implied to be Json-like
  if(that == nullptr || sizeof(C) < sizeof(Device)) return;

  String m = fqaToString(that->getFQA());
  m += F(" @0x");
  m += String((uintptr_t)that->getFQA(), HEX);
  m += F(" '");
  m += that->getID();
  m += F("' @0x");
  m += String((uintptr_t)that->getID(), HEX);
  #ifdef I2CIP_INPUTS_USE_TOSTRING
  if(that->getInput() != nullptr) { m += ' '; m += (that->getInput()->printCache()); }
  #endif
  out.println(m);
}
#endif



template <class C, typename std::enable_if<std::is_base_of<Device, C>::value, int>::type = 0> String I2CIP::Module::deviceCacheToString(C* that) {
  if(that == nullptr || sizeof(C) < sizeof(Device)) return String();
  String m = "\"";
  m += that->getID();
  m += "\":";
  m += that->cacheToString();
  return m;
}

#endif
#endif