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
#include "network/fattree/Network.h"

#include <cassert>
#include <cmath>

#include "factory/ObjectFactory.h"
#include "network/fattree/InjectionAlgorithm.h"
#include "network/fattree/RoutingAlgorithm.h"
#include "network/fattree/util.h"
#include "strop/strop.h"

namespace FatTree {

Network::Network(const std::string& _name, const Component* _parent,
                 MetadataHandler* _metadataHandler, nlohmann::json _settings)
    : ::Network(_name, _parent, _metadataHandler, _settings) {
  // interface ports
  interfacePorts_ = _settings["interface_ports"].get<u32>();
  assert(interfacePorts_ > 0);

  // network structure
  numInterfaces_ = 1;
  u32 routersAtLevel = 1;
  routersAtLevelPerGroup_.push_back(routersAtLevel);
  numLevels_ = _settings["down_up"].size();

  for (u32 level = 0; level < numLevels_; level++) {
    if (level < numLevels_ - 1) {
      assert(_settings["down_up"][level].size() == 2);
      u32 down = _settings["down_up"][level][0].get<u32>();
      u32 up = _settings["down_up"][level][1].get<u32>();
      // radices
      radices_.push_back(std::make_tuple(down, up, down + up));
      // interfaces per group
      if (level == 0) {
        assert(down % interfacePorts_ == 0);
        numInterfaces_ *= down / interfacePorts_;
      } else {
        numInterfaces_ *= down;
      }
      interfacesPerGroup_.push_back(numInterfaces_);
      // routers per level
      routersAtLevel *= up;
      routersAtLevelPerGroup_.push_back(routersAtLevel);
    } else {
      // last level
      assert(_settings["down_up"][level].size() == 1);
      u32 down = _settings["down_up"][level][0].get<u32>();
      // radices
      radices_.push_back(std::make_tuple(down, 0, down));
      // interfaces per group
      numInterfaces_ *= down;
      interfacesPerGroup_.push_back(numInterfaces_);
    }
  }

  for (u32 level = 0; level < numLevels_; level++) {
    // total groups
    u32 groups = numInterfaces_ / interfacesPerGroup_.at(level);
    totalGroups_.push_back(groups);
    // routers per level
    routersAtLevel_.push_back(groups * routersAtLevelPerGroup_.at(level));
  }

  // create routers vector
  for (u32 level = 0; level < numLevels_; level++) {
    routers_.push_back(
        std::vector<Router*>(routersAtLevel_.at(level), nullptr));
  }

  // parse the protocol classes description
  loadProtocolClassInfo(_settings["protocol_classes"]);

  // create all routers
  for (u32 level = 0; level < numLevels_; level++) {
    u32 levelRouters = routersAtLevel_.at(level);
    for (u32 col = 0; col < levelRouters; col++) {
      // router info
      std::vector<u32> routerAddress = {level, col};
      u32 routerId = translateRouterAddressToId(&routerAddress);
      std::string rname = "Router_" + strop::vecString<u32>(routerAddress, '-');

      // make router
      u32 rRadix = std::get<2>(radices_.at(level));
      routers_.at(level).at(col) =
          Router::create(rname, this, this, routerId, routerAddress, rRadix,
                         numVcs_, _metadataHandler, _settings["router"]);
    }
  }

  assert(_settings["internal_channels"].is_array());
  assert(_settings["internal_channels"].size() == numLevels_ - 1);

  // create internal channels, link routers via channels
  for (u32 level = 0; level < numLevels_ - 1; level++) {
    for (u32 col = 0; col < routersAtLevel_.at(level); col++) {
      // "up" = level + 1
      // "down" = level
      // group = the group of the next level not current
      // sub-groups = sub-groups within group, i.e. groups in current level

      // current level up ports
      u32 upPorts = std::get<1>(radices_.at(level));
      // upper level down ports
      u32 downPorts = std::get<0>(radices_.at(level + 1));

      // group structure
      u32 upRouters = routersAtLevelPerGroup_.at(level + 1);
      u32 upStripe = routersAtLevelPerGroup_.at(level);
      u32 numSubGroups = downPorts;

      // current group and sub-groups within
      // sub-groups are counted as relative to group
      u32 group = col / (numSubGroups * upStripe);
      u32 subGroup = (col / upStripe) % downPorts;
      u32 routerInSubGroup = col % upStripe;
      u32 basePort = std::get<0>(radices_.at(level));

      // create this router
      Router* thisRouter = routers_.at(level).at(col);
      for (u32 p = 0; p < upPorts; p++) {
        // this router
        // u32 thisLevel = level;
        // u32 thisCol = col;
        u32 thisPort = basePort + p;

        // that router
        u32 thatLevel = level + 1;
        u32 thatCol = (upRouters * group) + (routerInSubGroup) + (p * upStripe);
        u32 thatPort = subGroup;

        // create that router
        Router* thatRouter = routers_.at(thatLevel).at(thatCol);
        // create channels
        std::string upChannelName = "UpChannel_" + std::to_string(level) + ":" +
                                    std::to_string(col) + ":" +
                                    std::to_string(p);
        Channel* up = new Channel(upChannelName, this, numVcs_,
                                  _settings["internal_channels"][level]);
        internalChannels_.push_back(up);
        std::string downChannelName = "DownChannel_" + std::to_string(level) +
                                      ":" + std::to_string(col) + ":" +
                                      std::to_string(p);
        Channel* down = new Channel(downChannelName, this, numVcs_,
                                    _settings["internal_channels"][level]);
        internalChannels_.push_back(down);
        // link routers
        thisRouter->setInputChannel(thisPort, down);
        thisRouter->setOutputChannel(thisPort, up);
        thatRouter->setInputChannel(thatPort, up);
        thatRouter->setOutputChannel(thatPort, down);
      }
    }
  }

  // create interfaces, external channels, link together
  interfaces_.resize(numInterfaces_, nullptr);
  u32 interfacesPerRouter = interfacesPerGroup_.at(0);
  for (u32 col = 0; col < routersAtLevel_.at(0); col++) {
    // get the router now, for later linking with terminals
    Router* router = routers_.at(0).at(col);
    const std::vector<u32>& routerAddress = router->address();

    for (u32 iface = 0; iface < interfacesPerRouter; iface++) {
      // interface id and address
      u32 interfaceId = col * interfacesPerRouter + iface;
      std::vector<u32> interfaceAddress;
      translateInterfaceIdToAddress(interfaceId, &interfaceAddress);

      // create interface
      std::string interfaceName =
          "Interface_" + strop::vecString<u32>(interfaceAddress, '-');
      Interface* interface = Interface::create(
          interfaceName, this, this, interfaceId, interfaceAddress,
          interfacePorts_, numVcs_, _metadataHandler, _settings["interface"]);
      interfaces_.at(interfaceId) = interface;

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

  // total number of routers
  numRouters_ = 0;
  for (const auto& n : routersAtLevel_) {
    numRouters_ += n;
  }

  // clear the protocol class info
  clearProtocolClassInfo();

  for (u32 id = 0; id < numInterfaces_; id++) {
    assert(getInterface(id) != nullptr);
  }

  for (u32 id = 0; id < numRouters_; id++) {
    assert(getRouter(id) != nullptr);
  }
}

Network::~Network() {
  // delete routers
  for (u32 level = 0; level < numLevels_; level++) {
    u32 levelRouters = routersAtLevel_.at(level);
    for (u32 col = 0; col < levelRouters; col++) {
      delete routers_.at(level).at(col);
    }
  }

  // delete interfaces
  for (auto it = interfaces_.begin(); it != interfaces_.end(); ++it) {
    Interface* interface = *it;
    delete interface;
  }

  // delete channels
  for (auto it = internalChannels_.begin(); it != internalChannels_.end();
       ++it) {
    Channel* c = *it;
    delete c;
  }
  for (auto it = externalChannels_.begin(); it != externalChannels_.end();
       ++it) {
    Channel* c = *it;
    delete c;
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
  return RoutingAlgorithm::create(_name, _parent, _router, settings.baseVc,
                                  settings.numVcs, _inputPort, _inputVc,
                                  &radices_, interfacePorts_, settings.routing);
}

u32 Network::numRouters() const {
  return numRouters_;
}

u32 Network::numInterfaces() const {
  return numInterfaces_;
}

Router* Network::getRouter(u32 _id) const {
  std::vector<u32> routerAddress;
  translateRouterIdToAddress(_id, &routerAddress);
  u32 level = routerAddress.at(0);
  u32 col = routerAddress.at(1);
  return routers_.at(level).at(col);
}

Interface* Network::getInterface(u32 _id) const {
  return interfaces_.at(_id);
}

void Network::translateInterfaceIdToAddress(u32 _id,
                                            std::vector<u32>* _address) const {
  FatTree::translateInterfaceIdToAddress(numLevels_, interfacesPerGroup_, _id,
                                         _address);
}

u32 Network::translateInterfaceAddressToId(
    const std::vector<u32>* _address) const {
  return FatTree::translateInterfaceAddressToId(numLevels_, interfacesPerGroup_,
                                                _address);
}

void Network::translateRouterIdToAddress(u32 _id,
                                         std::vector<u32>* _address) const {
  FatTree::translateRouterIdToAddress(numLevels_, routersAtLevel_, _id,
                                      _address);
}

u32 Network::translateRouterAddressToId(
    const std::vector<u32>* _address) const {
  return FatTree::translateRouterAddressToId(numLevels_, routersAtLevel_,
                                             _address);
}

u32 Network::computeMinimalHops(const std::vector<u32>* _source,
                                const std::vector<u32>* _destination) const {
  return FatTree::computeMinimalHops(_source, _destination);
}

void Network::collectChannels(std::vector<Channel*>* _channels) {
  for (auto it = externalChannels_.begin(); it != externalChannels_.end();
       ++it) {
    Channel* c = *it;
    _channels->push_back(c);
  }
  for (auto it = internalChannels_.begin(); it != internalChannels_.end();
       ++it) {
    Channel* c = *it;
    _channels->push_back(c);
  }
}

}  // namespace FatTree

registerWithObjectFactory("fat_tree", ::Network, FatTree::Network,
                          NETWORK_ARGS);
