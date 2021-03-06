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
#ifndef NETWORK_MESH_ROUTINGALGORITHM_H_
#define NETWORK_MESH_ROUTINGALGORITHM_H_

#include <string>
#include <vector>

#include "event/Component.h"
#include "nlohmann/json.hpp"
#include "prim/prim.h"
#include "router/Router.h"
#include "routing/RoutingAlgorithm.h"

#define MESH_ROUTINGALGORITHM_ARGS                                   \
  const std::string&, const Component*, Router*, u32, u32, u32, u32, \
      const std::vector<u32>&, const std::vector<u32>&, u32, u32,    \
      nlohmann::json

namespace Mesh {

class RoutingAlgorithm : public ::RoutingAlgorithm {
 public:
  RoutingAlgorithm(const std::string& _name, const Component* _parent,
                   Router* _router, u32 _baseVc, u32 _numVcs, u32 _inputPort,
                   u32 _inputVc, const std::vector<u32>& _dimensionWidths,
                   const std::vector<u32>& _dimensionWeights,
                   u32 _concentration, u32 _interfacePorts,
                   nlohmann::json _settings);
  virtual ~RoutingAlgorithm();

  // this is a routing algorithm factory for the mesh topology
  static RoutingAlgorithm* create(MESH_ROUTINGALGORITHM_ARGS);

 protected:
  const std::vector<u32> dimensionWidths_;
  const std::vector<u32> dimensionWeights_;
  const u32 concentration_;
  const u32 interfacePorts_;
  const u32 inputPortDim_;
};

}  // namespace Mesh

#endif  // NETWORK_MESH_ROUTINGALGORITHM_H_
