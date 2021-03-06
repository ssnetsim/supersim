/*
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * See the NOTICE file distributed with this work for additional information
 * regarding copyright ownership. You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "network/mesh/Network.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <tuple>

#include "factory/ObjectFactory.h"
#include "network/cube/util.h"
#include "network/mesh/InjectionAlgorithm.h"
#include "network/mesh/RoutingAlgorithm.h"
#include "network/mesh/util.h"
#include "strop/strop.h"
#include "util/DimensionIterator.h"

namespace Mesh {

Network::Network(const std::string& _name, const Component* _parent,
                 MetadataHandler* _metadataHandler, nlohmann::json _settings)
    : ::Network(_name, _parent, _metadataHandler, _settings) {
  // dimensions and concentration
  assert(_settings["dimension_widths"].is_array());
  dimensions_ = _settings["dimension_widths"].size();
  concentration_ = _settings["concentration"].get<u32>();
  assert(concentration_ > 0);
  interfacePorts_ = _settings["interface_ports"].get<u32>();
  assert(interfacePorts_ > 0);
  assert(concentration_ % interfacePorts_ == 0);
  dimensionWidths_.resize(dimensions_);
  for (u32 i = 0; i < dimensions_; i++) {
    dimensionWidths_.at(i) = _settings["dimension_widths"][i].get<u32>();
  }

  assert(_settings["dimension_weights"].size() == dimensions_);
  dimensionWeights_.resize(dimensions_);
  for (u32 i = 0; i < dimensions_; i++) {
    dimensionWeights_.at(i) = _settings["dimension_weights"][i].get<u32>();
    assert(dimensionWeights_.at(i) >= 1);
  }

  dbgprintf("dimensions_ = %u", dimensions_);
  dbgprintf("dimensionWidths_ = %s",
            strop::vecString<u32>(dimensionWidths_, '-').c_str());
  dbgprintf("dimensionWeights_ = %s",
            strop::vecString<u32>(dimensionWeights_, '-').c_str());
  dbgprintf("concentration_ = %u", concentration_);
  dbgprintf("interfacePorts_ = %u", interfacePorts_);

  // router radix: concentration + 2 * sum(weights)
  u32 routerRadix = concentration_;
  for (u32 i = 0; i < dimensions_; i++) {
    routerRadix += 2 * dimensionWeights_.at(i);
  }
  dbgprintf("router radix = %u", routerRadix);

  // parse the protocol classes description
  loadProtocolClassInfo(_settings["protocol_classes"]);

  // setup a router iterator for looping over the router dimensions
  DimensionIterator routerIterator(dimensionWidths_);
  std::vector<u32> routerAddress(dimensionWidths_.size());

  // create the routers
  routerIterator.reset();
  routers_.setSize(dimensionWidths_);
  while (routerIterator.next(&routerAddress)) {
    std::string routerName =
        "Router_" + strop::vecString<u32>(routerAddress, '-');

    // use the router factory to create a router
    u32 routerId = translateRouterAddressToId(&routerAddress);
    routers_.at(routerAddress) = Router::create(
        routerName, this, this, routerId, routerAddress, routerRadix, numVcs_,
        _metadataHandler, _settings["router"]);
  }

  // link routers via channels
  routerIterator.reset();
  while (routerIterator.next(&routerAddress)) {
    u32 portBase = concentration_;
    u32 sourcePort, destinationPort;
    // determine the source router
    std::vector<u32> sourceAddress(routerAddress);
    for (u32 dim = 0; dim < dimensions_; dim++) {
      u32 dimWidth = dimensionWidths_.at(dim);
      u32 dimWeight = dimensionWeights_.at(dim);
      std::vector<u32> destinationAddress(sourceAddress);

      // there are 'dimWeight' ports going right (to router with larger
      // index in this dimension), and 'dimWeight' ports going left.

      std::string channelName;
      Channel* channel;

      // connect to the destination router going right
      if (sourceAddress.at(dim) < (dimWidth - 1)) {
        destinationAddress.at(dim) = sourceAddress.at(dim) + 1;
        for (u32 wInd = 0; wInd < dimWeight; wInd++) {
          sourcePort = portBase + wInd;
          destinationPort = portBase + dimWeight + wInd;

          // create the channels
          channelName = "RChannel_" +
                        strop::vecString<u32>(routerAddress, '-') + "-to-" +
                        strop::vecString<u32>(destinationAddress, '-') + "-" +
                        std::to_string(wInd);
          channel = new Channel(channelName, this, numVcs_,
                                _settings["internal_channel"]);
          internalChannels_.push_back(channel);

          // link the routers from source to destination
          dbgprintf("linking %s:%u to %s:%u with %s",
                    strop::vecString<u32>(sourceAddress, '-').c_str(),
                    sourcePort,
                    strop::vecString<u32>(destinationAddress, '-').c_str(),
                    destinationPort, channelName.c_str());

          routers_.at(sourceAddress)->setOutputChannel(sourcePort, channel);
          routers_.at(destinationAddress)
              ->setInputChannel(destinationPort, channel);
        }
      }

      // connect to the destination router going left
      if (sourceAddress.at(dim) > 0) {
        destinationAddress.at(dim) = sourceAddress.at(dim) - 1;
        for (u32 wInd = 0; wInd < dimWeight; wInd++) {
          sourcePort = portBase + dimWeight + wInd;
          destinationPort = portBase + wInd;
          channelName = "LChannel_" +
                        strop::vecString<u32>(routerAddress, '-') + "-to-" +
                        strop::vecString<u32>(destinationAddress, '-') + "-" +
                        std::to_string(wInd);
          channel = new Channel(channelName, this, numVcs_,
                                _settings["internal_channel"]);
          internalChannels_.push_back(channel);

          // link the routers from source to destination
          dbgprintf("linking %s:%u to %s:%u with %s",
                    strop::vecString<u32>(sourceAddress, '-').c_str(),
                    sourcePort,
                    strop::vecString<u32>(destinationAddress, '-').c_str(),
                    destinationPort, channelName.c_str());
          routers_.at(sourceAddress)->setOutputChannel(sourcePort, channel);
          routers_.at(destinationAddress)
              ->setInputChannel(destinationPort, channel);
        }
      }

      portBase += 2 * dimWeight;
    }
  }

  // create a vector of dimension widths that contains the concentration
  u32 interfacesPerRouter = concentration_ / interfacePorts_;
  std::vector<u32> fullDimensionWidths(1);
  fullDimensionWidths.at(0) = interfacesPerRouter;
  fullDimensionWidths.insert(fullDimensionWidths.begin() + 1,
                             dimensionWidths_.begin(), dimensionWidths_.end());

  // create interfaces and link them with the routers
  interfaces_.setSize(fullDimensionWidths);
  routerIterator.reset();
  while (routerIterator.next(&routerAddress)) {
    // get the router now, for later linking with terminals
    Router* router = routers_.at(routerAddress);

    // loop over interfaces
    for (u32 iface = 0; iface < interfacesPerRouter; iface++) {
      // create a vector for the Interface address
      std::vector<u32> interfaceAddress(1);
      interfaceAddress.at(0) = iface;
      interfaceAddress.insert(interfaceAddress.begin() + 1,
                              routerAddress.begin(), routerAddress.end());

      // create an interface name
      std::string interfaceName =
          "Interface_" + strop::vecString<u32>(interfaceAddress, '-');

      // create the interface
      u32 interfaceId = translateInterfaceAddressToId(&interfaceAddress);
      Interface* interface = Interface::create(
          interfaceName, this, this, interfaceId, interfaceAddress,
          interfacePorts_, numVcs_, _metadataHandler, _settings["interface"]);
      interfaces_.at(interfaceAddress) = interface;

      // create and link channels
      for (u32 ch = 0; ch < interfacePorts_; ch++) {
        // create I/O channels
        std::string inChannelName =
            "Channel_" + strop::vecString<u32>(interfaceAddress, '-') + "-to-" +
            strop::vecString<u32>(routerAddress, '-') + "_" +
            std::to_string(ch);
        std::string outChannelName =
            "Channel_" + strop::vecString<u32>(routerAddress, '-') + "-to-" +
            strop::vecString<u32>(interfaceAddress, '-') + "_" +
            std::to_string(ch);
        Channel* inChannel = new Channel(inChannelName, this, numVcs_,
                                         _settings["external_channel"]);
        Channel* outChannel = new Channel(outChannelName, this, numVcs_,
                                          _settings["external_channel"]);
        externalChannels_.push_back(inChannel);
        externalChannels_.push_back(outChannel);

        // link with router
        u32 routerPort = iface * interfacePorts_ + ch;
        router->setInputChannel(routerPort, inChannel);
        interface->setOutputChannel(ch, inChannel);
        router->setOutputChannel(routerPort, outChannel);
        interface->setInputChannel(ch, outChannel);
      }
    }
  }

  // clear the protocol class info
  clearProtocolClassInfo();
}

Network::~Network() {
  // delete routers
  for (u32 id = 0; id < routers_.size(); id++) {
    delete routers_.at(id);
  }

  // delete interfaces
  for (u32 id = 0; id < interfaces_.size(); id++) {
    delete interfaces_.at(id);
  }

  // delete channels
  for (auto it = internalChannels_.begin(); it != internalChannels_.end();
       ++it) {
    delete *it;
  }
  for (auto it = externalChannels_.begin(); it != externalChannels_.end();
       ++it) {
    delete *it;
  }
}

::InjectionAlgorithm* Network::createInjectionAlgorithm(
    u32 _inputPc, const std::string& _name, const Component* _parent,
    Interface* _interface) {
  // get the info
  const ::Network::PcSettings& settings = pcSettings(_inputPc);

  // call the routing algorithm factory
  return InjectionAlgorithm::create(_name, _parent, _interface, settings.baseVc,
                                    settings.numVcs, _inputPc,
                                    settings.injection);
}

::RoutingAlgorithm* Network::createRoutingAlgorithm(u32 _inputPort,
                                                    u32 _inputVc,
                                                    const std::string& _name,
                                                    const Component* _parent,
                                                    Router* _router) {
  // get the info
  u32 pc = vcToPc(_inputVc);
  const ::Network::PcSettings& settings = pcSettings(pc);

  // call the routing algorithm factory
  return RoutingAlgorithm::create(
      _name, _parent, _router, settings.baseVc, settings.numVcs, _inputPort,
      _inputVc, dimensionWidths_, dimensionWeights_, concentration_,
      interfacePorts_, settings.routing);
}

u32 Network::numRouters() const {
  return routers_.size();
}

u32 Network::numInterfaces() const {
  return interfaces_.size();
}

Router* Network::getRouter(u32 _id) const {
  return routers_.at(_id);
}

Interface* Network::getInterface(u32 _id) const {
  return interfaces_.at(_id);
}

void Network::translateInterfaceIdToAddress(u32 _id,
                                            std::vector<u32>* _address) const {
  Cube::translateInterfaceIdToAddress(_id, dimensionWidths_, concentration_,
                                      interfacePorts_, _address);
}

u32 Network::translateInterfaceAddressToId(
    const std::vector<u32>* _address) const {
  return Cube::translateInterfaceAddressToId(_address, dimensionWidths_,
                                             concentration_, interfacePorts_);
}

void Network::translateRouterIdToAddress(u32 _id,
                                         std::vector<u32>* _address) const {
  Cube::translateRouterIdToAddress(_id, dimensionWidths_, _address);
}

u32 Network::translateRouterAddressToId(
    const std::vector<u32>* _address) const {
  return Cube::translateRouterAddressToId(_address, dimensionWidths_);
}

u32 Network::computeMinimalHops(const std::vector<u32>* _source,
                                const std::vector<u32>* _destination) const {
  return Mesh::computeMinimalHops(_source, _destination, dimensions_,
                                  dimensionWidths_);
}

void Network::collectChannels(std::vector<Channel*>* _channels) {
  for (auto it = externalChannels_.begin(); it != externalChannels_.end();
       ++it) {
    _channels->push_back(*it);
  }
  for (auto it = internalChannels_.begin(); it != internalChannels_.end();
       ++it) {
    _channels->push_back(*it);
  }
}

}  // namespace Mesh

registerWithObjectFactory("mesh", ::Network, Mesh::Network, NETWORK_ARGS);
