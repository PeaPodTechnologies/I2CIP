#ifndef I2CIP_GUARANTEE_H_
#define I2CIP_GUARANTEE_H_

#define I2CIP_USE_GUARANTEES 1

#include <Arduino.h>

#define I2CIP_GUARANTEE_NULL 0
#define I2CIP_GUARANTEE_DEVICE 1
#define I2CIP_GUARANTEE_INPUT 2
#define I2CIP_GUARANTEE_OUTPUT 3

namespace I2CIP {

  class Device; template <typename G, typename A> class InputInterface; template <typename S, typename B> class OutputInterface;

  template <typename C> struct _guarantee {
    typedef uint32_t i2cip_guarantee_t;
  };

  // If this method exists on a pointer and returns the expected 
  template <typename C> class Guarantee {
    // These are handed out
    public:
      typedef typename I2CIP::_guarantee<C>::i2cip_guarantee_t i2cip_guarantee_t;
      // template <> typedef uint32_t i2cip_guarantee_t;
    protected:
      template <typename T> T* that(void) { return ((Guarantee<T>*)(this->that()))->that();}
      virtual C* that(void) = 0; // Will be pointed to `this` down the line
      template <typename T> i2cip_guarantee_t guarantee() { return this->that<T>()->guarantee(); }
      virtual i2cip_guarantee_t guarantee() = 0; // To be implemented in each child class - check this against known
    public:
      // static bool guarantee() { return _Guarantee::guarantee<C>(that()); }
      operator bool(void) { return (this->that<C>() == nullptr) ? false : this->guarantee<C>() == I2CIP::_guarantee<C>()); }
  };

  // Used for guaranteeing the validity of inputs, outputs,  (useful for pointer dereferencing during runtime device operations)
  template <typename C> Guarantee<C>::i2cip_guarantee_t _guarantee<C>(); // We first define it here

}

#define I2CIP_GUARANTEE_DEFINE(CLASS, G) \
  template <> I2CIP::Guarantee<CLASS>::i2cip_guarantee_t I2CIP::_guarantee<CLASS>() {\
    return G;\
  }

// OH NO THEYRE DISTINGUISHED BY RETURN TYPE ALONE - IS THIS NECESSARILY A PROBLEM?
#define I2CIP_CLASS_USE_GUARANTEE(CLASS, G)\
  protected:\
    CLASS* that(void) { return (CLASS*)this; }\
  public:\
    I2CIP::Guarantee<CLASS>::i2cip_guarantee_t guarantee() { return G; }

#endif