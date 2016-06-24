/*
 * Application.h
 *
 *  Created on: Jun 07, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_APPLICATION_H
#define CHIMERATK_APPLICATION_H

#include <mutex>

#include <mtca4u/DeviceBackend.h>
#include <ControlSystemAdapter/DevicePVManager.h>

#include "ApplicationException.h"
#include "VariableNetwork.h"
#include "Flags.h"
#include "ImplementationAdapter.h"

namespace ChimeraTK {

  class ApplicationModule;
  class AccessorBase;
  class InvalidAccessor;
  class VariableNetwork;

  template<typename UserType>
  class Accessor;

  template<typename UserType>
  class DeviceAccessor;

  class Application {

    public:

      /** Constructor: the first instance will be created explicitly by the control system adapter code. Any second
       *  instance is not allowed, thus calling the constructor multiple times will throw an exception.
       *  Design note: We are not using a true singleton pattern, since Application is an abstract base class. The
       *  actual instance is created as a static variable.
       *  The application developer should derive his application from this class and implement the initialise()
       *  function only. */
      Application(const std::string& name);

      /** The destructor will remove the global pointer to the instance and allows creating another instance
       *  afterwards. This is mostly useful for writing tests, as it allows to run several applications sequentially
       *  in the same executable. Note that any ApplicationModules etc. owned by this Application are no longer
       *  valid after destroying the Application and must be destroyed as well (or at least no longer used). */
      virtual ~Application();

      /** Set the process variable manager. This will be called by the control system adapter initialisation code. */
      void setPVManager(boost::shared_ptr<mtca4u::DevicePVManager> const &processVariableManager) {
        _processVariableManager = processVariableManager;
      }

      /** Obtain the process variable manager. */
      boost::shared_ptr<mtca4u::DevicePVManager> getPVManager() {
        return _processVariableManager;
      }

      /** Initialise and run the application */
      void run();

      /** Instead of running the application, just initialise it and output the published variables to an XML file. */
      void generateXML();

      /** Obtain instance of the application. Will throw an exception if called before the instance has been
       *  created by the control system adapter. */
      static Application& getInstance() {
        // @todo TODO Throw the exception if instance==nullptr !!!
        return *instance;
      }

      /** Output the connections requested in the initialise() function to std::cout. This may be done also before
       *  makeConnections() has been called. */
      void dumpConnections();

    protected:

      friend class ApplicationModule;
      friend class VariableNetwork;
      friend class VariableNetworkNode;

      template<typename UserType>
      friend class Accessor;

      /** To be implemented by the user: Instantiate all application modules and connect the variables to each other */
      virtual void initialise() = 0;

      /** Make the connections between accessors as requested in the initialise() function. */
      void makeConnections();

      /** Make the connections for a single network */
      void makeConnectionsForNetwork(VariableNetwork &network);

      /** UserType-dependent part of makeConnectionsForNetwork() */
      template<typename UserType>
      void typedMakeConnection(VariableNetwork &network);

      /** Register a connection between two Accessors */
      void connectAccessors(AccessorBase &a, AccessorBase &b);

      /** Register a connection between two VariableNetworkNode */
      VariableNetwork& connect(VariableNetworkNode a, VariableNetworkNode b);

      /** Return a VariableNetworkNode for a device register with a not yet defined direction */
      template<typename UserType>
      VariableNetworkNode DevReg(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode);
      VariableNetworkNode DevReg(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode,
          const std::type_info &valTyp=typeid(AnyType));

      /** Return a VariableNetworkNode for a control system variable with a not yet defined direction */
      template<typename UserType>
      VariableNetworkNode CtrlVar(const std::string &publicName);
      VariableNetworkNode CtrlVar(const std::string &publicName, const std::type_info &valTyp=typeid(AnyType));

      /** Return a VariableNetworkNode for a feeding device register (i.e. a register that will be read by the application) */
      template<typename UserType>
      VariableNetworkNode feedingDevReg(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode);
      VariableNetworkNode feedingDevReg(const std::string &deviceAlias, const std::string &registerName, UpdateMode mode,
          const std::type_info &valTyp=typeid(AnyType));

      /** Return a VariableNetworkNode for a consuming control system variable */
      template<typename UserType>
      VariableNetworkNode consumingCtrlVar(const std::string &publicName);
      VariableNetworkNode consumingCtrlVar(const std::string &publicName, const std::type_info &valTyp=typeid(AnyType));

      /** Return a VariableNetworkNode for a consuming control system variable */
      template<typename UserType>
      VariableNetworkNode feedingCtrlVar(const std::string &publicName);
      VariableNetworkNode feedingCtrlVar(const std::string &publicName, const std::type_info &valTyp=typeid(AnyType));

      /** Register a connection between a device read-only register and the control system adapter */
      template<typename UserType>
      void feedDeviceRegisterToControlSystem(const std::string &deviceAlias, const std::string &registerName,
          const std::string& publicName, AccessorBase &trigger=InvalidAccessor());

      /** Register a connection between a device write-only register and the control system adapter */
      template<typename UserType>
      void consumeDeviceRegisterFromControlSystem(const std::string &deviceAlias, const std::string &registerName,
          const std::string& publicName);

      /** Perform the actual connection of an accessor to a device register */
      template<typename UserType>
      boost::shared_ptr<mtca4u::ProcessVariable> createDeviceAccessor(const std::string &deviceAlias,
          const std::string &registerName, VariableDirection direction, UpdateMode mode);

      /** Create a process variable with the PVManager, which is exported to the control system adapter */
      template<typename UserType>
      boost::shared_ptr<mtca4u::ProcessVariable> createProcessScalar(VariableDirection direction,
          const std::string &name);

      /** Create a local process variable which is not exported. The first element in the returned pair will be the
       *  sender, the second the receiver. */
      template<typename UserType>
      std::pair< boost::shared_ptr<mtca4u::ProcessVariable>, boost::shared_ptr<mtca4u::ProcessVariable> >
        createProcessScalar();

      /** Register an application module with the application. Will be called automatically by all modules in their
       *  constructors. */
      void registerModule(ApplicationModule &module) {
        moduleList.push_back(&module);
      }

      /** The name of the application */
      std::string applicationName;

      /** List of application modules */
      std::list<ApplicationModule*> moduleList;

      /** List of ImplementationAdapters */
      std::list<boost::shared_ptr<ImplementationAdapterBase>> adapterList;

      /** List of variable networks */
      std::list<VariableNetwork> networkList;

      /** Find the network containing one of the given registers. If no network has been found, create an empty one
       *  and add it to the networkList. */
      VariableNetwork& findOrCreateNetwork(AccessorBase *a, AccessorBase *b);
      VariableNetwork& findOrCreateNetwork(AccessorBase *a);

      /** Find the network containing one of the given registers. If no network has been found, invalidNetwork
       *  is returned. */
      VariableNetwork& findNetwork(AccessorBase *a);

      /** Instance of VariableNetwork to indicate an invalid network */
      VariableNetwork invalidNetwork;

      /** Pointer to the process variable manager used to create variables exported to the control system */
      boost::shared_ptr<mtca4u::DevicePVManager> _processVariableManager;

      /** Pointer to the only instance of the Application */
      static Application *instance;

      /** Mutex for thread-safety when setting the instance pointer */
      static std::mutex instance_mutex;

      /** Map of DeviceBackends used by this application. The map key is the alias name from the DMAP file */
      std::map<std::string, boost::shared_ptr<mtca4u::DeviceBackend>> deviceMap;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_APPLICATION_H */
