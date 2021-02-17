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
#include "network/interfaceonly/Network.h"

#include <cassert>
#include <cmath>
#include <tuple>

#include "factory/ObjectFactory.h"
#include "network/interfaceonly/InjectionAlgorithm.h"

namespace InterfaceOnly {

Network::Network(const std::string& _name, const Component* _parent,
                 MetadataHandler* _metadataHandler, nlohmann::json _settings)
    : ::Network(_name, _parent, _metadataHandler, _settings) {
  // num_interfaces
  numInterfaces_ = _settings["num_interfaces"].get<u32>();
  assert(numInterfaces_ > 0);
  assert(numInterfaces_ <= 2);
  interfacePorts_ = _settings["interface_ports"].get<u32>();
  assert(interfacePorts_ > 0);
  dbgprintf("num_interfaces_ = %u", numInterfaces_);

  // parse the protocol classes description
  loadProtocolClassInfo(_settings["protocol_classes"]);

  // create the interfaces
  interfaces_.resize(numInterfaces_, nullptr);
  for (u32 id = 0; id < numInterfaces_; id++) {
    // create the interface
    std::string interfaceName = "Interface_" + std::to_string(id);
    Interface* interface =
        Interface::create(interfaceName, this, this, id, {id}, interfacePorts_,
                          numVcs_, _metadataHandler, _settings["interface"]);
    interfaces_.at(id) = interface;
  }

  // create the channels
  for (u32 iface = 0; iface < numInterfaces_; iface++) {
    for (u32 ch = 0; ch < interfacePorts_; ch++) {
      // create the single channels
      std::string channelName =
          "Channel_" + std::to_string(iface) + "_" + std::to_string(ch);
      Channel* channel = new Channel(channelName, this, numVcs_,
                                     _settings["external_channel"]);
      externalChannels_.push_back(channel);
    }
  }

  // connect the interface(s) and channel(s)
  if (numInterfaces_ == 1) {
    // a single interface is wired in loopback mode
    for (u32 ch = 0; ch < interfacePorts_; ch++) {
      Channel* channel = externalChannels_.at(ch);
      interfaces_.at(0)->setInputChannel(ch, channel);
      interfaces_.at(0)->setOutputChannel(ch, channel);
    }
  } else {  // numInterfaces_ == 2
    // two interfaces are wired directly together
    for (u32 ch = 0; ch < interfacePorts_; ch++) {
      Channel* channel0 = externalChannels_.at(ch);
      Channel* channel1 = externalChannels_.at(interfacePorts_ + ch);
      interfaces_.at(0)->setOutputChannel(ch, channel0);
      interfaces_.at(1)->setInputChannel(ch, channel0);
      interfaces_.at(1)->setOutputChannel(ch, channel1);
      interfaces_.at(0)->setInputChannel(ch, channel1);
    }
  }

  // clear the protocol class info
  clearProtocolClassInfo();
}

Network::~Network() {
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
  assert(false);  // routers aren't created thus this will never be called
  return nullptr;
}

u32 Network::numRouters() const {
  return 0;
}

u32 Network::numInterfaces() const {
  return numInterfaces_;
}

Router* Network::getRouter(u32 _id) const {
  assert(false);  // there are no routers
  return nullptr;
}

Interface* Network::getInterface(u32 _id) const {
  return interfaces_.at(_id);
}

void Network::translateInterfaceIdToAddress(u32 _id,
                                            std::vector<u32>* _address) const {
  _address->resize(1);
  _address->at(0) = _id;
}

u32 Network::translateInterfaceAddressToId(
    const std::vector<u32>* _address) const {
  return _address->at(0);
}

void Network::translateRouterIdToAddress(u32 _id,
                                         std::vector<u32>* _address) const {
  assert(false);  // there are no routers
}

u32 Network::translateRouterAddressToId(
    const std::vector<u32>* _address) const {
  assert(false);  // there are no routers
  return 0;
}

u32 Network::computeMinimalHops(const std::vector<u32>* _source,
                                const std::vector<u32>* _destination) const {
  return 0;
}

void Network::collectChannels(std::vector<Channel*>* _channels) {
  for (auto it = externalChannels_.begin(); it != externalChannels_.end();
       ++it) {
    Channel* channel = *it;
    _channels->push_back(channel);
  }
}

}  // namespace InterfaceOnly

registerWithObjectFactory("interface_only", ::Network, InterfaceOnly::Network,
                          NETWORK_ARGS);
