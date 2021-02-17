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
#include "network/singlerouter/DirectRoutingAlgorithm.h"

#include <cassert>
#include <tuple>
#include <vector>

#include "factory/ObjectFactory.h"
#include "types/Message.h"
#include "types/Packet.h"

namespace SingleRouter {

DirectRoutingAlgorithm::DirectRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc, u32 _concentration,
    u32 _interfacePorts, nlohmann::json _settings)
    : RoutingAlgorithm(_name, _parent, _router, _baseVc, _numVcs, _inputPort,
                       _inputVc, _concentration, _interfacePorts, _settings),
      adaptive_(_settings["adaptive"].get<bool>()) {
  assert(!_settings["adaptive"].is_null());
}

DirectRoutingAlgorithm::~DirectRoutingAlgorithm() {}

void DirectRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {
  // direct route to destination
  const std::vector<u32>* destinationAddress =
      _flit->packet()->message()->getDestinationAddress();
  std::vector<u32> outputPorts;
  u32 basePort = destinationAddress->at(0) * interfacePorts_;
  for (u32 offset = 0; offset < interfacePorts_; offset++) {
    u32 outputPort = basePort + offset;
    assert(outputPort < concentration_);
    outputPorts.push_back(outputPort);
  }

  if (!adaptive_) {
    // select all VCs in the output port
    for (u32 port : outputPorts) {
      for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
        _response->add(port, vc);
      }
    }
  } else {
    // select all minimally congested VCs
    std::vector<std::tuple<u32, u32>> minCongVcs;
    f64 minCong = F64_POS_INF;
    for (u32 port : outputPorts) {
      for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
        f64 cong = router_->congestionStatus(inputPort_, inputVc_, port, vc);
        if (cong < minCong) {
          minCong = cong;
          minCongVcs.clear();
        }
        if (cong <= minCong) {
          minCongVcs.push_back(std::make_tuple(port, vc));
        }
        for (const std::tuple<u32, u32> portVc : minCongVcs) {
          _response->add(std::get<0>(portVc), std::get<1>(portVc));
        }
      }
    }
  }
}

}  // namespace SingleRouter

registerWithObjectFactory("direct", SingleRouter::RoutingAlgorithm,
                          SingleRouter::DirectRoutingAlgorithm,
                          SINGLEROUTER_ROUTINGALGORITHM_ARGS);
