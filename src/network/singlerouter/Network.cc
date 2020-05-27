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
#include "network/singlerouter/Network.h"

#include <factory/ObjectFactory.h>

#include <cassert>
#include <cmath>

#include <tuple>

#include "network/singlerouter/InjectionAlgorithm.h"
#include "network/singlerouter/RoutingAlgorithm.h"

namespace SingleRouter {

Network::Network(const std::string& _name, const Component* _parent,
                 MetadataHandler* _metadataHandler, Json::Value _settings)
    : ::Network(_name, _parent, _metadataHandler, _settings) {
  // concentration
  concentration_ = _settings["concentration"].asUInt();
  assert(concentration_ > 0);
  dbgprintf("concentration_ = %u", concentration_);

  // ports per interface
  interfacePorts_ = _settings["interface_ports"].asUInt();
  assert(interfacePorts_ > 0);
  assert(concentration_ % interfacePorts_ == 0);
  dbgprintf("interfacePorts_ = %u", interfacePorts_);

  // router radix
  u32 routerRadix = concentration_;

  // parse the protocol classes description
  loadProtocolClassInfo(_settings["protocol_classes"]);

  // create the router
  router_ = Router::create(
      "Router", this, this, 0, std::vector<u32>(), routerRadix, numVcs_,
      _metadataHandler, _settings["router"]);

  // create the interfaces and external channels
  interfaces_.resize(numInterfaces(), nullptr);
  for (u32 id = 0; id < numInterfaces(); id++) {
    // create the interface
    std::string interfaceName = "Interface_" + std::to_string(id);
    Interface* interface = Interface::create(
        interfaceName, this, this, id, {id}, interfacePorts_, numVcs_,
        _metadataHandler, _settings["interface"]);
    interfaces_.at(id) = interface;

    // create and link channels
    for (u32 ch = 0; ch < interfacePorts_; ch++) {
      // create the channels
      std::string inChannelName = "InChannel_" + std::to_string(id) + "_" +
                                  std::to_string(ch);
      Channel* inChannel = new Channel(inChannelName, this, numVcs_,
                                       _settings["external_channel"]);
      externalChannels_.push_back(inChannel);
      std::string outChannelName = "OutChannel_" + std::to_string(id) + "_" +
                                   std::to_string(ch);
      Channel* outChannel = new Channel(outChannelName, this, numVcs_,
                                        _settings["external_channel"]);
      externalChannels_.push_back(outChannel);

      // link interfaces to router via channels
      u32 routerPort = id * interfacePorts_ + ch;
      router_->setInputChannel(routerPort, inChannel);
      router_->setOutputChannel(routerPort, outChannel);
      interface->setInputChannel(ch, outChannel);
      interface->setOutputChannel(ch, inChannel);
    }
  }

  // clear the protocol class info
  clearProtocolClassInfo();
}

Network::~Network() {
  delete router_;
  for (auto it = interfaces_.begin(); it != interfaces_.end(); ++it) {
    Interface* interface = *it;
    delete interface;
  }
  for (auto it = externalChannels_.begin(); it != externalChannels_.end();
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
      _inputVc, concentration_, interfacePorts_, settings.routing);
}

u32 Network::numRouters() const {
  return 1;
}

u32 Network::numInterfaces() const {
  return concentration_ / interfacePorts_;
}

Router* Network::getRouter(u32 _id) const {
  assert(_id == 0);
  return router_;
}

Interface* Network::getInterface(u32 _id) const {
  return interfaces_.at(_id);
}

void Network::translateInterfaceIdToAddress(
    u32 _id, std::vector<u32>* _address) const {
  _address->resize(1);
  _address->at(0) = _id;
}

u32 Network::translateInterfaceAddressToId(
    const std::vector<u32>* _address) const {
  return _address->at(0);
}

void Network::translateRouterIdToAddress(
    u32 _id, std::vector<u32>* _address) const {
  _address->resize(1);
  _address->at(0) = 0;
}

u32 Network::translateRouterAddressToId(
    const std::vector<u32>* _address) const {
  return 0;
}

u32 Network::computeMinimalHops(const std::vector<u32>* _source,
                                const std::vector<u32>* _destination) const {
  return 1;
}

void Network::collectChannels(std::vector<Channel*>* _channels) {
  for (auto it = externalChannels_.begin(); it != externalChannels_.end();
       ++it) {
    Channel* channel = *it;
    _channels->push_back(channel);
  }
}

}  // namespace SingleRouter

registerWithObjectFactory("single_router", ::Network,
                          SingleRouter::Network, NETWORK_ARGS);
