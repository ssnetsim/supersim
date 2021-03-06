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
#include "network/mesh/DimOrderRoutingAlgorithm.h"

#include <cassert>
#include <tuple>
#include <unordered_set>

#include "factory/ObjectFactory.h"
#include "network/mesh/util.h"
#include "strop/strop.h"
#include "types/Message.h"
#include "types/Packet.h"

namespace Mesh {

DimOrderRoutingAlgorithm::DimOrderRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    u32 _interfacePorts, nlohmann::json _settings)
    : RoutingAlgorithm(_name, _parent, _router, _baseVc, _numVcs, _inputPort,
                       _inputVc, _dimensionWidths, _dimensionWeights,
                       _concentration, _interfacePorts, _settings),
      mode_(parseRoutingMode(_settings["mode"].get<std::string>())) {
  // VC set mapping:
  //  0 = all routing
  assert(numVcs_ >= 1);

  // create the reduction
  reduction_ = Reduction::create("Reduction", this, _router, mode_, false,
                                 _settings["reduction"]);
}

DimOrderRoutingAlgorithm::~DimOrderRoutingAlgorithm() {
  delete reduction_;
}

void DimOrderRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {
  u32 outputPort;

  // ex: [x,y,z]
  const std::vector<u32>& routerAddress = router_->address();
  // ex: [c,x,y,z]
  const std::vector<u32>* destinationAddress =
      _flit->packet()->message()->getDestinationAddress();
  assert(routerAddress.size() == (destinationAddress->size() - 1));

  // determine the next dimension to work on
  u32 dim;
  u32 numDimensions = dimensionWidths_.size();
  u32 portBase = concentration_;
  u32 dimWeight = U32_MAX;
  for (dim = 0; dim < numDimensions; dim++) {
    dimWeight = dimensionWeights_.at(dim);
    if (routerAddress.at(dim) != destinationAddress->at(dim + 1)) {
      break;
    }
    portBase += 2 * dimWeight;
  }

  // create a temporary router address with a dummy concentration for use with
  //  'util.h' 'computeMinimalHops() frunction'
  std::vector<u32> tempRA = std::vector<u32>(1 + routerAddress.size());
  tempRA.at(0) = U32_MAX;  // dummy
  for (u32 ind = 1; ind < tempRA.size(); ind++) {
    tempRA.at(ind) = routerAddress.at(ind - 1);
  }

  // determine minimum number of hops to destination for reduction algorithm
  u32 hops = computeMinimalHops(&tempRA, destinationAddress, numDimensions,
                                dimensionWidths_);

  // test if already at destination router
  if (dim == routerAddress.size()) {
    u32 basePort = destinationAddress->at(0) * interfacePorts_;
    for (u32 offset = 0; offset < interfacePorts_; offset++) {
      addPort(basePort + offset, hops, 0);
    }
  } else {
    // more router-to-router hops needed
    u32 src = routerAddress.at(dim);
    u32 dst = destinationAddress->at(dim + 1);
    assert(src != dst);

    // determine direction
    bool right = dst > src;

    // choose output port, figure out next router in this dimension
    u32 next;
    if (right) {
      outputPort = portBase;
      assert(src < (dimensionWidths_.at(dim) - 1));
      next = src + 1;
    } else {
      outputPort = portBase + dimWeight;
      assert(src > 0);
      next = src - 1;
    }
    assert(next != src);

    // add all ports connecting to the destination (based on weight)
    for (u32 wInd = 0; wInd < dimWeight; wInd++) {
      addPort(outputPort + wInd, hops, 0);
    }
  }

  // reduction phase
  const std::unordered_set<std::tuple<u32, u32>>* outputs =
      reduction_->reduce(nullptr);
  for (const auto& t : *outputs) {
    u32 port = std::get<0>(t);
    u32 vc = std::get<1>(t);
    if (vc == U32_MAX) {
      for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc += 1) {
        _response->add(port, vc);
      }
    } else {
      _response->add(port, vc);
    }
  }
}

void DimOrderRoutingAlgorithm::addPort(u32 _port, u32 _hops, u32 vcSet) {
  if (routingModeIsPort(mode_)) {
    // add the port as a whole
    f64 cong = portCongestion(mode_, router_, inputPort_, inputVc_, _port);
    reduction_->add(_port, U32_MAX, _hops, cong);
  } else {
    for (u32 vc = baseVc_ + vcSet; vc < baseVc_ + numVcs_; vc += 1) {
      f64 cong = router_->congestionStatus(inputPort_, inputVc_, _port, vc);
      reduction_->add(_port, vc, _hops, cong);
    }
  }
}
}  // namespace Mesh

registerWithObjectFactory("dimension_order", Mesh::RoutingAlgorithm,
                          Mesh::DimOrderRoutingAlgorithm,
                          MESH_ROUTINGALGORITHM_ARGS);
