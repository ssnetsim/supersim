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
#include "network/butterfly/DestTagRoutingAlgorithm.h"

#include <cassert>

#include "factory/ObjectFactory.h"
#include "types/Message.h"
#include "types/Packet.h"

namespace Butterfly {

DestTagRoutingAlgorithm::DestTagRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc, u32 _numPorts,
    u32 _numStages, u32 _interfacePorts, u32 _stage, nlohmann::json _settings)
    : RoutingAlgorithm(_name, _parent, _router, _baseVc, _numVcs, _inputPort,
                       _inputVc, _numPorts, _numStages, _interfacePorts, _stage,
                       _settings) {}

DestTagRoutingAlgorithm::~DestTagRoutingAlgorithm() {}

void DestTagRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {
  const std::vector<u32>* destinationAddress =
      _flit->packet()->message()->getDestinationAddress();
  assert(destinationAddress->size() == numStages_);

  std::vector<u32> outputPorts;
  if (stage_ != numStages_ - 1) {
    // pick the output port using the "tag" in the address
    outputPorts.push_back(destinationAddress->at(stage_));
  } else {
    // use the tag
    u32 basePort = destinationAddress->at(stage_) * interfacePorts_;
    for (u32 offset = 0; offset < interfacePorts_; offset++) {
      u32 port = basePort + offset;
      outputPorts.push_back(port);
    }
  }

  // select all VCs in the output ports
  for (u32 port : outputPorts) {
    for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
      _response->add(port, vc);
    }
  }
}

}  // namespace Butterfly

registerWithObjectFactory("dest_tag", Butterfly::RoutingAlgorithm,
                          Butterfly::DestTagRoutingAlgorithm,
                          BUTTERFLY_ROUTINGALGORITHM_ARGS);
