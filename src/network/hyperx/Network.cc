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
#include "network/hyperx/Network.h"

#include <cassert>
#include <cmath>
#include <tuple>

#include "factory/ObjectFactory.h"
#include "network/cube/util.h"
#include "network/hyperx/InjectionAlgorithm.h"
#include "network/hyperx/RoutingAlgorithm.h"
#include "network/hyperx/util.h"
#include "strop/strop.h"
#include "util/DimensionIterator.h"

namespace HyperX {

Network::Network(const std::string& _name, const Component* _parent,
                 MetadataHandler* _metadataHandler, nlohmann::json _settings)
    : ::Network(_name, _parent, _metadataHandler, _settings) {
  // dimensions and concentration
  assert(_settings["dimension_widths"].is_array());
  dimensions_ = _settings["dimension_widths"].size();
  assert(_settings["dimension_weights"].size() == dimensions_);
  dimensionWidths_.resize(dimensions_);
  for (u32 i = 0; i < dimensions_; i++) {
    dimensionWidths_.at(i) = _settings["dimension_widths"][i].get<u32>();
    assert(dimensionWidths_.at(i) >= 2);
  }
  dimensionWeights_.resize(dimensions_);
  for (u32 i = 0; i < dimensions_; i++) {
    dimensionWeights_.at(i) = _settings["dimension_weights"][i].get<u32>();
    assert(dimensionWeights_.at(i) >= 1);
  }
  concentration_ = _settings["concentration"].get<u32>();
  assert(concentration_ > 0);
  interfacePorts_ = _settings["interface_ports"].get<u32>();
  assert(interfacePorts_ > 0);
  assert(concentration_ % interfacePorts_ == 0);
  dbgprintf("dimensions_ = %u", dimensions_);
  dbgprintf("dimensionWidths_ = %s",
            strop::vecString<u32>(dimensionWidths_, '-').c_str());
  dbgprintf("dimensionWeights_ = %s",
            strop::vecString<u32>(dimensionWeights_, '-').c_str());
  dbgprintf("concentration_ = %u", concentration_);
  dbgprintf("interfacePorts_ = %u", interfacePorts_);

  // varying channel latency per dimension
  std::vector<f64> scalars;
  assert(_settings.contains("channel_mode"));

  // scalar
  if (_settings["channel_mode"].get<std::string>() == "scalar") {
    assert(_settings["channel_scalars"].is_array());
    assert(_settings["channel_scalars"].size() == dimensions_);
    scalars.resize(dimensions_);
    for (u32 i = 0; i < dimensions_; i++) {
      if (_settings["channel_scalars"][i].get<f32>() > 0.0) {
        scalars.at(i) = _settings["channel_scalars"][i].get<f32>();
      } else {
        scalars.at(i) = 1.0;
      }
    }
    dbgprintf("scalars = %s", strop::vecString<f64>(scalars, ',').c_str());
  }

  // router radix
  u32 routerRadix = concentration_;
  for (u32 i = 0; i < dimensions_; i++) {
    routerRadix += ((dimensionWidths_.at(i) - 1) * dimensionWeights_.at(i));
  }

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
    for (u32 dim = 0; dim < dimensions_; dim++) {
      u32 dimWidth = dimensionWidths_.at(dim);
      u32 dimWeight = dimensionWeights_.at(dim);
      dbgprintf("dim=%u width=%u weight=%u\n", dim, dimWidth, dimWeight);

      for (u32 offset = 1; offset < dimWidth; offset++) {
        // determine the source router
        std::vector<u32> sourceAddress(routerAddress);

        // determine the destination router
        std::vector<u32> destinationAddress(sourceAddress);
        destinationAddress.at(dim) =
            (sourceAddress.at(dim) + offset) % dimWidth;

        // determine the channel latency for current dim and offset
        if (_settings["channel_mode"].get<std::string>() == "scalar") {
          f64 link_dist = fabs((s64)sourceAddress.at(dim) -
                               (s64)destinationAddress.at(dim));
          u32 channelLatency = (u32)(ceil(scalars[dim] * link_dist));
          dbgprintf("s=%s d=%s c_dist=%.2f l_val=%.2f latency=%u",
                    strop::vecString<u32>(sourceAddress, '-').c_str(),
                    strop::vecString<u32>(destinationAddress, '-').c_str(),
                    link_dist, scalars[dim], channelLatency);

          // override settings
          _settings["internal_channel"]["latency"] = channelLatency;
        }

        for (u32 weight = 0; weight < dimWeight; weight++) {
          // create the channel
          std::string channelName =
              "Channel_" + strop::vecString<u32>(routerAddress, '-') + "-to-" +
              strop::vecString<u32>(destinationAddress, '-') + "-" +
              std::to_string(weight);
          Channel* channel = new Channel(channelName, this, numVcs_,
                                         _settings["internal_channel"]);
          internalChannels_.push_back(channel);

          // determine the port numbers
          u32 sourcePort = portBase + ((offset - 1) * dimWeight) + weight;
          u32 destinationPort = portBase + ((dimWidth - 1) * dimWeight) -
                                (offset * dimWeight) + weight;
          dbgprintf("s=%s:%u to d=%s:%u with %s latency=%d",
                    strop::vecString<u32>(sourceAddress, '-').c_str(),
                    sourcePort,
                    strop::vecString<u32>(destinationAddress, '-').c_str(),
                    destinationPort, channelName.c_str(), channel->latency());

          // link the routers from source to destination
          routers_.at(sourceAddress)->setOutputChannel(sourcePort, channel);
          routers_.at(destinationAddress)
              ->setInputChannel(destinationPort, channel);
        }
      }
      portBase += ((dimWidth - 1) * dimWeight);
    }
  }

  // create a vector of dimension widths that contains the interfaces
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
  return HyperX::computeMinimalHops(_source, _destination, dimensions_);
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

}  // namespace HyperX

registerWithObjectFactory("hyperx", ::Network, HyperX::Network, NETWORK_ARGS);
