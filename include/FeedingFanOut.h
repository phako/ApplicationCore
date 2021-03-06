/*
 * FeedingFanOut.h
 *
 *  Created on: Jun 15, 2016
 *      Author: Martin Hierholzer
 */

#ifndef CHIMERATK_FEEDING_FAN_OUT_H
#define CHIMERATK_FEEDING_FAN_OUT_H

#include <mtca4u/NDRegisterAccessor.h>

#include "FanOut.h"

namespace ChimeraTK {

  /**
   * NDRegisterAccessor implementation which distributes values written to this accessor out to any number of slaves.
   */
  template<typename UserType>
  class FeedingFanOut : public FanOut<UserType>, public mtca4u::NDRegisterAccessor<UserType> {

    public:

      FeedingFanOut(std::string const &name, std::string const &unit, std::string const &description,
                    size_t numberOfElements)
      : FanOut<UserType>(boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>>()),
        mtca4u::NDRegisterAccessor<UserType>(name, unit, description)
      {
        mtca4u::NDRegisterAccessor<UserType>::buffer_2D.resize(1);
        mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0].resize(numberOfElements);
      }

      /** Add a slave to the FanOut. Only sending end-points of a consuming node may be added. */
      void addSlave(boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType>> slave) override {
        if(!slave->isWriteable()) {
          throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(
              "FeedingFanOut::addSlave() has been called with a receiving implementation!");
        }
        // check if array shape is compatible, unless the receiver is a trigger node, so no data is expected
        if( slave->getNumberOfSamples() != 0 &&
            ( slave->getNumberOfChannels() != 1 || slave->getNumberOfSamples() != this->getNumberOfSamples() ) ) {
          std::string what = "FeedingFanOut::addSlave(): Trying to add a slave '" + slave->getName();
          what += "' with incompatible array shape! Name of fan out: '" + this->getName() + "'";
          throw ApplicationExceptionWithID<ApplicationExceptionID::illegalParameter>(what.c_str());
        }
        FanOut<UserType>::slaves.push_back(slave);
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

      bool doReadTransferLatest() override {
        throw std::logic_error("Read operation called on write-only variable.");
      }

      void doPreRead() override {
        throw std::logic_error("Read operation called on write-only variable.");
      }

      void doPostRead() override {
        throw std::logic_error("Read operation called on write-only variable.");
      }

      ChimeraTK::TransferFuture doReadTransferAsync() override {
        throw std::logic_error("Read operation called on write-only variable.");
      }

      void doPreWrite() override {
        for(auto &slave : FanOut<UserType>::slaves) {     // send out copies to slaves
          if(slave->getNumberOfSamples() != 0) {          // do not send copy if no data is expected (e.g. trigger)
            if(slave == FanOut<UserType>::slaves.front()) {     // in case of first slave, swap instead of copy
              slave->accessChannel(0).swap(mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0]);
            }
            else {                                // not the first slave: copy the data from the first slave
              slave->accessChannel(0) = FanOut<UserType>::slaves.front()->accessChannel(0);
            }
          }
        }
        // pre write may only be called on the target accessors after we have filled them all, otherwise the first
        // accessor might take us the data away...
        for(auto &slave : FanOut<UserType>::slaves) {
          slave->preWrite();
        }
      }

      bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber={}) override {
        bool dataLost = false;
        for(auto &slave : FanOut<UserType>::slaves) {
          bool ret = slave->doWriteTransfer(versionNumber);
          if(ret) dataLost = true;
        }
        return dataLost;
      }

      void doPostWrite() override {
        for(auto &slave : FanOut<UserType>::slaves) {
          slave->postWrite();
        }
        FanOut<UserType>::slaves.front()->accessChannel(0).swap(mtca4u::NDRegisterAccessor<UserType>::buffer_2D[0]);
      }

      bool mayReplaceOther(const boost::shared_ptr<const mtca4u::TransferElement>&) const override {
        return false;   /// @todo implement properly?
      }

      std::list<boost::shared_ptr<mtca4u::TransferElement> > getInternalElements() override {
        return {};    /// @todo implement properly?
      }

      std::vector<boost::shared_ptr<mtca4u::TransferElement> > getHardwareAccessingElements() override {
        return { boost::enable_shared_from_this<mtca4u::TransferElement>::shared_from_this() }; /// @todo implement properly?
      }

      void replaceTransferElement(boost::shared_ptr<mtca4u::TransferElement>) override {
        // You can't replace anything here. Just do nothing.
        /// @todo implement properly?
      }

  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_FEEDING_FAN_OUT_H */

