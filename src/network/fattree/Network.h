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
#ifndef NETWORK_FATTREE_NETWORK_H_
#define NETWORK_FATTREE_NETWORK_H_

#include <string>
#include <tuple>
#include <vector>

#include "event/Component.h"
#include "interface/Interface.h"
#include "network/Channel.h"
#include "network/Network.h"
#include "nlohmann/json.hpp"
#include "prim/prim.h"
#include "router/Router.h"

namespace FatTree {

class Network : public ::Network {
 public:
  Network(const std::string& _name, const Component* _parent,
          MetadataHandler* _metadataHandler, nlohmann::json _settings);
  ~Network();

  // this is the injection algorithm factory for this network
  ::InjectionAlgorithm* createInjectionAlgorithm(
      u32 _inputPc, const std::string& _name, const Component* _parent,
      Interface* _interface) override;

  // this is the routing algorithm factory for this network
  ::RoutingAlgorithm* createRoutingAlgorithm(u32 _inputPort, u32 _inputVc,
                                             const std::string& _name,
                                             const Component* _parent,
                                             Router* _router) override;

  // Network
  u32 numRouters() const override;
  u32 numInterfaces() const override;
  Router* getRouter(u32 _id) const override;
  Interface* getInterface(u32 _id) const override;
  void translateInterfaceIdToAddress(u32 _id,
                                     std::vector<u32>* _address) const override;
  u32 translateInterfaceAddressToId(
      const std::vector<u32>* _address) const override;
  void translateRouterIdToAddress(u32 _id,
                                  std::vector<u32>* _address) const override;
  u32 translateRouterAddressToId(
      const std::vector<u32>* _address) const override;
  u32 computeMinimalHops(const std::vector<u32>* _source,
                         const std::vector<u32>* _destination) const override;

 protected:
  void collectChannels(std::vector<Channel*>* _channels) override;

 private:
  u32 numRouters_;
  u32 numInterfaces_;
  u32 interfacePorts_;
  u32 numLevels_;
  std::vector<u32> routersAtLevel_;
  std::vector<u32> interfacesPerGroup_;
  std::vector<u32> routersAtLevelPerGroup_;
  std::vector<u32> totalGroups_;
  std::vector<std::tuple<u32, u32, u32>> radices_;  // down, up, total

  std::vector<std::vector<Router*>> routers_;
  std::vector<Interface*> interfaces_;

  std::vector<Channel*> internalChannels_;
  std::vector<Channel*> externalChannels_;
};

}  // namespace FatTree

#endif  // NETWORK_FATTREE_NETWORK_H_
