/*
 * Accessor.h
 *
 *  Created on: Jun 09, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_ACCESSOR_H
#define CHIMERATK_ACCESSOR_H

#include <string>
#include <atomic>

#include <mtca4u/RegisterPath.h>

#include "Flags.h"
#include "Application.h"
#include "ApplicationModule.h"

namespace ChimeraTK {

  using namespace mtca4u;

  class Application;

  /*********************************************************************************************************************/

  // stupid temporary base class for accessors, which is not templated @todo TODO replace with proper class structure
  class AccessorBase {
    public:
      virtual ~AccessorBase() {}

      /** Return if the accessor is properly initialised. It is initialised if it was constructed passing the pointer
       *  to an implementation (a NDRegisterAccessor), it is not initialised if it was constructed only using the
       *  placeholder constructor without arguments. */
      virtual bool isInitialised() const = 0;

      /** Use a ProcessVariable as implementation. */
      virtual void useProcessVariable(const boost::shared_ptr<TransferElement> &var) = 0;

      /* Obtain the type info of the UserType */
      virtual const std::type_info& getValueType() const = 0;

      /** Obtain direction of the accessor */
      virtual VariableDirection getDirection() const = 0;

      /** Obtain the update mode of the accessor */
      virtual UpdateMode getUpdateMode() const = 0;

      /* Obtain the unit of the variable */
      virtual const std::string& getUnit() const = 0;
      
      /** Read an input variable. In case of an output variable, an exception will be thrown. This function will block
       *  the calling thread until the variable has been read. If the UpdateMode::push flag has been set when creating
       *  the accessor, this function will wait until a new value has been provided to the variable. If a new value is
       *  already available before calling this function, the function will be non-blocking and lock-free. */
      virtual void read() = 0;
      
      /** Non-blocking read. Will return whether a new value was obtained. For pull-type varibales, always true is
       *  returned, independently whether the value was changed or not. */
      virtual bool readNonBlocking() = 0;
      
      /** Write an output variable. In case of an input variable, an exception will be thrown. */
      virtual void write() = 0;
      
    protected:

      friend class ApplicationModule;
  };

  /*********************************************************************************************************************/

  /** An invalid instance which can be used e.g. for optional arguments passed by reference */
  class InvalidAccessor : public AccessorBase {
    public:
      constexpr InvalidAccessor() {}
      ~InvalidAccessor() {}
      bool isFeeding() {std::terminate();}
      bool isInitialised() const {std::terminate();}
      void useProcessVariable(const boost::shared_ptr<ProcessVariable> &) {std::terminate();}
      const std::type_info& getValueType() const {std::terminate();}
      VariableDirection getDirection() const {std::terminate();}
      UpdateMode getUpdateMode() const {std::terminate();}
  };

  /*********************************************************************************************************************/

  template< typename UserType >
  class Accessor : public AccessorBase {

    public:

      /** The default accessor takes no arguments and leaves the accessor uninitialised. It will be dysfunctional
       *  until it is properly initialised using connectTo(). */
      Accessor(Module *owner, const std::string &name, VariableDirection direction, std::string unit, UpdateMode mode)
      : _owner(owner), _name(name), _direction(direction), _unit(unit), _mode(mode), node{*this}
      {
        owner->registerAccessor(this);
      }

      VariableDirection getDirection() const {return _direction;}

      UpdateMode getUpdateMode() const {return _mode;}

      const std::string& getUnit() const {return _unit;}

      const std::type_info& getValueType() const {
        return typeid(UserType);
      }

      operator const VariableNetworkNode&() const {
        return node;
      }

      operator VariableNetworkNode&() {
        return node;
      }

      /** Connect with node */
/*      VariableNetworkNode& operator<<(const VariableNetworkNode &other) {
        return node.operator<<(other);
      } */
      VariableNetworkNode operator>>(VariableNetworkNode other) {
        return node.operator>>(other);
      }

    protected:

      Module *_owner;
      std::string _name;
      VariableDirection _direction;
      std::string _unit;
      UpdateMode _mode;

      VariableNetworkNode node;


  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_ACCESSOR_H */
