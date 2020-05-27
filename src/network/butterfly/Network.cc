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
#include "network/butterfly/Network.h"

#include <factory/ObjectFactory.h>
#include <strop/strop.h>

#include <cassert>
#include <cmath>

#include <tuple>

#include "network/butterfly/InjectionAlgorithm.h"
#include "network/butterfly/RoutingAlgorithm.h"
#include "network/butterfly/util.h"

namespace Butterfly {

Network::Network(const std::string& _name, const Component* _parent,
                 MetadataHandler* _metadataHandler, Json::Value _settings)
    : ::Network(_name, _parent, _metadataHandler, _settings) {
  // radix and stages
  routerRadix_ = _settings["radix"].asUInt();
  assert(routerRadix_ >= 2);
  numStages_ = _settings["stages"].asUInt();
  assert(numStages_ >= 1);
  stageWidth_ = (u32)pow(routerRadix_, numStages_ - 1);
  interfacePorts_ = _settings["interface_ports"].asUInt();
  assert(interfacePorts_ > 0);
  assert(routerRadix_ % interfacePorts_ == 0);

  // parse the protocol classes description
  loadProtocolClassInfo(_settings["protocol_classes"]);

  // create the routers
  routers_.resize(stageWidth_ * numStages_, nullptr);
  for (u32 stage = 0; stage < numStages_; stage++) {
    tmpStage_ = stage;
    for (u32 column = 0; column < stageWidth_; column++) {
      // create the router name
      std::string rname = "Router_" + std::to_string(stage) + "-" +
                          std::to_string(column);

      // create the router
      u32 routerId = stage * stageWidth_ + column;
      routers_.at(routerId) = Router::create(
          rname, this, this, routerId, {stage, column}, routerRadix_, numVcs_,
          _metadataHandler, _settings["router"]);
    }
  }

  // create internal channels, link routers via channels
  for (u32 cStage = 0; cStage < numStages_ - 1; cStage++) {
    u32 cBaseUnit = (u32)pow(routerRadix_, numStages_ - 1 - cStage);
    u32 nStage = cStage + 1;
    u32 nBaseUnit = (u32)pow(routerRadix_, numStages_ - 1 - nStage);
    for (u32 cColumn = 0; cColumn < stageWidth_; cColumn++) {
      u32 sourceId = cStage * stageWidth_ + cColumn;
      Router* sourceRouter = routers_.at(sourceId);
      u32 cBaseOffset = (cColumn / cBaseUnit) * cBaseUnit;
      u32 cBaseIndex = cColumn % cBaseUnit;
      for (u32 cOutputPort = 0; cOutputPort < routerRadix_; cOutputPort++) {
        u32 nColumn = cBaseOffset + (cBaseIndex % nBaseUnit) +
                      (cOutputPort * nBaseUnit);
        u32 destinationId = nStage * stageWidth_ + nColumn;
        Router* destinationRouter = routers_.at(destinationId);
        u32 nInputPort = cBaseIndex / nBaseUnit;  // cBaseIndex / routerRadix_;

        // create channel
        std::string chname =
            "Channel_" + strop::vecString<u32>(sourceRouter->address(), '-') +
            "-to-" + strop::vecString<u32>(destinationRouter->address(), '-');
        Channel* channel = new Channel(chname, this, numVcs_,
                                       _settings["internal_channel"]);
        internalChannels_.push_back(channel);

        // connect routers
        sourceRouter->setOutputChannel(cOutputPort, channel);
        destinationRouter->setInputChannel(nInputPort, channel);
      }
    }
  }

  // create the interfaces and external channels
  u32 interfacesPerRouter = routerRadix_ / interfacePorts_;
  u32 totalInterfaces = (routerRadix_ * stageWidth_) / interfacePorts_;
  interfaces_.resize(totalInterfaces, nullptr);
  for (u32 r = 0, interfaceId = 0; r < stageWidth_; r++) {
    // get the routers
    u32 inputRouterId = 0 * stageWidth_ + r;
    u32 outputRouterId = (numStages_ - 1) * stageWidth_ + r;
    Router* inputRouter = routers_.at(inputRouterId);
    Router* outputRouter = routers_.at(outputRouterId);

    // loop over interfaces
    for (u32 iface = 0; iface < interfacesPerRouter; iface++, interfaceId++) {
      // create the interface
      std::string interfaceName = "Interface_" + std::to_string(interfaceId);
      std::vector<u32> interfaceAddress;
      translateInterfaceIdToAddress(interfaceId, &interfaceAddress);
      Interface* interface = Interface::create(
          interfaceName, this, this, interfaceId, interfaceAddress,
          interfacePorts_, numVcs_, _metadataHandler, _settings["interface"]);
      interfaces_.at(interfaceId) = interface;

      // create and link channels
      for (u32 ch = 0; ch < interfacePorts_; ch++) {
        // create I/O channels
        std::string inChannelName =
            "InChannel_" + std::to_string(interfaceId) + "_" +
            std::to_string(ch);
        std::string outChannelName =
            "OutChannel_" + std::to_string(interfaceId) + "_" +
            std::to_string(ch);
        Channel* inChannel = new Channel(
            inChannelName, this, numVcs_, _settings["external_channel"]);
        Channel* outChannel = new Channel(
            outChannelName, this, numVcs_, _settings["external_channel"]);
        externalChannels_.push_back(inChannel);
        externalChannels_.push_back(outChannel);

        // link interfaces to routers via channels
        u32 routerPort = iface * interfacePorts_ + ch;
        interface->setOutputChannel(ch, inChannel);
        inputRouter->setInputChannel(routerPort, inChannel);
        interface->setInputChannel(ch, outChannel);
        outputRouter->setOutputChannel(routerPort, outChannel);
      }
    }
  }

  // clear the protocol class info
  clearProtocolClassInfo();
}

Network::~Network() {
  for (auto it = routers_.begin(); it != routers_.end(); ++it) {
    Router* router = *it;
    delete router;
  }
  for (auto it = interfaces_.begin(); it != interfaces_.end(); ++it) {
    Interface* interface = *it;
    delete interface;
  }
  for (auto it = externalChannels_.begin(); it != externalChannels_.end();
       ++it) {
    Channel* channel = *it;
    delete channel;
  }
  for (auto it = internalChannels_.begin(); it != internalChannels_.end();
       ++it) {
    Channel* channel = *it;
    delete channel;
  }
}

::InjectionAlgorithm* Network::createInjectionAlgorithm(
     u32 _inputPc, const std::string& _name,
     const Component* _parent, Interface* _interface) {
  // get the info
  const ::Network::PcSettings& settings = pcSettings(_inputPc);

  // call the routing algorithm factory
  return InjectionAlgorithm::create(
      _name, _parent, _interface, settings.baseVc, settings.numVcs, _inputPc,
      settings.injection);
}

::RoutingAlgorithm* Network::createRoutingAlgorithm(
     u32 _inputPort, u32 _inputVc, const std::string& _name,
     const Component* _parent, Router* _router) {
  // get the info
  u32 pc = vcToPc(_inputVc);
  const ::Network::PcSettings& settings = pcSettings(pc);

  // call the routing algorithm factory
  return RoutingAlgorithm::create(
      _name, _parent, _router, settings.baseVc, settings.numVcs, _inputPort,
      _inputVc, routerRadix_, numStages_, interfacePorts_, tmpStage_,
      settings.routing);
}

u32 Network::numRouters() const {
  return stageWidth_ * numStages_;
}

u32 Network::numInterfaces() const {
  return (routerRadix_ * stageWidth_) / interfacePorts_;
}

Router* Network::getRouter(u32 _id) const {
  return routers_.at(_id);
}

Interface* Network::getInterface(u32 _id) const {
  return interfaces_.at(_id);
}

void Network::translateInterfaceIdToAddress(
    u32 _id, std::vector<u32>* _address) const {
  Butterfly::translateInterfaceIdToAddress(
      routerRadix_, numStages_, stageWidth_, interfacePorts_, _id, _address);
}

u32 Network::translateInterfaceAddressToId(
    const std::vector<u32>* _address) const {
  return Butterfly::translateInterfaceAddressToId(
      routerRadix_, numStages_, stageWidth_, interfacePorts_, _address);
}

void Network::translateRouterIdToAddress(
    u32 _id, std::vector<u32>* _address) const {
  Butterfly::translateRouterIdToAddress(
      routerRadix_, numStages_, stageWidth_, _id, _address);
}

u32 Network::translateRouterAddressToId(
    const std::vector<u32>* _address) const {
  return Butterfly::translateRouterAddressToId(
      routerRadix_, numStages_, stageWidth_, _address);
}

u32 Network::computeMinimalHops(const std::vector<u32>* _source,
                                const std::vector<u32>* _destination) const {
  return Butterfly::computeMinimalHops(numStages_);
}

void Network::collectChannels(std::vector<Channel*>* _channels) {
  for (auto it = externalChannels_.begin(); it != externalChannels_.end();
       ++it) {
    Channel* channel = *it;
    _channels->push_back(channel);
  }
  for (auto it = internalChannels_.begin(); it != internalChannels_.end();
       ++it) {
    Channel* channel = *it;
    _channels->push_back(channel);
  }
}

}  // namespace Butterfly

registerWithObjectFactory("butterfly", ::Network,
                          Butterfly::Network, NETWORK_ARGS);
