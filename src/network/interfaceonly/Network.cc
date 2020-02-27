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

#include <factory/ObjectFactory.h>

#include <cassert>
#include <cmath>

#include <tuple>

namespace InterfaceOnly {

Network::Network(const std::string& _name, const Component* _parent,
                 MetadataHandler* _metadataHandler, Json::Value _settings)
    : ::Network(_name, _parent, _metadataHandler, _settings) {
  // num_interfaces
  num_interfaces_ = _settings["num_interfaces"].asUInt();
  assert(num_interfaces_ > 0);
  assert(num_interfaces_ <= 2);
  dbgprintf("num_interfaces_ = %u", num_interfaces_);

  // parse the protocol classes description
  loadProtocolClassInfo(_settings["protocol_classes"]);

  // create the interfaces
  interfaces_.resize(num_interfaces_, nullptr);
  for (u32 id = 0; id < num_interfaces_; id++) {
    // create the interface
    std::string interfaceName = "Interface_" + std::to_string(id);
    Interface* interface = Interface::create(
        interfaceName, this, id, {id}, numVcs_, protocolClassVcs_,
        _metadataHandler, _settings["interface"]);
    interfaces_.at(id) = interface;
  }

  // create the channels
  for (u32 id = 0; id < num_interfaces_; id++) {
    // create the single channels
    std::string channelName = "Channel_" + std::to_string(id);
    Channel* channel = new Channel(channelName, this, numVcs_,
                                   _settings["external_channel"]);
    externalChannels_.push_back(channel);
  }

  // connect the interface(s) and channel(s)
  if (num_interfaces_ == 1) {
    // a single interface is wired in loopback mode
    Channel* channel = externalChannels_.at(0);
    interfaces_.at(0)->setInputChannel(0, channel);
    interfaces_.at(0)->setOutputChannel(0, channel);
  } else {  // num_interfaces_ == 2
    // two interfaces are wired directly together
    Channel* channel0 = externalChannels_.at(0);
    Channel* channel1 = externalChannels_.at(1);
    interfaces_.at(0)->setOutputChannel(0, channel0);
    interfaces_.at(1)->setInputChannel(0, channel0);
    interfaces_.at(1)->setOutputChannel(0, channel1);
    interfaces_.at(0)->setInputChannel(0, channel1);
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

::RoutingAlgorithm* Network::createRoutingAlgorithm(
     u32 _inputPort, u32 _inputVc, const std::string& _name,
     const Component* _parent, Router* _router) {
  assert(false);  // routers aren't created thus this will never be called
  return nullptr;
}

u32 Network::numRouters() const {
  return 0;
}

u32 Network::numInterfaces() const {
  return num_interfaces_;
}

Router* Network::getRouter(u32 _id) const {
  assert(false);  // there are no routers
  return nullptr;
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

registerWithObjectFactory("interface_only", ::Network,
                          InterfaceOnly::Network, NETWORK_ARGS);
