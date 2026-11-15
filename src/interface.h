#ifndef I2CIP_INTERFACE_H_
#define I2CIP_INTERFACE_H_

#include <Arduino.h>

#include "device.h"

namespace I2CIP {
  template <bool B, typename T = void> struct i2cip_enable_if_t { };
  template <typename T> struct i2cip_enable_if_t<true, T> { typedef T type; };
  template <typename T> T&& i2cip_declval_t(void);

  template <typename From, typename To> class i2cip_is_convertible_t {
    private:
      static void accept(To);

      template <typename F, typename = decltype(accept(i2cip_declval_t<F>()))> static char check(int);
      template <typename F> static int check(...);

    public:
      enum { value = sizeof(check<From>(0)) == sizeof(char) };
  };

  #ifdef FSM_STATE_H_
  template <typename G, typename A> class CallbackArgsInput;
  #endif

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
      InputInterface() : InputInterface(nullptr) { }
      InputInterface(Device* device);
      virtual ~InputInterface() = 0;

      typedef G i2cip_input_type_t;
      typedef A i2cip_input_args_t;

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

      #ifdef FSM_STATE_H_
      CallbackArgsInput<G, A> asCallbackArgsInput(void) const;
      #endif
  };

  #ifdef FSM_STATE_H_
  template <typename G, typename A> class CallbackArgsInput : public FSM::CallbackArgs {
    private:
      const InputInterface<G, A>* source = nullptr;

      template <typename T, typename i2cip_enable_if_t<i2cip_is_convertible_t<G, T>::value, int>::type = 0> bool getValue(T& out) const;
      template <typename T, typename i2cip_enable_if_t<!i2cip_is_convertible_t<G, T>::value, int>::type = 0> bool getValue(T& out) const;

    public:
      CallbackArgsInput(const InputInterface<G, A>& source);

      const void* getPointer(FSM::args_type_t* typeOut = nullptr) const override;

      bool get(bool& out) const override;
      bool get(char& out) const override;
      bool get(short& out) const override;
      bool get(int& out) const override;
      bool get(long& out) const override;
      bool get(unsigned char& out) const override;
      bool get(unsigned short& out) const override;
      bool get(unsigned int& out) const override;
      bool get(unsigned long& out) const override;
      bool get(float& out) const override;
      bool get(double& out) const override;
  };

  template <typename G, typename A> CallbackArgsInput<G, A> makeCallbackArgsInput(const InputInterface<G, A>& source);

  #ifdef FSM_TIMER_H_
  template <typename G, typename A> FSM::IntervalCallback* addIntervalWithInput(FSM::Chronograph& chronograph, const FSM::fsm_timestamp_t& delta, typename FSM::ConditionalCallback<FSM::fsm_timestamp_t>::cb_compvalrefargs_t cb, const InputInterface<G, A>* input, bool invert = false);
  template <typename G, typename A> FSM::IntervalCallback* addIntervalWithInput(FSM::Chronograph& chronograph, FSM::fsm_key_t key, const FSM::fsm_timestamp_t& delta, typename FSM::ConditionalCallback<FSM::fsm_timestamp_t>::cb_compvalrefargs_t cb, const InputInterface<G, A>* input, bool invert = false);
  template <typename G, typename A> FSM::IntervalCallback* addIntervalWithInput(FSM::Chronograph& chronograph, const FSM::fsm_timestamp_t& delta, const FSM::fsm_timestamp_t& phase, typename FSM::ConditionalCallback<FSM::fsm_timestamp_t>::cb_compvalrefargs_t cb, const InputInterface<G, A>* input, bool invert = false);
  template <typename G, typename A> FSM::IntervalCallback* addIntervalWithInput(FSM::Chronograph& chronograph, FSM::fsm_key_t key, const FSM::fsm_timestamp_t& delta, const FSM::fsm_timestamp_t& phase, typename FSM::ConditionalCallback<FSM::fsm_timestamp_t>::cb_compvalrefargs_t cb, const InputInterface<G, A>* input, bool invert = false);
  #endif
  #endif

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
      OutputInterface() : OutputInterface(nullptr) { }
      OutputInterface(Device* device);
      
      virtual ~OutputInterface() = 0;

      typedef S i2cip_output_type_t;
      typedef B i2cip_output_args_t;

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
      virtual void resetFailsafe(void) = 0;
      // virtual void resetFailsafe(void);

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

#include "interface.tpp"

#endif