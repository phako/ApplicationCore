/*
 * FeedingFanOut.h
 *
 *  Created on: Jun 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_FEEDING_FAN_OUT_H
#define CHIMERATK_FEEDING_FAN_OUT_H

#include <mtca4u/NDRegisterAccessor.h>

#include "ApplicationException.h"

namespace ChimeraTK {

  
  template<typename UserType>
  class FeedingFanOut : public mtca4u::NDRegisterAccessor<UserType> {

    public:

      /** Add a slave to the FanOut. Only sending end-points of a consuming node may be added. */
      void addSlave(boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> slave) {
        if(!slave->isWriteable()) {
          throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(
              "FeedingFanOut::addSlave() has been called with a receiving implementation!");
        }
        if(slaves.size() == 0) {    // first slave: initialise buffers  @todo TODO FIXME first slave could be a trigger receiver!
          mtca4u::NDRegisterAccessor<UserType>::buffer_2D.resize( slave->getNumberOfChannels() );
          for(size_t i=0; i<slave->getNumberOfChannels(); i++) {
            mtca4u::NDRegisterAccessor<UserType>::buffer_2D[i].resize( slave->getNumberOfSamples() );
          }
        }
        else {
          // check if array shape is compatible, unless the receiver is a trigger node, so no data is expected
          if( slave->getNumberOfSamples() != 0 && 
              ( slave->getNumberOfChannels() != slaves.front()->getNumberOfChannels() ||
                slave->getNumberOfSamples() != slaves.front()->getNumberOfSamples()      ) ) {
            std::string what = "FeedingFanOut::addSlave(): Trying to add a slave '";
            what += slave->getName();
            what += "' with incompatible array shape! Name of first slave: ";
            what += slaves.front()->getName();
            throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(what.c_str());
          }
        }
        slaves.push_back(slave);
      }

      bool isReadable() const override {
        return false;
      }
      
      bool isReadOnly() const override {
        return false;
      }
      
      bool isWriteable() const override {
        return true;
      }
      
      void doReadTransfer() override {
        throw std::logic_error("Read operation called on write-only variable.");
      }
      
      bool doReadTransferNonBlocking() override {
        throw std::logic_error("Read operation called on write-only variable.");
      }
      
      void postRead() override {
        throw std::logic_error("Read operation called on write-only variable.");
      }

      void write() override {
        boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> firstSlave;   // will have the data for the other slaves after swapping
        for(auto &slave : slaves) {               // send out copies to slaves
          if(slave->getNumberOfSamples() != 0) {  // do not send copy if no data is expected (e.g. trigger)
            if(!firstSlave) {                     // in case of first slave, swap instead of copy
              firstSlave = slave;
              firstSlave->accessChannel(0).swap(mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0]);
            }
            else {                                // not the first slave: copy the data from the first slave
              slave->accessChannel(0) = firstSlave->accessChannel(0);
            }
          }
          slave->write();
        }
        // swap back the data from the first slave so we still have it available
        if(firstSlave) {
          firstSlave->accessChannel(0).swap(mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0]);
        }
        return;
      }
      
      bool isSameRegister(const boost::shared_ptr<const mtca4u::TransferElement>& e) const override {
        // only true if the very instance of the transfer element is the same
        return e.get() == this;
      }
      
      std::vector<boost::shared_ptr<mtca4u::TransferElement> > getHardwareAccessingElements() override {
        return { boost::enable_shared_from_this<mtca4u::TransferElement>::shared_from_this() };
      }
      
      void replaceTransferElement(boost::shared_ptr<mtca4u::TransferElement>) override {
        // You can't replace anything here. Just do nothing.
      }
      
    protected:

      std::list<boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>>> slaves;

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_FEEDING_FAN_OUT_H */

