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
#ifndef NETWORK_FATTREE_COMMONANCESTORROUTINGALGORITHM_H_
#define NETWORK_FATTREE_COMMONANCESTORROUTINGALGORITHM_H_

#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "colhash/tuplehash.h"
#include "event/Component.h"
#include "network/fattree/RoutingAlgorithm.h"
#include "nlohmann/json.hpp"
#include "prim/prim.h"
#include "rnd/Random.h"
#include "router/Router.h"
#include "routing/Reduction.h"
#include "routing/mode.h"

namespace FatTree {

class CommonAncestorRoutingAlgorithm : public RoutingAlgorithm {
 public:
  CommonAncestorRoutingAlgorithm(
      const std::string& _name, const Component* _parent, Router* _router,
      u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc,
      const std::vector<std::tuple<u32, u32, u32> >* _radices,
      u32 _interfacePorts, nlohmann::json _settings);
  ~CommonAncestorRoutingAlgorithm();

 protected:
  void processRequest(
      Flit* _flit, RoutingAlgorithm::Response* _response) override;

 private:
  enum class Selection { kAll, kFlowCache, kFlowHash };

  void addPort(u32 _port, u32 _hops);
  Selection parseSelection(const std::string& _selection) const;
  u64 flowCache(const Flit* _flit);
  u64 flowHash(const Flit* _flit);

  const RoutingMode mode_;
  const bool leastCommonAncestor_;
  const Selection selection_;
  const u64 randomId_;
  rnd::Random random_;
  Reduction* reduction_;
  std::unordered_map<u64, u32> flowCache_;
};

}  // namespace FatTree

#endif  // NETWORK_FATTREE_COMMONANCESTORROUTINGALGORITHM_H_
