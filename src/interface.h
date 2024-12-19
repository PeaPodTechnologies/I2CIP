#ifndef I2CIP_INTERFACE_H_
#define I2CIP_INTERFACE_H_

#include <device.h>

namespace I2CIP {

  /**
   * An I2CIP peripheral used for input/state "getting".
   * @param G type used for "get" variable
   * @param A type used for "get" arguments
   **/
  template <typename G, typename A> class InputInterface : public InputGetter {
    private:
      G cache;  // Last RECIEVED value
      A argsA;  // Last passed arguments

      bool argsAset = false;
    protected:
      void setCache(G value);
      void setArgsA(A args);
      
      /**
       * Gets the default arguments used for the "get" operation.
       * A constant reference.
       * To be implemented by the child class.
      */
      virtual const A& getDefaultA(void) const = 0;
    public:
      InputInterface(Device* device);
      virtual ~InputInterface() = 0;

      i2cip_errorlevel_t get(const void* args = nullptr) override;

      /**
       * Gets the last recieved value.
      */
      const G& getCache(void) const;

      /**
       * Sets the cache to the default "zero" value.
       * To be implemented by the child class.
      */
      virtual void clearCache(void);

      /**
       * Gets the arguments used for the last "get" operation.
      */
      A getArgsA(void) const;

      /**
       * Gets the input device's state.
       **/
      virtual i2cip_errorlevel_t get(G& dest, const A& args) { return I2CIP_ERR_HARD; } // Unimplemented; Disable this device
  };

  /**
   * An I2CIP peripheral used for output/state "setting".
   * @param S type used for "set" value
   * @param B type used for "set" arguments
   **/
  template <typename S, typename B> class OutputInterface : public OutputSetter {
    private:
      S value;  // Last SET value (not PASSED value)
      B argsB;  // Last passed arguments

      bool argsBset = false;
    protected:
      void setValue(S value);
      void setArgsB(B args);

      /**
       * Gets the default arguments used for the "set" operation.
       * To be implemented by the child class.
      */
      virtual const B& getDefaultB(void) const = 0;
    public:
      OutputInterface(Device* device);
      
      virtual ~OutputInterface() = 0;

      i2cip_errorlevel_t set(const void* value = nullptr, const void* args = nullptr) override;

      /**
       * Gets the arguments used for the last "set" operation.
      */
      B getArgsB(void) const;

      /**
       * Gets the last set value.
      */
      S getValue(void) const;

      /**
       * Gets the default "zero"/off-state value.
       * To be implemented by the child class.
      */
      virtual void resetFailsafe(void);

      /**
       * Sets the output device's state.
       **/
      virtual i2cip_errorlevel_t set(const S& value, const B& args) { return I2CIP_ERR_HARD; } // Unimplemented; Disable this device
  };

  /**
   * An I2CIP peripheral used for input/state "getting" as well as output/state "setting".
   * @param G type used for "get" variable
   * @param A type used for "get" arguments
   * @param S type used for "set" value
   * @param B type used for "set" arguments
   **/
  template <typename G, typename A, typename S, typename B> class IOInterface : public InputInterface<G, A>, public OutputInterface<S, B> {
    public:
      IOInterface(Device* device);
      virtual ~IOInterface() = 0;
  };

}

#include <interface.tpp>

#endif