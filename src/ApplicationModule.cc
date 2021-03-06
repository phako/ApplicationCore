/*
 * ApplicationModule.cc
 *
 *  Created on: Jun 17, 2016
 *      Author: Martin Hierholzer
 */

#include "ApplicationCore.h"

namespace ChimeraTK {

/*********************************************************************************************************************/

  ApplicationModule::ApplicationModule(EntityOwner *owner, const std::string &name, const std::string &description,
          bool eliminateHierarchy, const std::unordered_set<std::string> &tags)
  : ModuleImpl(owner,name,description,eliminateHierarchy,tags)
  {
    if(!dynamic_cast<ModuleGroup*>(owner) && !dynamic_cast<Application*>(owner)) {
      throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(
        "ApplicationModules must be owned either by ModuleGroups or the Application!");
    }
  }

/*********************************************************************************************************************/

  void ApplicationModule::run() {

    // start the module thread
    assert(!moduleThread.joinable());
    moduleThread = boost::thread(&ApplicationModule::mainLoopWrapper, this);
  }

/*********************************************************************************************************************/

  void ApplicationModule::terminate() {
    if(moduleThread.joinable()) {
      moduleThread.interrupt();
      moduleThread.join();
    }
    assert(!moduleThread.joinable());
  }

/*********************************************************************************************************************/

  ApplicationModule::~ApplicationModule() {
    assert(!moduleThread.joinable());
  }

/*********************************************************************************************************************/

  void ApplicationModule::mainLoopWrapper() {
    Application::registerThread("ApplicationModule "+getName());
    Application::testableModeLock("start");
    // enter the main loop
    mainLoop();
    Application::testableModeUnlock("terminate");
  }

} /* namespace ChimeraTK */
